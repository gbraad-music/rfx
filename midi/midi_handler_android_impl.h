/**
 * Android MIDI Handler Implementation (Header-Only)
 * Works on Android 6.0+ (API 23+)
 *
 * USAGE: Define ANDROID_APP_PACKAGE before including this file:
 *   #define ANDROID_APP_PACKAGE nl_gbraad_junglizer
 *   #include "midi_handler_android_impl.h"
 *
 * This generates JNI function names specific to your app package.
 */

#ifndef ANDROID_APP_PACKAGE
#error "ANDROID_APP_PACKAGE must be defined before including midi_handler_android_impl.h"
#error "Example: #define ANDROID_APP_PACKAGE nl_gbraad_junglizer"
#endif

#include "midi_handler.h"

#ifdef __ANDROID__

// Default MIDI handler Java class path (can be overridden at compile time)
#ifndef MIDI_HANDLER_CLASS_PATH
#define MIDI_HANDLER_CLASS_PATH "nl/gbraad/regroove/MidiHandler"
#endif

// Macro magic to concatenate package name with function suffix
#define JNI_FUNC_NAME_HELPER(pkg, func) Java_ ## pkg ## _MidiHandler_ ## func
#define JNI_FUNC_NAME(pkg, func) JNI_FUNC_NAME_HELPER(pkg, func)

#include <android/log.h>
#include <jni.h>
#include <vector>
#include <string>

// Allow log tag to be overridden at compile time (no default - must be defined by app)
#ifndef MIDI_LOG_TAG
#define MIDI_LOG_TAG "RFX-MIDI"
#endif

#define LOG_TAG MIDI_LOG_TAG
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct MidiDeviceInfo {
    int index;
    int id;
    std::string name;
};

static struct {
    bool initialized;
    midi_callback_t callback;
    void* callback_user_data;
    std::vector<MidiDeviceInfo> devices;
    int current_device_id;
} g_midi_state = {
    .initialized = false,
    .callback = nullptr,
    .callback_user_data = nullptr,
    .current_device_id = -1
};

bool midi_handler_init(void) {
    if (g_midi_state.initialized) {
        LOGI("MIDI handler already initialized");
        return true;
    }

    LOGI("Initializing MIDI handler (Java MidiManager)");
    g_midi_state.initialized = true;
    g_midi_state.current_device_id = -1;

    LOGI("MIDI handler initialized - waiting for Java to enumerate devices");
    return true;
}

void midi_handler_cleanup(void) {
    if (!g_midi_state.initialized) {
        return;
    }

    LOGI("Cleaning up MIDI handler");
    g_midi_state.devices.clear();
    g_midi_state.initialized = false;
}

void midi_handler_set_callback(midi_callback_t callback, void* user_data) {
    g_midi_state.callback = callback;
    g_midi_state.callback_user_data = user_data;
    LOGI("MIDI callback set: %p", (void*)callback);
}

// Forward declaration for SDL JNI functions (SDL3)
extern "C" void* SDL_GetAndroidJNIEnv();

bool midi_handler_open_device(int device_index) {
    LOGI("Opening MIDI device at index: %d (total devices: %d)", device_index, (int)g_midi_state.devices.size());

    // Validate device index
    if (device_index < 0 || device_index >= (int)g_midi_state.devices.size()) {
        LOGE("Invalid device index %d", device_index);
        return false;
    }

    // Get JNI environment from SDL
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env) {
        LOGE("Failed to get JNI environment from SDL");
        return false;
    }

    jclass midiHandlerClass = env->FindClass(MIDI_HANDLER_CLASS_PATH);
    if (!midiHandlerClass) {
        LOGE("Failed to find MidiHandler class: %s", MIDI_HANDLER_CLASS_PATH);
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return false;
    }

    jmethodID openMethod = env->GetStaticMethodID(midiHandlerClass, "openDeviceByIndex", "(I)V");
    if (!openMethod) {
        LOGE("Failed to find openDeviceByIndex method");
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        env->DeleteLocalRef(midiHandlerClass);
        return false;
    }

    LOGI("Calling openDeviceByIndex(%d)...", device_index);
    env->CallStaticVoidMethod(midiHandlerClass, openMethod, device_index);

    // Check for Java exceptions
    if (env->ExceptionCheck()) {
        LOGE("Exception in openDeviceByIndex(%d)", device_index);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(midiHandlerClass);
        return false;
    }

    env->DeleteLocalRef(midiHandlerClass);

    g_midi_state.current_device_id = device_index;
    LOGI("Successfully opened MIDI device %d", device_index);
    return true;
}

