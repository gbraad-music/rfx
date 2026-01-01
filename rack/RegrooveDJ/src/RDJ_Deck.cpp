#include "plugin.hpp"
#include "../../regroove_components.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <cstring>
#include <osdialog.h>

#define DR_WAV_IMPLEMENTATION
#include "../dep/dr_wav.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include "../dep/minimp3_ex.h"

// Audio file loader (WAV and MP3)
struct AudioFile {
	std::vector<float> dataL;
	std::vector<float> dataR;
	int sampleRate = 44100;
	float duration = 0.0f;
	std::string fileName;

	bool load(const std::string& path) {
		// Detect file type by extension
		std::string ext = path.substr(path.find_last_of(".") + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == "mp3") {
			return loadMP3(path);
		} else {
			return loadWAV(path);
		}
	}

	bool loadWAV(const std::string& path) {
		unsigned int channels;
		unsigned int loadSampleRate;
		drwav_uint64 totalFrameCount;

		// Load WAV file using dr_wav
		float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
			path.c_str(),
			&channels,
			&loadSampleRate,
			&totalFrameCount,
			NULL
		);

		if (!pSampleData) {
			WARN("Failed to load WAV file: %s", path.c_str());
			return false;
		}

		sampleRate = loadSampleRate;
		duration = (float)totalFrameCount / sampleRate;

		INFO("Loaded WAV: %d Hz, %d ch, %llu frames, %.2f sec",
		     sampleRate, channels, totalFrameCount, duration);

		// Deinterleave samples into left and right channels
		dataL.resize(totalFrameCount);
		dataR.resize(totalFrameCount);

		for (drwav_uint64 i = 0; i < totalFrameCount; i++) {
			dataL[i] = pSampleData[i * channels];
			dataR[i] = (channels > 1) ? pSampleData[i * channels + 1] : pSampleData[i * channels];
		}

		// Free the loaded data
		drwav_free(pSampleData, NULL);

		// Extract filename
		size_t lastSlash = path.find_last_of("/\\");
		fileName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

		return true;
	}

	bool loadMP3(const std::string& path) {
		mp3dec_t mp3d;
		mp3dec_file_info_t info;

		mp3dec_init(&mp3d);
		memset(&info, 0, sizeof(info));

		// Load MP3 file using minimp3
		int loadResult;

#ifdef _WIN32
		// On Windows, try wide-character API first, then fallback
		// Try UTF-8 conversion first
		int wideLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
		if (wideLen > 0) {
			wchar_t* widePath = new wchar_t[wideLen];
			MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, widePath, wideLen);
			loadResult = mp3dec_load_w(&mp3d, widePath, &info, NULL, NULL);
			delete[] widePath;

			// If that failed, try regular API as fallback
			if (loadResult != 0) {
				INFO("Wide-char API failed (error %d), trying regular API", loadResult);
				loadResult = mp3dec_load(&mp3d, path.c_str(), &info, NULL, NULL);
			}
		} else {
			// Fallback to regular API
			loadResult = mp3dec_load(&mp3d, path.c_str(), &info, NULL, NULL);
		}
#else
		loadResult = mp3dec_load(&mp3d, path.c_str(), &info, NULL, NULL);
