#include "plugin.hpp"
#include "../../regroove_components.hpp"
#include "../../../synth/mod_player.h"
#include "../../../synth/mmd_player.h"
#include "../../../synth/ahx_player.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <cstring>
#include <algorithm>
#include <osdialog.h>

// Forward declaration
struct RM_Deck;

// Forward declaration of position callbacks
static void playerPositionCallback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data);
static void ahxPositionCallback(uint8_t subsong, uint16_t position, uint16_t row, void* user_data);

// Player type enum
enum class PlayerType {
	NONE,
	MOD,
	MED,
	AHX
};

// Custom pad widget for deck controls
struct DeckPad : RegroovePad {
	RM_Deck* module;
	int padIndex;

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			pressed = true;
			ParamQuantity* pq = getParamQuantity();
			if (pq) {
				pq->setValue(1.0f);
			}
		}
		else if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			pressed = false;
		}
		ParamWidget::onButton(e);
	}

	void step() override;  // Implemented after RM_Deck definition
};

struct RM_Deck : Module {
	enum ParamId {
		PAD1_PARAM,  // Set loop
		PLAY_PARAM,
		PAD3_PARAM,  // Pattern -
		PAD4_PARAM,  // Pattern +
		PAD5_PARAM,  // Mute (all)
		PAD6_PARAM,  // PFL
		CHAN1_MUTE_PARAM,  // Channel 1 mute
		CHAN2_MUTE_PARAM,  // Channel 2 mute
		CHAN3_MUTE_PARAM,  // Channel 3 mute
		CHAN4_MUTE_PARAM,  // Channel 4 mute
		TEMPO_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		PFL_L_OUTPUT,
		PFL_R_OUTPUT,
		AUDIO_L_OUTPUT,  // OUT L/1
		AUDIO_R_OUTPUT,  // OUT R/2
		AUDIO_3_OUTPUT,  // OUT 3
		AUDIO_4_OUTPUT,  // OUT 4
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	// Players (only one is active at a time)
	ModPlayer* modPlayer = nullptr;
	MedPlayer* medPlayer = nullptr;
	AhxPlayer* ahxPlayer = nullptr;
	PlayerType playerType = PlayerType::NONE;

	std::atomic<bool> playing{false};
	std::atomic<bool> fileLoaded{false};
	std::atomic<bool> muted{false};
	std::atomic<bool> pflActive{false};
	std::atomic<bool> singlePatternLoop{false};  // Track if loop is set to single pattern
	std::atomic<bool> channelMuted0{false};  // Channel 1 mute state
	std::atomic<bool> channelMuted1{false};  // Channel 2 mute state
	std::atomic<bool> channelMuted2{false};  // Channel 3 mute state
	std::atomic<bool> channelMuted3{false};  // Channel 4 mute state
	float smoothTempo = 1.0f;
	std::atomic<bool> loading{false};
	std::atomic<bool> shouldStopLoading{false};
	std::mutex swapMutex;
	std::thread loadingThread;
	std::atomic<bool> initialized{false};
	std::string currentFileName;

	// Position tracking from callback
	std::atomic<uint8_t> currentOrder{0};
	std::atomic<uint16_t> currentPattern{0};  // uint16_t to support AHX positions
	std::atomic<uint16_t> currentRow{0};

	// Helper methods for channel mute access
	bool getChannelMuted(int index) const {
		switch(index) {
			case 0: return channelMuted0;
			case 1: return channelMuted1;
			case 2: return channelMuted2;
			case 3: return channelMuted3;
			default: return false;
		}
	}

	void setChannelMuted(int index, bool muted) {
		switch(index) {
			case 0: channelMuted0 = muted; break;
			case 1: channelMuted1 = muted; break;
			case 2: channelMuted2 = muted; break;
			case 3: channelMuted3 = muted; break;
		}
	}

	// Audio buffer for rendering (stereo interleaved)
	static constexpr size_t BUFFER_SIZE = 512;
	float leftBuffer[BUFFER_SIZE];
	float rightBuffer[BUFFER_SIZE];
	float channel1Buffer[BUFFER_SIZE];
	float channel2Buffer[BUFFER_SIZE];
	float channel3Buffer[BUFFER_SIZE];
	float channel4Buffer[BUFFER_SIZE];
	size_t bufferPos = 0;
	bool bufferValid = false;

	RM_Deck() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configButton(PAD1_PARAM, "Set Loop");
		configButton(PLAY_PARAM, "Play/Stop");
		configButton(PAD3_PARAM, "Pattern -");
		configButton(PAD4_PARAM, "Pattern +");
		configButton(PAD5_PARAM, "Mute All");
		configButton(PAD6_PARAM, "PFL");
		configButton(CHAN1_MUTE_PARAM, "Channel 1 Mute");
		configButton(CHAN2_MUTE_PARAM, "Channel 2 Mute");
		configButton(CHAN3_MUTE_PARAM, "Channel 3 Mute");
		configButton(CHAN4_MUTE_PARAM, "Channel 4 Mute");
		configParam(TEMPO_PARAM, 0.9f, 1.1f, 1.0f, "Tempo", "%", -100.f, 100.f, -100.f);