void midi_handler_close_device(void) {
    LOGI("Closing current MIDI device");

    // Get JNI environment from SDL
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    if (!env) {
        LOGE("Failed to get JNI environment from SDL");
        return;
    }

    jclass midiHandlerClass = env->FindClass(MIDI_HANDLER_CLASS_PATH);
    if (!midiHandlerClass) {
        LOGE("Failed to find MidiHandler class");
        return;
    }

    jmethodID closeMethod = env->GetStaticMethodID(midiHandlerClass, "closeCurrentDevice", "()V");
    if (!closeMethod) {
        LOGE("Failed to find closeCurrentDevice method");
        env->DeleteLocalRef(midiHandlerClass);
        return;
    }

    env->CallStaticVoidMethod(midiHandlerClass, closeMethod);
    env->DeleteLocalRef(midiHandlerClass);

    LOGI("Called Java closeCurrentDevice()");
}

int midi_handler_get_device_count(void) {
    return (int)g_midi_state.devices.size();
}

const char* midi_handler_get_device_name(int device_index) {
    if (device_index < 0 || device_index >= (int)g_midi_state.devices.size()) {
        return nullptr;
    }
    // NOTE: This returns a pointer to std::string internals which can become
    // invalid if the vector is modified. Callers should copy immediately.
    return g_midi_state.devices[device_index].name.c_str();
}

bool midi_handler_send_message(const midi_message_t* message) {
    // MIDI output not implemented yet (Java side would need to handle this)
    (void)message;
    return false;
}

// JNI callbacks called from Java
// Function names are generated using ANDROID_APP_PACKAGE macro

extern "C" __attribute__((visibility("default"))) JNIEXPORT void JNICALL
JNI_FUNC_NAME(ANDROID_APP_PACKAGE, nativeMidiClearDevices)(
    JNIEnv* env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    int old_count = (int)g_midi_state.devices.size();
    g_midi_state.devices.clear();
    LOGI("[JNI] nativeMidiClearDevices called - cleared %d devices", old_count);
}

extern "C" __attribute__((visibility("default"))) JNIEXPORT void JNICALL
JNI_FUNC_NAME(ANDROID_APP_PACKAGE, nativeMidiAddDevice)(
    JNIEnv* env,
    jobject thiz,
    jint index,
    jint id,
    jstring name) {

    (void)thiz;

    const char* name_str = env->GetStringUTFChars(name, nullptr);

    MidiDeviceInfo info;
    info.index = (int)index;
    info.id = (int)id;
    info.name = name_str;

    g_midi_state.devices.push_back(info);

    LOGI("Added MIDI device %d: %s (id=%d)", index, name_str, id);

    env->ReleaseStringUTFChars(name, name_str);
}

extern "C" __attribute__((visibility("default"))) JNIEXPORT void JNICALL
JNI_FUNC_NAME(ANDROID_APP_PACKAGE, nativeMidiDeviceOpened)(
    JNIEnv* env,
    jobject thiz,
    jint id) {

    (void)env;
    (void)thiz;

    g_midi_state.current_device_id = (int)id;
    LOGI("MIDI device opened: id=%d", id);
}

extern "C" __attribute__((visibility("default"))) JNIEXPORT void JNICALL
JNI_FUNC_NAME(ANDROID_APP_PACKAGE, nativeMidiMessage)(
    JNIEnv* env,
    jobject thiz,
    jint status,
    jint data1,
    jint data2,
    jint timestamp) {

    (void)env;
    (void)thiz;

    if (!g_midi_state.callback) {
        return;
    }

    midi_message_t message = {};
    message.status = (uint8_t)status;
    message.data1 = (uint8_t)data1;
    message.data2 = (uint8_t)data2;
    message.timestamp = (uint32_t)timestamp;

    // Call the callback
    g_midi_state.callback(&message, g_midi_state.callback_user_data);
}

#endif // __ANDROID__