#endif

		if (loadResult != 0) {
			const char* errMsg = "Unknown error";
			switch (loadResult) {
				case -1: errMsg = "Invalid parameter"; break;
				case -2: errMsg = "Memory allocation failed"; break;
				case -3: errMsg = "File I/O error (file not found or can't open)"; break;
				case -4: errMsg = "File too small or invalid MP3 format"; break;
				case -5: errMsg = "Decode error (incompatible format)"; break;
			}
			WARN("Failed to load MP3 file: %s (error %d: %s)", path.c_str(), loadResult, errMsg);
			return false;
		}

		if (!info.buffer || info.samples == 0) {
			WARN("MP3 file has no data: %s", path.c_str());
			if (info.buffer) free(info.buffer);
			return false;
		}

		// Sanity check the data
		if (info.channels < 1 || info.channels > 8) {
			WARN("MP3 has invalid channel count: %d", info.channels);
			free(info.buffer);
			return false;
		}

		if (info.hz < 8000 || info.hz > 192000) {
			WARN("MP3 has invalid sample rate: %d", info.hz);
			free(info.buffer);
			return false;
		}

		size_t totalFrames = info.samples / info.channels;

		// Sanity check: max 1 billion frames (~6 hours at 48kHz)
		if (totalFrames == 0 || totalFrames > 1000000000) {
			WARN("MP3 has invalid frame count: %zu", totalFrames);
			free(info.buffer);
			return false;
		}

		sampleRate = info.hz;
		duration = (float)totalFrames / sampleRate;

		INFO("Loaded MP3: %d Hz, %d ch, %zu frames, %.2f sec",
		     sampleRate, info.channels, totalFrames, duration);

		// Deinterleave samples into left and right channels
		try {
			dataL.resize(totalFrames);
			dataR.resize(totalFrames);
		} catch (const std::exception& e) {
			WARN("Failed to allocate memory for MP3: %s", e.what());
			free(info.buffer);
			return false;
		}

		// Convert from mp3d_sample_t (float with MINIMP3_FLOAT_OUTPUT) to our format
		for (size_t i = 0; i < totalFrames; i++) {
			dataL[i] = info.buffer[i * info.channels];
			dataR[i] = (info.channels > 1) ? info.buffer[i * info.channels + 1] : info.buffer[i * info.channels];
		}

		// Free the loaded data
		free(info.buffer);

		// Extract filename
		size_t lastSlash = path.find_last_of("/\\");
		fileName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

		return true;
	}
};

struct RDJ_Deck : Module {
	enum ParamId {
		PAD1_PARAM,
		PLAY_PARAM,
		PAD3_PARAM,
		PAD4_PARAM,
		PAD5_PARAM,
		PAD6_PARAM,
		TEMPO_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		PFL_L_OUTPUT,
		PFL_R_OUTPUT,
		AUDIO_L_OUTPUT,
		AUDIO_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	AudioFile audio;
	double playPosition = 0.0;
	std::atomic<bool> playing{false};
	std::atomic<bool> fileLoaded{false};
	std::atomic<bool> muted{false};
	std::atomic<bool> looping{true};  // Loop enabled by default
	std::atomic<bool> pflActive{false};  // PFL (Pre-Fader Listening) state
	float smoothTempo = 1.0f;
	std::atomic<bool> loading{false};  // Track if currently loading
	std::atomic<bool> shouldStopLoading{false};  // Signal to stop loading thread
	std::mutex swapMutex;  // ONLY for protecting audio data swap
	std::thread loadingThread;  // Track the loading thread
	std::atomic<size_t> audioSize{0};  // Safe to read from any thread

	RDJ_Deck() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configButton(PAD1_PARAM, "Sync");
		configButton(PLAY_PARAM, "Play/Stop");
		configButton(PAD3_PARAM, "Cue");
		configButton(PAD4_PARAM, "Loop");
		configButton(PAD5_PARAM, "Mute");
		configButton(PAD6_PARAM, "PFL");
		configParam(TEMPO_PARAM, 0.9f, 1.1f, 1.0f, "Tempo", "%", -100.f, 100.f, -100.f);

		configOutput(PFL_L_OUTPUT, "PFL Left");
		configOutput(PFL_R_OUTPUT, "PFL Right");
		configOutput(AUDIO_L_OUTPUT, "Left audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");
	}

	~RDJ_Deck() {
		// Stop any loading thread
		shouldStopLoading = true;
		if (loadingThread.joinable()) {
			loadingThread.join();
		}
	}