		configOutput(PFL_L_OUTPUT, "PFL Left");
		configOutput(PFL_R_OUTPUT, "PFL Right");
		configOutput(AUDIO_L_OUTPUT, "Left audio / Channel 1");
		configOutput(AUDIO_R_OUTPUT, "Right audio / Channel 2");
		configOutput(AUDIO_3_OUTPUT, "Channel 3");
		configOutput(AUDIO_4_OUTPUT, "Channel 4");

		// Create all three players (only one will be used at a time)
		modPlayer = mod_player_create();
		medPlayer = med_player_create();
		ahxPlayer = ahx_player_create();

		// Safety check - ensure all players were created successfully
		if (!modPlayer || !medPlayer || !ahxPlayer) {
			INFO("ERROR: Failed to create one or more players!");
			if (modPlayer) { mod_player_destroy(modPlayer); modPlayer = nullptr; }
			if (medPlayer) { med_player_destroy(medPlayer); medPlayer = nullptr; }
			if (ahxPlayer) { ahx_player_destroy(ahxPlayer); ahxPlayer = nullptr; }
			return;
		}

		initialized = true;
	}

	~RM_Deck() {
		shouldStopLoading = true;
		if (loadingThread.joinable()) {
			loadingThread.join();
		}
		if (modPlayer) {
			mod_player_destroy(modPlayer);
		}
		if (medPlayer) {
			med_player_destroy(medPlayer);
		}
		if (ahxPlayer) {
			ahx_player_destroy(ahxPlayer);
		}
	}

	// Helper: detect file type from content (using player detection functions)
	PlayerType detectFileType(const std::vector<uint8_t>& fileData) {
		if (fileData.empty()) {
			return PlayerType::NONE;
		}

		// Try MOD detection first (most common)
		if (mod_player_detect(fileData.data(), fileData.size())) {
			return PlayerType::MOD;
		}

		// Try MMD detection
		if (med_player_detect(fileData.data(), fileData.size())) {
			return PlayerType::MED;
		}

		// Try AHX detection
		if (ahx_player_detect(fileData.data(), fileData.size())) {
			return PlayerType::AHX;
		}

		return PlayerType::NONE;
	}