	void loadFile(const std::string& path) {
		// Stop any existing loading thread
		shouldStopLoading = true;
		if (loadingThread.joinable()) {
			loadingThread.join();
		}
		shouldStopLoading = false;

		loadingThread = std::thread([this, path]() {
			// CRITICAL: Clear fileLoaded FIRST to stop UI from accessing old data
			{
				std::lock_guard<std::mutex> lock(swapMutex);
				fileLoaded = false;
				loading = true;
				playing = false;
			}

			AudioFile newAudio;
			bool success = false;

			try {
				// Check if we should stop before loading
				if (!shouldStopLoading) {
					success = newAudio.load(path);
				}
			} catch (...) {
				WARN("Exception loading audio file: %s", path.c_str());
				success = false;
			}

			// Check again before swapping - might have been cancelled
			if (success && !shouldStopLoading) {
				// Lock for the swap operation
				std::lock_guard<std::mutex> lock(swapMutex);
				playPosition = 0.0;
				audio = std::move(newAudio);
				audioSize = audio.dataL.size();
				fileLoaded = true;
				loading = false;
			} else {
				std::lock_guard<std::mutex> lock(swapMutex);
				loading = false;
			}
		});
	}

	void process(const ProcessArgs& args) override {
		// Handle play button
		if (params[PLAY_PARAM].getValue() > 0.5f) {
			if (fileLoaded) {
				// Need to lock to check size
				if (swapMutex.try_lock()) {
					size_t dataSize = audio.dataL.size();
					playing = !playing;
					if (playing && playPosition >= dataSize) {
						playPosition = 0.0;
					}
					swapMutex.unlock();
				}
			}
			params[PLAY_PARAM].setValue(0.f);
		}

		// Handle loop button (PAD4) - toggle loop state
		if (params[PAD4_PARAM].getValue() > 0.5f) {
			looping = !looping;
			params[PAD4_PARAM].setValue(0.f);
		}

		// Handle mute button (PAD5) - toggle mute state
		if (params[PAD5_PARAM].getValue() > 0.5f) {
			muted = !muted;
			params[PAD5_PARAM].setValue(0.f);
		}

		// Handle PFL button (PAD6) - toggle PFL state
		if (params[PAD6_PARAM].getValue() > 0.5f) {
			pflActive = !pflActive;
			params[PAD6_PARAM].setValue(0.f);
		}

		// Playback - try to lock, skip if we can't
		if (loading || !playing || !fileLoaded) {
			outputs[AUDIO_L_OUTPUT].setVoltage(0.f);
			outputs[AUDIO_R_OUTPUT].setVoltage(0.f);
			return;
		}

		if (!swapMutex.try_lock()) {
			// Can't lock, output silence this frame
			outputs[AUDIO_L_OUTPUT].setVoltage(0.f);
			outputs[AUDIO_R_OUTPUT].setVoltage(0.f);
			return;
		}

		// We have the lock now
		size_t dataSize = audio.dataL.size();
		if (dataSize == 0) {
			swapMutex.unlock();
			outputs[AUDIO_L_OUTPUT].setVoltage(0.f);
			outputs[AUDIO_R_OUTPUT].setVoltage(0.f);
			return;
		}

		float tempo = params[TEMPO_PARAM].getValue();
		smoothTempo += (tempo - smoothTempo) * 0.001f;

		size_t pos = (size_t)playPosition;
		if (pos >= dataSize) pos = 0;

		float leftSample = audio.dataL[pos];
		float rightSample = audio.dataR[pos];
		int sampleRate = audio.sampleRate;

		swapMutex.unlock();

		// Output audio (outside lock)
		float gain = muted ? 0.f : 5.f;
		outputs[AUDIO_L_OUTPUT].setVoltage(leftSample * gain);
		outputs[AUDIO_R_OUTPUT].setVoltage(rightSample * gain);

		// PFL output (Pre-Fader Listening - always at full level when active)
		if (pflActive) {
			outputs[PFL_L_OUTPUT].setVoltage(leftSample * 5.f);
			outputs[PFL_R_OUTPUT].setVoltage(rightSample * 5.f);
		} else {
			outputs[PFL_L_OUTPUT].setVoltage(0.f);
			outputs[PFL_R_OUTPUT].setVoltage(0.f);
		}

		// Advance playback
		double sampleRateRatio = (double)sampleRate / args.sampleRate;
		playPosition += sampleRateRatio * smoothTempo;

		// Loop or stop at end
		if (playPosition >= dataSize) {
			if (looping) {
				playPosition = 0.0;  // Loop back to start
			} else {
				playing = false;  // Stop playback
				playPosition = 0.0;
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		if (fileLoaded && !audio.fileName.empty()) {
			json_object_set_new(rootJ, "fileName", json_string(audio.fileName.c_str()));
		}
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* fileNameJ = json_object_get(rootJ, "fileName");
		if (fileNameJ) {
			audio.fileName = json_string_value(fileNameJ);
		}
	}
};

// Overview waveform - shows entire file with playhead
struct OverviewWaveformDisplay : TransparentWidget {
	RDJ_Deck* module;

	void draw(const DrawArgs& args) override {
		// Red border
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgStrokeColor(args.vg, REGROOVE_RED);
		nvgStrokeWidth(args.vg, 2);
		nvgStroke(args.vg);

		if (!module || module->loading || !module->fileLoaded) {
			nvgFontSize(args.vg, 10);
			nvgFontFaceId(args.vg, APP->window->uiFont->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, nvgRGB(0x55, 0x55, 0x55));
			const char* text = (!module || !module->fileLoaded) ? "Right-click to load file" : "Loading...";
			nvgText(args.vg, box.size.x / 2, box.size.y / 2, text, NULL);
			Widget::draw(args);
			return;
		}

		// Read atomic size - safe from any thread
		size_t dataSize = module->audioSize;
		double playPos = module->playPosition;

		// Double-check flags haven't changed
		if (module->loading || !module->fileLoaded || dataSize == 0) {
			Widget::draw(args);
			return;
		}

		if (dataSize == 0 || dataSize > 1000000000) {  // Sanity check
			Widget::draw(args);
			return;
		}

		// Draw entire waveform WITHOUT holding the lock
		// This allows audio thread to run freely
		// Worst case: we might read slightly stale data during file loading
		const float samplesPerPixel = (float)dataSize / box.size.x;
		const float centerY = box.size.y / 2;
		const float scale = box.size.y * 0.4f;

		nvgStrokeColor(args.vg, REGROOVE_RED);
		nvgStrokeWidth(args.vg, 1);

		for (float x = 0; x < box.size.x; x += 1.0f) {
			size_t startSample = (size_t)(x * samplesPerPixel);
			size_t endSample = (size_t)((x + 1) * samplesPerPixel);

			if (startSample >= dataSize) break;
			if (endSample > dataSize) endSample = dataSize;

			float peak = 0.0f;
			for (size_t i = startSample; i < endSample; i++) {
				float absVal = fabs(module->audio.dataL[i]);
				if (absVal > peak) peak = absVal;
			}

			float height = peak * scale;
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, x, centerY - height);
			nvgLineTo(args.vg, x, centerY + height);
			nvgStroke(args.vg);
		}

		// Playhead (drawn without lock)
		if (dataSize > 0 && playPos >= 0 && playPos < dataSize) {
			float playheadX = (playPos / dataSize) * box.size.x;
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, playheadX, 0);
			nvgLineTo(args.vg, playheadX, box.size.y);
			nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
			nvgStrokeWidth(args.vg, 2);
			nvgStroke(args.vg);
		}

		Widget::draw(args);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (module && !module->loading && module->fileLoaded) {
				size_t dataSize = module->audio.dataL.size();
				if (dataSize > 0) {
					float newPos = e.pos.x / box.size.x;
					newPos = clamp(newPos, 0.f, 1.f);
					module->playPosition = newPos * dataSize;
					e.consume(this);
				}
			}
		}
		TransparentWidget::onButton(e);
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (module && !module->loading && module->fileLoaded) {
			size_t dataSize = module->audio.dataL.size();
			if (dataSize > 0) {
				float deltaPos = e.mouseDelta.x / box.size.x;
				module->playPosition += deltaPos * dataSize;
				if (module->playPosition < 0.0) module->playPosition = 0.0;
				if (module->playPosition >= dataSize) {
					module->playPosition = dataSize - 1;
				}
			}
		}
		TransparentWidget::onDragMove(e);
	}
};

// Detail waveform - shows zoomed view that scrolls with playback
struct DetailWaveformDisplay : TransparentWidget {
	RDJ_Deck* module;
	float zoomFactor = 30.0f;  // Show 1/30th of the file at a time

	void draw(const DrawArgs& args) override {
		// Red border
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgStrokeColor(args.vg, REGROOVE_RED);
		nvgStrokeWidth(args.vg, 2);
		nvgStroke(args.vg);

		if (!module || module->loading || !module->fileLoaded) {
			Widget::draw(args);
			return;
		}

		// Read atomic size - safe from any thread
		size_t dataSize = module->audioSize;
		double playPos = module->playPosition;

		// Double-check flags haven't changed
		if (module->loading || !module->fileLoaded || dataSize == 0) {
			Widget::draw(args);
			return;
		}

		if (dataSize == 0 || dataSize > 1000000000) {
			Widget::draw(args);
			return;
		}

		// Calculate visible window - ALWAYS centered on playhead
		const size_t windowSize = dataSize / zoomFactor;
		if (windowSize == 0) {
			Widget::draw(args);
			return;
		}

		// Clamp playPos to valid range
		if (playPos < 0) playPos = 0;
		if (playPos >= dataSize) playPos = dataSize - 1;

		// Calculate window centered on playhead (may go beyond file boundaries)
		const double halfWindow = windowSize / 2.0;
		const long long startSample = (long long)playPos - (long long)halfWindow;
		const long long endSample = startSample + windowSize;

		const float samplesPerPixel = (float)windowSize / box.size.x;
		const float centerY = box.size.y / 2;
		const float scale = box.size.y * 0.4f;

		nvgStrokeColor(args.vg, REGROOVE_RED);
		nvgStrokeWidth(args.vg, 1);

		// Draw waveform - playhead always at center
		for (float x = 0; x < box.size.x; x += 1.0f) {
			long long sampleStart = startSample + (long long)(x * samplesPerPixel);
			long long sampleEnd = startSample + (long long)((x + 1) * samplesPerPixel);

			// Skip if completely outside valid range
			if (sampleEnd < 0 || sampleStart >= (long long)dataSize) continue;

			// Clamp to valid range
			if (sampleStart < 0) sampleStart = 0;
			if (sampleEnd > (long long)dataSize) sampleEnd = dataSize;

			float peak = 0.0f;
			for (long long i = sampleStart; i < sampleEnd; i++) {
				float absVal = fabs(module->audio.dataL[i]);
				if (absVal > peak) peak = absVal;
			}

			float height = peak * scale;
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, x, centerY - height);
			nvgLineTo(args.vg, x, centerY + height);
			nvgStroke(args.vg);
		}