	void loadFile(const std::string& path) {
		if (!initialized || loading) {
			return;
		}

		// Stop any existing loading thread
		shouldStopLoading = true;
		if (loadingThread.joinable()) {
			loadingThread.join();
		}
		shouldStopLoading = false;

		INFO("=== STARTING LOAD THREAD for: %s ===", path.c_str());
		loadingThread = std::thread([this, path]() {
			INFO("[LOAD] Thread started");
			{
				std::lock_guard<std::mutex> lock(swapMutex);
				fileLoaded = false;
				loading = true;
				playing = false;
				singlePatternLoop = false;  // Reset loop flag
				currentFileName = "Opening file...";  // Stage 1
				INFO("[LOAD] Stage 1: Opening file...");
				// Reset channel mute states
				for (int i = 0; i < 4; i++) {
					setChannelMuted(i, false);
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Visible delay

			// Read file into memory first
			INFO("[LOAD] About to fopen");
			FILE* f = fopen(path.c_str(), "rb");
			if (!f) {
				INFO("[LOAD] ERROR: fopen failed!");
				currentFileName = "ERROR: Cannot open";
				loading = false;
				return;
			}
			INFO("[LOAD] fopen successful");

			currentFileName = "Reading file...";  // Stage 2
			INFO("[LOAD] Stage 2: Reading file...");
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			fseek(f, 0, SEEK_END);
			long fileSize = ftell(f);
			fseek(f, 0, SEEK_SET);

			std::vector<uint8_t> fileData(fileSize);
			size_t bytesRead = fread(fileData.data(), 1, fileSize, f);
			fclose(f);

			if (bytesRead != (size_t)fileSize) {
				INFO("[LOAD] ERROR: fread failed! Expected %ld, got %zu", fileSize, bytesRead);
				currentFileName = "ERROR: Read failed";
				loading = false;
				return;
			}
			INFO("[LOAD] File read successful: %zu bytes", bytesRead);

			currentFileName = "Detecting format...";  // Stage 3
			INFO("[LOAD] Stage 3: Detecting format...");
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			// Detect file type from content
			PlayerType detectedType = detectFileType(fileData);
			if (detectedType == PlayerType::NONE) {
				INFO("File type detection failed");
				currentFileName = "ERROR: Unknown format";
				loading = false;
				return;
			}

			INFO("Detected file type: %d (1=MOD, 2=MED, 3=AHX)", (int)detectedType);

			// Load file using appropriate player
			bool success = false;
			{
				std::lock_guard<std::mutex> lock(swapMutex);
				if (!shouldStopLoading) {
					try {
						if (detectedType == PlayerType::MOD && modPlayer) {
							currentFileName = "Loading MOD...";  // Stage 4a
							INFO("Loading MOD file...");
							success = mod_player_load(modPlayer, fileData.data(), fileData.size());
							if (success) {
								currentFileName = "MOD: Set callback";  // Stage 5a
								playerType = PlayerType::MOD;
								mod_player_set_position_callback(modPlayer, playerPositionCallback, this);
								INFO("MOD load successful");
							} else {
								currentFileName = "ERROR: MOD load fail";
								INFO("MOD load failed");
							}
						} else if (detectedType == PlayerType::MED && medPlayer) {
							currentFileName = "Loading MED...";  // Stage 4b
							INFO("[LOAD] Stage 4b: Loading MED...");
							std::this_thread::sleep_for(std::chrono::milliseconds(500));
							INFO("[LOAD] About to call med_player_load");
							success = med_player_load(medPlayer, fileData.data(), fileData.size());
							INFO("[LOAD] med_player_load returned: %d", success);
							if (success) {
								currentFileName = "MED: Set callback";  // Stage 5b
								INFO("[LOAD] Stage 5b: MED loaded, setting callback...");
								std::this_thread::sleep_for(std::chrono::milliseconds(500));
								playerType = PlayerType::MED;
								INFO("[LOAD] About to call med_player_set_position_callback");
								med_player_set_position_callback(medPlayer, playerPositionCallback, this);
								INFO("[LOAD] Callback set successfully");
								currentFileName = "MED: Callback done";  // Stage 6b
								std::this_thread::sleep_for(std::chrono::milliseconds(500));
								INFO("[LOAD] MED load complete!");
							} else {
								currentFileName = "ERROR: MED load fail";
								INFO("[LOAD] ERROR: MED load failed!");
							}
						} else if (detectedType == PlayerType::AHX && ahxPlayer) {
							currentFileName = "Loading AHX...";  // Stage 4c
							INFO("Loading AHX file...");
							success = ahx_player_load(ahxPlayer, fileData.data(), fileData.size());
							if (success) {
								currentFileName = "AHX: Set callback";  // Stage 5c
								playerType = PlayerType::AHX;
								ahx_player_set_position_callback(ahxPlayer, ahxPositionCallback, this);
								INFO("AHX load successful");
							} else {
								currentFileName = "ERROR: AHX load fail";
								INFO("AHX load failed");
							}
						}
					} catch (...) {
						// Catch any exceptions during load to prevent crash
						currentFileName = "ERROR: Exception!";
						DEBUG("Exception caught during load!");
						success = false;
						playerType = PlayerType::NONE;
					}
				}
			}

			INFO("[LOAD] Load success: %d", success);

			if (success && !shouldStopLoading) {
				INFO("[LOAD] Entering finalization");
				std::lock_guard<std::mutex> lock(swapMutex);

				currentFileName = "Finalizing...";  // Stage 7
				INFO("[LOAD] Stage 7: Finalizing...");
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				// Extract filename
				size_t lastSlash = path.find_last_of("/\\");
				std::string actualFileName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

				currentFileName = "Resetting mutes...";  // Stage 8
				INFO("[LOAD] Stage 8: Resetting channel mutes...");
				std::this_thread::sleep_for(std::chrono::milliseconds(500));

				// Reset all channel mutes in the player
				INFO("[LOAD] About to reset %d channel mutes for playerType=%d", 4, (int)playerType);
				for (int i = 0; i < 4; i++) {
					if (playerType == PlayerType::MOD) {
						mod_player_set_channel_mute(modPlayer, i, false);
					} else if (playerType == PlayerType::MED) {
						INFO("[LOAD] Calling med_player_set_channel_mute for channel %d", i);
						med_player_set_channel_mute(medPlayer, i, false);
						INFO("[LOAD] Channel %d mute reset OK", i);
					} else if (playerType == PlayerType::AHX) {
						ahx_player_set_channel_mute(ahxPlayer, i, false);
					}
				}
				INFO("[LOAD] All channel mutes reset");

				currentFileName = actualFileName;  // Stage 9 - show actual filename
				INFO("[LOAD] Stage 9: Success! Filename: %s", actualFileName.c_str());
				fileLoaded = true;
				bufferValid = false;
			}

			INFO("[LOAD] Setting loading=false");
			loading = false;
			INFO("[LOAD] === LOAD THREAD COMPLETE ===");
		});
	}

	void renderBuffer() {
		// Lock mutex BEFORE checking state to prevent race condition
		std::lock_guard<std::mutex> lock(swapMutex);

		if (!fileLoaded || loading || playerType == PlayerType::NONE) {
			bufferValid = false;
			return;
		}

		// Additional safety check: ensure player instance is valid
		if ((playerType == PlayerType::MOD && !modPlayer) ||
		    (playerType == PlayerType::MED && !medPlayer) ||
		    (playerType == PlayerType::AHX && !ahxPlayer)) {
			bufferValid = false;
			return;
		}

		// Create array of channel output pointers
		float* channelOutputs[4] = {
			channel1Buffer,
			channel2Buffer,
			channel3Buffer,
			channel4Buffer
		};

		if (playerType == PlayerType::MOD && modPlayer) {
			mod_player_process_channels(modPlayer, leftBuffer, rightBuffer,
			                            channelOutputs, BUFFER_SIZE,
			                            APP->engine->getSampleRate());
		} else if (playerType == PlayerType::MED && medPlayer) {
			// For MED files with more than 4 channels, only request first 4 channels
			// This prevents buffer overflow when files have 8+ channels
			uint8_t num_channels = med_player_get_num_channels(medPlayer);
			uint8_t channels_to_output = (num_channels > 4) ? 4 : num_channels;
			med_player_process_channels(medPlayer, leftBuffer, rightBuffer,
			                            channelOutputs, channels_to_output, BUFFER_SIZE,
			                            APP->engine->getSampleRate());
		} else if (playerType == PlayerType::AHX && ahxPlayer) {
			ahx_player_process_channels(ahxPlayer, leftBuffer, rightBuffer,
			                            channelOutputs, BUFFER_SIZE,
			                            APP->engine->getSampleRate());
		} else {
			// Should not happen, but clear buffer as safety
			bufferValid = false;
			return;
		}

		bufferPos = 0;
		bufferValid = true;
	}

	void process(const ProcessArgs& args) override {
		// Handle play button
		if (params[PLAY_PARAM].getValue() > 0.5f) {
			if (fileLoaded) {
				playing = !playing;
				if (playing) {
					std::lock_guard<std::mutex> lock(swapMutex);
					if (playerType == PlayerType::MOD && modPlayer) {
						mod_player_start(modPlayer);
					} else if (playerType == PlayerType::MED && medPlayer) {
						med_player_start(medPlayer);
					} else if (playerType == PlayerType::AHX && ahxPlayer) {
						ahx_player_start(ahxPlayer);
					}
				} else {
					std::lock_guard<std::mutex> lock(swapMutex);
					if (playerType == PlayerType::MOD && modPlayer) {
						mod_player_stop(modPlayer);
					} else if (playerType == PlayerType::MED && medPlayer) {
						med_player_stop(medPlayer);
					} else if (playerType == PlayerType::AHX && ahxPlayer) {
						ahx_player_stop(ahxPlayer);
					}
				}
			}
			params[PLAY_PARAM].setValue(0.f);
		}

		// Handle set loop button (PAD1) - toggle between single pattern loop and full song
		if (params[PAD1_PARAM].getValue() > 0.5f) {
			if (fileLoaded && playerType != PlayerType::NONE) {
				std::lock_guard<std::mutex> lock(swapMutex);
				if (singlePatternLoop) {
					// Disable loop - restore full song playback
					uint8_t songLen = 0;
					if (playerType == PlayerType::MOD && modPlayer) {
						songLen = mod_player_get_song_length(modPlayer);
						mod_player_set_loop_range(modPlayer, 0, songLen > 0 ? songLen - 1 : 0);
					} else if (playerType == PlayerType::MED && medPlayer) {
						songLen = med_player_get_song_length(medPlayer);
						med_player_set_loop_range(medPlayer, 0, songLen > 0 ? songLen - 1 : 0);
					}
					singlePatternLoop = false;
				} else {
					// Enable single pattern loop
					uint8_t currentPattern;
					uint16_t currentRow;
					if (playerType == PlayerType::MOD && modPlayer) {
						mod_player_get_position(modPlayer, &currentPattern, &currentRow);
						mod_player_set_loop_range(modPlayer, currentPattern, currentPattern);
					} else if (playerType == PlayerType::MED && medPlayer) {
						med_player_get_position(medPlayer, &currentPattern, &currentRow);
						med_player_set_loop_range(medPlayer, currentPattern, currentPattern);
					}
					singlePatternLoop = true;
				}
			}
			params[PAD1_PARAM].setValue(0.f);
		}

		// Handle pattern - button (PAD3)
		if (params[PAD3_PARAM].getValue() > 0.5f) {
			if (fileLoaded && playerType != PlayerType::NONE) {
				std::lock_guard<std::mutex> lock(swapMutex);
				uint8_t currentPattern;
		uint16_t currentRow;
				if (playerType == PlayerType::MOD && modPlayer) {
					mod_player_get_position(modPlayer, &currentPattern, &currentRow);
					if (currentPattern > 0) {
						mod_player_set_position(modPlayer, currentPattern - 1, 0);
					}
				} else if (playerType == PlayerType::MED && medPlayer) {
					med_player_get_position(medPlayer, &currentPattern, &currentRow);
					if (currentPattern > 0) {
						med_player_set_position(medPlayer, currentPattern - 1, 0);
					}
				}
			}
			params[PAD3_PARAM].setValue(0.f);
		}

		// Handle pattern + button (PAD4)
		if (params[PAD4_PARAM].getValue() > 0.5f) {
			if (fileLoaded && playerType != PlayerType::NONE) {
				std::lock_guard<std::mutex> lock(swapMutex);
				uint8_t currentPattern;
		uint16_t currentRow;
				uint8_t songLen = 0;
				if (playerType == PlayerType::MOD && modPlayer) {
					mod_player_get_position(modPlayer, &currentPattern, &currentRow);
					songLen = mod_player_get_song_length(modPlayer);
					if (currentPattern < songLen - 1) {
						mod_player_set_position(modPlayer, currentPattern + 1, 0);
					}
				} else if (playerType == PlayerType::MED && medPlayer) {
					med_player_get_position(medPlayer, &currentPattern, &currentRow);
					songLen = med_player_get_song_length(medPlayer);
					if (currentPattern < songLen - 1) {
						med_player_set_position(medPlayer, currentPattern + 1, 0);
					}
				}
			}
			params[PAD4_PARAM].setValue(0.f);
		}

		// Handle mute button (PAD5)
		if (params[PAD5_PARAM].getValue() > 0.5f) {
			muted = !muted;
			params[PAD5_PARAM].setValue(0.f);
		}

		// Handle PFL button (PAD6)
		if (params[PAD6_PARAM].getValue() > 0.5f) {
			pflActive = !pflActive;
			params[PAD6_PARAM].setValue(0.f);
		}

		// Handle channel mute buttons
		for (int i = 0; i < 4; i++) {
			if (params[CHAN1_MUTE_PARAM + i].getValue() > 0.5f) {
				if (fileLoaded && playerType != PlayerType::NONE) {
					std::lock_guard<std::mutex> lock(swapMutex);
					bool newMuteState = !getChannelMuted(i);
					setChannelMuted(i, newMuteState);
					if (playerType == PlayerType::MOD && modPlayer) {
						mod_player_set_channel_mute(modPlayer, i, newMuteState);
					} else if (playerType == PlayerType::MED && medPlayer) {
						med_player_set_channel_mute(medPlayer, i, newMuteState);
					} else if (playerType == PlayerType::AHX && ahxPlayer) {
						ahx_player_set_channel_mute(ahxPlayer, i, newMuteState);
					}
				}
				params[CHAN1_MUTE_PARAM + i].setValue(0.f);
			}
		}

		// Playback
		if (loading || !playing || !fileLoaded) {
			outputs[AUDIO_L_OUTPUT].setVoltage(0.f);
			outputs[AUDIO_R_OUTPUT].setVoltage(0.f);
			outputs[PFL_L_OUTPUT].setVoltage(0.f);
			outputs[PFL_R_OUTPUT].setVoltage(0.f);
			return;
		}

		// Render buffer if needed
		if (!bufferValid || bufferPos >= BUFFER_SIZE) {
			renderBuffer();
		}

		if (!bufferValid) {
			outputs[AUDIO_L_OUTPUT].setVoltage(0.f);
			outputs[AUDIO_R_OUTPUT].setVoltage(0.f);
			outputs[PFL_L_OUTPUT].setVoltage(0.f);
			outputs[PFL_R_OUTPUT].setVoltage(0.f);
			return;
		}

		// Get samples from buffer
		float leftSample = leftBuffer[bufferPos];
		float rightSample = rightBuffer[bufferPos];
		float ch1Sample = channel1Buffer[bufferPos];
		float ch2Sample = channel2Buffer[bufferPos];
		float ch3Sample = channel3Buffer[bufferPos];
		float ch4Sample = channel4Buffer[bufferPos];
		bufferPos++;

		// Output audio with flexible routing
		float gain = muted ? 0.f : 5.f;

		// Detect which outputs are connected to determine routing mode
		bool ch3Connected = outputs[AUDIO_3_OUTPUT].isConnected();
		bool ch4Connected = outputs[AUDIO_4_OUTPUT].isConnected();
		bool multiChannelMode = ch3Connected || ch4Connected;

		if (multiChannelMode) {
			// Multi-channel mode: Output all 4 channels separately
			// L becomes 1, R becomes 2, plus 3, 4
			outputs[AUDIO_L_OUTPUT].setVoltage(ch1Sample * gain);
			outputs[AUDIO_R_OUTPUT].setVoltage(ch2Sample * gain);
			outputs[AUDIO_3_OUTPUT].setVoltage(ch3Sample * gain);
			outputs[AUDIO_4_OUTPUT].setVoltage(ch4Sample * gain);
		} else {
			// Stereo mode: Use pre-mixed L/R outputs
			// Amiga panning [L R R L]: Left = Ch0+Ch3, Right = Ch1+Ch2
			outputs[AUDIO_L_OUTPUT].setVoltage(leftSample * gain);
			outputs[AUDIO_R_OUTPUT].setVoltage(rightSample * gain);
			outputs[AUDIO_3_OUTPUT].setVoltage(0.f);
			outputs[AUDIO_4_OUTPUT].setVoltage(0.f);
		}

		// PFL output (Pre-Fader Listening)
		if (pflActive) {
			outputs[PFL_L_OUTPUT].setVoltage(leftSample * 5.f);
			outputs[PFL_R_OUTPUT].setVoltage(rightSample * 5.f);
		} else {
			outputs[PFL_L_OUTPUT].setVoltage(0.f);
			outputs[PFL_R_OUTPUT].setVoltage(0.f);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		if (fileLoaded && !currentFileName.empty()) {
			json_object_set_new(rootJ, "fileName", json_string(currentFileName.c_str()));
		}
		json_object_set_new(rootJ, "singlePatternLoop", json_boolean(singlePatternLoop));
		json_object_set_new(rootJ, "muted", json_boolean(muted));
		json_object_set_new(rootJ, "pflActive", json_boolean(pflActive));

		// Save channel mute states
		json_t* channelMutesJ = json_array();
		for (int i = 0; i < 4; i++) {
			json_array_append_new(channelMutesJ, json_boolean(getChannelMuted(i)));
		}
		json_object_set_new(rootJ, "channelMutes", channelMutesJ);

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* fileNameJ = json_object_get(rootJ, "fileName");
		if (fileNameJ) {
			currentFileName = json_string_value(fileNameJ);
		}

		json_t* loopJ = json_object_get(rootJ, "singlePatternLoop");
		if (loopJ) singlePatternLoop = json_boolean_value(loopJ);

		json_t* mutedJ = json_object_get(rootJ, "muted");
		if (mutedJ) muted = json_boolean_value(mutedJ);

		json_t* pflJ = json_object_get(rootJ, "pflActive");
		if (pflJ) pflActive = json_boolean_value(pflJ);

		// Restore channel mute states
		json_t* channelMutesJ = json_object_get(rootJ, "channelMutes");
		if (channelMutesJ && json_is_array(channelMutesJ)) {
			for (int i = 0; i < 4 && i < (int)json_array_size(channelMutesJ); i++) {
				json_t* muteJ = json_array_get(channelMutesJ, i);
				if (muteJ && json_is_boolean(muteJ)) {
					bool muted = json_boolean_value(muteJ);
					setChannelMuted(i, muted);
					// Apply to player when file loads
					if (fileLoaded) {
						if (playerType == PlayerType::MOD && modPlayer) {
							mod_player_set_channel_mute(modPlayer, i, muted);
						} else if (playerType == PlayerType::MED && medPlayer) {
							med_player_set_channel_mute(medPlayer, i, muted);
						} else if (playerType == PlayerType::AHX && ahxPlayer) {
							ahx_player_set_channel_mute(ahxPlayer, i, muted);
						}
					}
				}
			}
		}
	}
};

// Implement position callbacks after RM_Deck is fully defined
static void playerPositionCallback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data) {
	RM_Deck* module = static_cast<RM_Deck*>(user_data);
	if (module) {
		module->currentOrder = order;
		module->currentPattern = pattern;
		module->currentRow = row;
	}
}

static void ahxPositionCallback(uint8_t subsong, uint16_t position, uint16_t row, void* user_data) {
	RM_Deck* module = static_cast<RM_Deck*>(user_data);
	if (module) {
		module->currentOrder = subsong;     // Use subsong as order for display
		module->currentPattern = position;  // AHX uses position (uint16_t)
		module->currentRow = row;
	}
}

// Implement DeckPad::step() after RM_Deck is fully defined
void DeckPad::step() {
	if (module && padIndex == 0) {
		// LOOP pad - show YELLOW when single pattern loop is set
		if (module->singlePatternLoop) {
			setPadState(3);  // YELLOW = state 3
		} else {
			setPadState(0);  // GREY = state 0
		}
	} else if (module && padIndex == 1) {
		// PLAY pad - show GREEN when playing, RED when stopped with file loaded
		if (module->playing) {
			setPadState(2);  // GREEN = state 2
		} else if (module->fileLoaded) {
			setPadState(1);  // RED = state 1
		} else {
			setPadState(0);  // GREY = state 0
		}
	} else if (module && padIndex == 4) {
		// MUTE ALL pad - show RED when muted
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
	} else if (module && padIndex >= 6 && padIndex <= 9) {
		// Channel mute pads (CH1-CH4) - show RED when muted
		int chanIndex = padIndex - 6;
		if (module->getChannelMuted(chanIndex)) {
			setPadState(1);  // RED = state 1
		} else {
			setPadState(2);  // GREEN = state 2 (unmuted/active)
		}
	} else {
		setPadState(0);  // GREY for pattern +/- buttons
	}
	RegroovePad::step();
}

// Display widget showing song position info
struct ModInfoDisplay : TransparentWidget {
	RM_Deck* module;

	void draw(const DrawArgs& args) override {
		// Red border
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgStrokeColor(args.vg, REGROOVE_RED);
		nvgStrokeWidth(args.vg, 2);
		nvgStroke(args.vg);

		if (!module || !module->fileLoaded || module->loading) {
			nvgFontSize(args.vg, 10);
			nvgFontFaceId(args.vg, APP->window->uiFont->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, nvgRGB(0x55, 0x55, 0x55));

			// Read currentFileName with mutex protection to see loading progress
			std::string displayText;
			if (module) {
				std::lock_guard<std::mutex> lock(module->swapMutex);
				displayText = module->currentFileName;
			}
			const char* text = (!module || !module->fileLoaded) ? "Right-click to load file" : displayText.c_str();
			nvgText(args.vg, box.size.x / 2, box.size.y / 2, text, NULL);
			Widget::draw(args);
			return;
		}

		// Get position info from atomic variables
		uint8_t order = module->currentOrder;
		uint8_t pattern = module->currentPattern;
		uint16_t row = module->currentRow;

		// Display song position
		nvgFontSize(args.vg, 11);
		nvgFontFaceId(args.vg, APP->window->uiFont->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgFillColor(args.vg, REGROOVE_TEXT);

		char buf[64];
		snprintf(buf, sizeof(buf), "Order: %02d", order);
		nvgText(args.vg, 5, 5, buf, NULL);

		snprintf(buf, sizeof(buf), "Pattern: %02d", pattern);
		nvgText(args.vg, 5, 17, buf, NULL);

		snprintf(buf, sizeof(buf), "Row: %02d", row);
		nvgText(args.vg, 5, 29, buf, NULL);

		// Show filename
		nvgFontSize(args.vg, 9);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
		nvgText(args.vg, box.size.x / 2, box.size.y - 5, module->currentFileName.c_str(), NULL);

		Widget::draw(args);
	}
};

struct RM_DeckWidget : ModuleWidget {
	RM_DeckWidget(RM_Deck* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/RM_Deck.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Title
		RegrooveLabel* titleLabel = new RegrooveLabel();
		titleLabel->box.pos = mm2px(Vec(0, 6.5));
		titleLabel->box.size = mm2px(Vec(60.96, 5));
		titleLabel->text = "Tracker Deck";
		titleLabel->fontSize = 18.0;
		titleLabel->color = nvgRGB(0xff, 0xff, 0xff);
		titleLabel->bold = true;
		addChild(titleLabel);

		// Info display (shows song order, row, filename)
		ModInfoDisplay* infoDisplay = new ModInfoDisplay();
		infoDisplay->box.pos = mm2px(Vec(3, 16));
		infoDisplay->box.size = mm2px(Vec(54.96, 22));
		infoDisplay->module = module;
		addChild(infoDisplay);

		// Channel mute pads (4 small pads in a row below the display)
		float chanPadStartX = 6;
		float chanPadStartY = 40;
		float chanPadSpacing = 1.5;
		float chanPadSize = 11;

		for (int i = 0; i < 4; i++) {
			DeckPad* chanPad = new DeckPad();
			chanPad->box.pos = mm2px(Vec(chanPadStartX + i * (chanPadSize + chanPadSpacing), chanPadStartY));
			chanPad->box.size = mm2px(Vec(chanPadSize, chanPadSize));
			chanPad->module = module;
			chanPad->padIndex = 6 + i;
			chanPad->label = "CH" + std::to_string(i + 1);
			chanPad->app::ParamWidget::module = module;
			chanPad->app::ParamWidget::paramId = RM_Deck::CHAN1_MUTE_PARAM + i;
			chanPad->initParamQuantity();
			addParam(chanPad);
		}

		// Pad grid
		float padStartX = 5;
		float padStartY = 54;
		float padSpacing = 5;
		float padSize = 13;

		// Row 1
		DeckPad* pad1 = createParam<DeckPad>(mm2px(Vec(padStartX, padStartY)), module, RM_Deck::PAD1_PARAM);
		pad1->module = module;
		pad1->padIndex = 0;
		pad1->label = "LOOP";
		addParam(pad1);

		DeckPad* playPad = createParam<DeckPad>(mm2px(Vec(padStartX + padSize + padSpacing, padStartY)), module, RM_Deck::PLAY_PARAM);
		playPad->module = module;
		playPad->padIndex = 1;
		playPad->label = "PLAY";
		addParam(playPad);

		// Row 2
		DeckPad* pad3 = createParam<DeckPad>(mm2px(Vec(padStartX, padStartY + padSize + padSpacing)), module, RM_Deck::PAD3_PARAM);
		pad3->module = module;
		pad3->padIndex = 2;
		pad3->label = "PTN-";
		addParam(pad3);

		DeckPad* pad4 = createParam<DeckPad>(mm2px(Vec(padStartX + padSize + padSpacing, padStartY + padSize + padSpacing)), module, RM_Deck::PAD4_PARAM);
		pad4->module = module;
		pad4->padIndex = 3;
		pad4->label = "PTN+";
		addParam(pad4);

		// Row 3
		DeckPad* pad5 = createParam<DeckPad>(mm2px(Vec(padStartX, padStartY + (padSize + padSpacing) * 2)), module, RM_Deck::PAD5_PARAM);
		pad5->module = module;
		pad5->padIndex = 4;
		pad5->label = "MUTE";
		addParam(pad5);

		DeckPad* pad6 = createParam<DeckPad>(mm2px(Vec(padStartX + padSize + padSpacing, padStartY + (padSize + padSpacing) * 2)), module, RM_Deck::PAD6_PARAM);
		pad6->module = module;
		pad6->padIndex = 5;
		pad6->label = "PFL";
		addParam(pad6);

		// Tempo fader
		float faderWidth = 10;
		float faderHeight = 50;
		float faderCenterX = 52;
		float faderTopY = 56;
		float faderLeft = faderCenterX - (faderWidth / 2);

		auto* tempoFader = createParam<RegrooveSlider>(mm2px(Vec(faderLeft, faderTopY)), module, RM_Deck::TEMPO_PARAM);
		tempoFader->box.size = mm2px(Vec(faderWidth, faderHeight));
		addParam(tempoFader);

		RegrooveLabel* tempoLabel = new RegrooveLabel();
		tempoLabel->box.pos = mm2px(Vec(47, 53));
		tempoLabel->box.size = mm2px(Vec(10, 3));
		tempoLabel->text = "Tempo";
		tempoLabel->fontSize = 7.0;
		addChild(tempoLabel);

		// Outputs - 6 evenly spaced at bottom (same layout as DJ mixer)
		// Spacing: 9.4mm between each output
		float outY = 118.0;
		float labelY = 110.0;
		float labelSize = 3.0;

		// PFL L
		RegrooveLabel* pflLLabel = new RegrooveLabel();
		pflLLabel->box.pos = mm2px(Vec(7.5 - 2, labelY));
		pflLLabel->box.size = mm2px(Vec(4, labelSize));
		pflLLabel->text = "PFL";
		pflLLabel->fontSize = 6.0;
		pflLLabel->align = NVG_ALIGN_CENTER;
		addChild(pflLLabel);
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(7.5, outY)), module, RM_Deck::PFL_L_OUTPUT));

		// PFL R
		RegrooveLabel* pflRLabel = new RegrooveLabel();
		pflRLabel->box.pos = mm2px(Vec(16.9 - 2, labelY));
		pflRLabel->box.size = mm2px(Vec(4, labelSize));
		pflRLabel->text = "PFL";
		pflRLabel->fontSize = 6.0;
		pflRLabel->align = NVG_ALIGN_CENTER;
		addChild(pflRLabel);
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(16.9, outY)), module, RM_Deck::PFL_R_OUTPUT));

		// L/1
		RegrooveLabel* l1Label = new RegrooveLabel();
		l1Label->box.pos = mm2px(Vec(26.3 - 2, labelY));
		l1Label->box.size = mm2px(Vec(4, labelSize));
		l1Label->text = "L/1";
		l1Label->fontSize = 6.0;
		l1Label->align = NVG_ALIGN_CENTER;
		addChild(l1Label);
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(26.3, outY)), module, RM_Deck::AUDIO_L_OUTPUT));

		// R/2
		RegrooveLabel* r2Label = new RegrooveLabel();
		r2Label->box.pos = mm2px(Vec(35.7 - 2, labelY));
		r2Label->box.size = mm2px(Vec(4, labelSize));
		r2Label->text = "R/2";
		r2Label->fontSize = 6.0;
		r2Label->align = NVG_ALIGN_CENTER;
		addChild(r2Label);
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(35.7, outY)), module, RM_Deck::AUDIO_R_OUTPUT));

		// 3
		RegrooveLabel* ch3Label = new RegrooveLabel();
		ch3Label->box.pos = mm2px(Vec(45.1 - 1.5, labelY));
		ch3Label->box.size = mm2px(Vec(3, labelSize));
		ch3Label->text = "3";
		ch3Label->fontSize = 6.0;
		ch3Label->align = NVG_ALIGN_CENTER;
		addChild(ch3Label);
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(45.1, outY)), module, RM_Deck::AUDIO_3_OUTPUT));

		// 4
		RegrooveLabel* ch4Label = new RegrooveLabel();
		ch4Label->box.pos = mm2px(Vec(54.5 - 1.5, labelY));
		ch4Label->box.size = mm2px(Vec(3, labelSize));
		ch4Label->text = "4";
		ch4Label->fontSize = 6.0;
		ch4Label->align = NVG_ALIGN_CENTER;
		addChild(ch4Label);
		addOutput(createOutputCentered<RegroovePort>(mm2px(Vec(54.5, outY)), module, RM_Deck::AUDIO_4_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		RM_Deck* module = dynamic_cast<RM_Deck*>(this->module);
		if (!module) return;

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Module File"));

		menu->addChild(createMenuItem("Load MOD/MED/AHX file", "", [=]() {
			if (!module || !module->initialized || module->loading) {
				return;
			}
			char* path = osdialog_file(OSDIALOG_OPEN, NULL, NULL,
				osdialog_filters_parse("Module Files:mod,med,mmd,mmd0,mmd1,mmd2,mmd3,ahx,hvl"));
			if (path) {
				module->loadFile(path);
				free(path);
			}
		}));
	}
};

Model* modelRM_Deck = createModel<RM_Deck, RM_DeckWidget>("RM_Deck");