		// Playhead ALWAYS at center
		float playheadX = box.size.x / 2;

		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, playheadX, 0);
		nvgLineTo(args.vg, playheadX, box.size.y);
		nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
		nvgStrokeWidth(args.vg, 2);
		nvgStroke(args.vg);

		Widget::draw(args);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (module && !module->loading && module->fileLoaded) {
				const size_t dataSize = module->audio.dataL.size();
				if (dataSize > 0) {
					const size_t windowSize = dataSize / zoomFactor;
					if (windowSize > 0) {
						// Calculate click position relative to window
						float relativeX = (e.pos.x / box.size.x) - 0.5f;  // -0.5 to 0.5
						double currentPos = module->playPosition;
						if (currentPos < 0) currentPos = 0;
						if (currentPos >= dataSize) currentPos = dataSize - 1;

						double offset = relativeX * (double)windowSize;
						double newPos = currentPos + offset;
						if (newPos < 0) newPos = 0;
						if (newPos >= dataSize) newPos = dataSize - 1;
						module->playPosition = newPos;
						e.consume(this);
					}
				}
			}
		}
		TransparentWidget::onButton(e);
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (module && !module->loading && module->fileLoaded) {
			const size_t dataSize = module->audio.dataL.size();
			if (dataSize > 0) {
				const size_t windowSize = dataSize / zoomFactor;
				if (windowSize > 0) {
					float deltaPos = e.mouseDelta.x / box.size.x;
					module->playPosition += deltaPos * windowSize;
					if (module->playPosition < 0.0) module->playPosition = 0.0;
					if (module->playPosition >= dataSize) {
						module->playPosition = dataSize - 1;
					}
				}
			}
		}
		TransparentWidget::onDragMove(e);
	}
};

struct DeckPad : RegroovePad {
	RDJ_Deck* module;
	int padIndex;

	void step() override {
		if (module && padIndex == 1) {
			// PLAY pad - show red when playing
			if (module->playing) {
				setPadState(2);
			} else if (module->fileLoaded) {
				setPadState(1);
			} else {
				setPadState(0);
			}
		} else if (module && padIndex == 3) {
			// LOOP pad - show YELLOW when looping enabled
			if (module->looping) {
				setPadState(3);  // YELLOW = state 3
			} else {
				setPadState(0);  // GREY = state 0
			}
		} else if (module && padIndex == 4) {
			// MUTE pad - show RED when muted
			if (module->muted) {
				setPadState(1);  // RED = state 1
			} else {
				setPadState(0);  // GREY = state 0
			}
		} else if (module && padIndex == 5) {
			// PFL pad - show GREEN when active
			if (module->pflActive) {
				setPadState(2);  // GREEN = state 2
			} else {
				setPadState(0);  // GREY = state 0
			}
		} else {
			setPadState(0);
		}
		RegroovePad::step();
	}
};

struct RDJ_DeckWidget : ModuleWidget {
	RDJ_DeckWidget(RDJ_Deck* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RDJ_Deck.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(60.96, 5));
		titleLabel->text = "Deck";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Overview waveform display (top)
		OverviewWaveformDisplay* overview = new OverviewWaveformDisplay();
		overview->box.pos = mm2px(Vec(3, 16));
		overview->box.size = mm2px(Vec(54.96, 10));
		overview->module = module;
		addChild(overview);

		// Detail waveform display (bottom, scrolls with playback)
		DetailWaveformDisplay* detail = new DetailWaveformDisplay();
		detail->box.pos = mm2px(Vec(3, 26));
		detail->box.size = mm2px(Vec(54.96, 25));
		detail->module = module;
		addChild(detail);

		// Pad grid - positioned in red box area with VISIBLE spacing
		float padStartX = 5;
		float padStartY = 54;  // Moved up closer to waveform
		float padSpacing = 5;  // Clear visible gap between pads
		float padSize = 13;  // Pad size in mm

		// Row 1
		DeckPad* pad1 = createParam<DeckPad>(mm2px(Vec(padStartX, padStartY)), module, RDJ_Deck::PAD1_PARAM);
		pad1->module = module;
		pad1->padIndex = 0;
		pad1->label = "";  // No label
		addParam(pad1);

		DeckPad* playPad = createParam<DeckPad>(mm2px(Vec(padStartX + padSize + padSpacing, padStartY)), module, RDJ_Deck::PLAY_PARAM);
		playPad->module = module;
		playPad->padIndex = 1;
		playPad->label = "PLAY";
		addParam(playPad);

		// Row 2
		DeckPad* pad3 = createParam<DeckPad>(mm2px(Vec(padStartX, padStartY + padSize + padSpacing)), module, RDJ_Deck::PAD3_PARAM);
		pad3->module = module;
		pad3->padIndex = 2;
		pad3->label = "";  // No label
		addParam(pad3);

		DeckPad* pad4 = createParam<DeckPad>(mm2px(Vec(padStartX + padSize + padSpacing, padStartY + padSize + padSpacing)), module, RDJ_Deck::PAD4_PARAM);
		pad4->module = module;
		pad4->padIndex = 3;
		pad4->label = "LOOP";
		addParam(pad4);

		// Row 3
		DeckPad* pad5 = createParam<DeckPad>(mm2px(Vec(padStartX, padStartY + (padSize + padSpacing) * 2)), module, RDJ_Deck::PAD5_PARAM);
		pad5->module = module;
		pad5->padIndex = 4;
		pad5->label = "MUTE";
		addParam(pad5);

		DeckPad* pad6 = createParam<DeckPad>(mm2px(Vec(padStartX + padSize + padSpacing, padStartY + (padSize + padSpacing) * 2)), module, RDJ_Deck::PAD6_PARAM);
		pad6->module = module;
		pad6->padIndex = 5;
		pad6->label = "PFL";
		addParam(pad6);

		// Tempo fader - THICKER for visibility
		float faderWidth = 10;  // Much thicker
		float faderHeight = 50;
		float faderCenterX = 52;
		float faderTopY = 56;

		// Calculate top-left corner from center
		float faderLeft = faderCenterX - (faderWidth / 2);

		auto* tempoFader = createParam<RegrooveSlider>(mm2px(Vec(faderLeft, faderTopY)), module, RDJ_Deck::TEMPO_PARAM);
		tempoFader->box.size = mm2px(Vec(faderWidth, faderHeight));
		addParam(tempoFader);

		RegrooveLabel* tempoLabel = new RegrooveLabel();
		tempoLabel->box.pos = mm2px(Vec(47, 53));
		tempoLabel->box.size = mm2px(Vec(10, 3));
		tempoLabel->text = "Tempo";
		tempoLabel->fontSize = 7.0;
		addChild(tempoLabel);

		// PFL outputs (left side)
		RegrooveLabel* pflLabel = new RegrooveLabel();
		pflLabel->box.pos = mm2px(Vec(7.5, 110));
		pflLabel->box.size = mm2px(Vec(9, 3));
		pflLabel->text = "PFL";
		pflLabel->fontSize = 7.0;
		pflLabel->align = NVG_ALIGN_CENTER;
		addChild(pflLabel);

		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(7.5, 118.0)), module, RDJ_Deck::PFL_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(16.5, 118.0)), module, RDJ_Deck::PFL_R_OUTPUT));

		// Audio outputs (right side)
		RegrooveLabel* outLabel = new RegrooveLabel();
		outLabel->box.pos = mm2px(Vec(38, 110));
		outLabel->box.size = mm2px(Vec(20, 3));
		outLabel->text = "Out";
		outLabel->fontSize = 7.0;
		outLabel->align = NVG_ALIGN_CENTER;
		addChild(outLabel);

		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(43.48, 118.0)), module, RDJ_Deck::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(53.48, 118.0)), module, RDJ_Deck::AUDIO_R_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		RDJ_Deck* module = dynamic_cast<RDJ_Deck*>(this->module);
		if (!module)
			return;

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Audio File"));

		struct LoadFileItem : MenuItem {
			RDJ_Deck* module;
			void onAction(const event::Action& e) override {
				std::thread([=]() {
					char* path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, osdialog_filters_parse("Audio:wav,mp3,flac"));
					if (path) {
						module->loadFile(path);
						free(path);
					}
				}).detach();
			}
		};

		LoadFileItem* loadItem = new LoadFileItem();
		loadItem->text = "Load audio file...";
		loadItem->module = module;
		menu->addChild(loadItem);
	}
};

Model* modelRDJ_Deck = createModel<RDJ_Deck, RDJ_DeckWidget>("RDJ_Deck");
