/**
 * Gainput - C++ input library for games.
 *
 * Copyright (c) 2013-2017 Johannes Kuhlmann.
 * Licensed under the MIT license. See LICENSE file.
 */

#ifndef GAINPUT_H_
#define GAINPUT_H_

#if defined(__CYGWIN__) || defined(__MINGW32__)
#   if !defined(_WIN32)
#      define _WIN32
#   endif
#endif

#if defined(__ANDROID__) || defined(ANDROID)
	#define GAINPUT_PLATFORM_ANDROID
	#define GAINPUT_LIBEXPORT
#elif defined(__linux) || defined(__linux__) || defined(linux) || defined(LINUX)
	#define GAINPUT_PLATFORM_LINUX
	#define GAINPUT_LIBEXPORT
#elif defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
	#define GAINPUT_PLATFORM_WIN
	#if defined(GAINPUT_LIB_DYNAMIC)
		#define GAINPUT_LIBEXPORT		__declspec(dllexport)
	#elif defined(GAINPUT_LIB_DYNAMIC_USE)
		#define GAINPUT_LIBEXPORT		__declspec(dllimport)
	#else
		#define GAINPUT_LIBEXPORT
	#endif
#elif defined(__APPLE__)
	#define GAINPUT_LIBEXPORT
	#include <TargetConditionals.h>
    #if TARGET_OS_TV
        #define GAINPUT_PLATFORM_TVOS
	#elif TARGET_OS_IPHONE
		#define GAINPUT_PLATFORM_IOS
	#elif TARGET_OS_MAC
		#define GAINPUT_PLATFORM_MAC
	#else
		#error Gainput: Unknown/unsupported Apple platform!
	#endif
#elif defined(__EMSCRIPTEN__)
    #define GAINPUT_PLATFORM_EMSCRIPTEN
    #define GAINPUT_PLATFORM_LINUX
    #define GAINPUT_LIBEXPORT
#else
	#error Gainput: Unknown/unsupported platform!
#endif


//#define GAINPUT_DEBUG
//#define GAINPUT_DEV
#define GAINPUT_ENABLE_ALL_GESTURES
#define GAINPUT_ENABLE_RECORDER
#define GAINPUT_TEXT_INPUT_QUEUE_LENGTH 32

#ifdef GAINPUT_ENABLE_CONCURRENCY
#define MOODYCAMEL_EXCEPTIONS_DISABLED
#include "concurrentqueue.h"
#define GAINPUT_CONC_QUEUE(TYPE)            moodycamel::ConcurrentQueue<TYPE>
#define GAINPUT_CONC_CONSTRUCT(queue)       queue()
#define GAINPUT_CONC_ENQUEUE(queue, obj)    queue.enqueue(obj)
#define GAINPUT_CONC_DEQUEUE(queue, obj)    queue.try_dequeue(obj)
#else
#define GAINPUT_CONC_QUEUE(TYPE)            gainput::Array<TYPE>
#define GAINPUT_CONC_CONSTRUCT(queue)       queue(allocator)
#define GAINPUT_CONC_ENQUEUE(queue, obj)    queue.push_back(obj)
#define GAINPUT_CONC_DEQUEUE(queue, obj)    (!queue.empty() ? (obj = queue[queue.size()-1], queue.pop_back(), true) : false)
#endif

#include <cassert>
#include <cstring>
#include <new>

#define GAINPUT_ASSERT assert
#define GAINPUT_UNUSED(x) (void)(x)

#if defined(GAINPUT_PLATFORM_LINUX)

#include <cstdlib>
#include <stdint.h>

union _XEvent;
typedef _XEvent XEvent;

#elif defined(GAINPUT_PLATFORM_WIN)

#include <cstdlib>

typedef struct tagMSG MSG;

#if defined(__CYGWIN__) || defined(__MINGW32__)
namespace gainput
{
	typedef unsigned char uint8_t;
	typedef char int8_t;
	typedef unsigned int uint32_t;
	typedef unsigned long long uint64_t;
}
#else
namespace gainput
{
	typedef unsigned __int8 uint8_t;
	typedef __int8 int8_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;
}
#endif

#elif defined(GAINPUT_PLATFORM_ANDROID)

#include <stdint.h>
#include <stdlib.h>
struct AInputEvent;

#endif



/// Contains all Gainput related classes, types, and functions.
namespace gainput
{

/// ID of an input device.
typedef unsigned int DeviceId;
/// ID of a specific button unique to an input device.
typedef unsigned int DeviceButtonId;

/// Describes a device button on a specific device.
struct DeviceButtonSpec
{
	/// ID of the input device.
	DeviceId deviceId;
	/// ID of the button on the given input device.
	DeviceButtonId buttonId;
};

/// ID of a user-defined, mapped button.
typedef unsigned int UserButtonId;
/// ID of an input listener.
typedef unsigned int ListenerId;
/// ID of a device state modifier.
typedef unsigned int ModifierId;

/// An invalid device ID.
static const DeviceId InvalidDeviceId = -1;
/// An invalid device button ID.
static const DeviceButtonId InvalidDeviceButtonId = -1;
/// An invalid user button ID.
static const UserButtonId InvalidUserButtonId = -1;

/// Returns the name of the library, should be "Gainput".
const char* GetLibName();
/// Returns the version number of the library.
uint32_t GetLibVersion();
/// Returns the version number of the library as a printable string.
const char* GetLibVersionString();

class InputDeltaState;
class InputListener;
class InputManager;
class DebugRenderer;
class DeviceStateModifier;

template <class T> T Abs(T a) { return a < T() ? -a : a; }

/// Switches the library's internal development server to HTTP mode.
/**
 * When the server is in HTTP mode, it is possible to control touch
 * input using an external HTML page that connects to the library
 * via HTTP.
 *
 * The HTML page(s) can be found under `tools/html5client/` and should
 * be placed on an HTTP server that can be reached from the touch device
 * that should send touch events to the library. The touch device then
 * in turn connects to the library's internal HTTP server and periodically
 * sends touch input information.
 *
 * The pages can also be found hosted here:
 * http://gainput.johanneskuhlmann.de/html5client/
 */
void DevSetHttp(bool enable);
}

#define GAINPUT_VER_MAJOR_SHIFT		16
#define GAINPUT_VER_GET_MAJOR(ver)	(ver >> GAINPUT_VER_MAJOR_SHIFT)
#define GAINPUT_VER_GET_MINOR(ver)	(ver & (uint32_t(-1) >> GAINPUT_VER_MAJOR_SHIFT))


#include <gainput/GainputAllocator.h>
#include <gainput/GainputContainers.h>
#include <gainput/GainputInputState.h>
#include <gainput/GainputInputDevice.h>
#include <gainput/GainputInputListener.h>
#include <gainput/GainputInputManager.h>
#include <gainput/GainputInputMap.h>

#include <gainput/GainputInputDeviceMouse.h>
#include <gainput/GainputInputDeviceKeyboard.h>
#include <gainput/GainputInputDevicePad.h>
#include <gainput/GainputInputDeviceTouch.h>
#include <gainput/GainputInputDeviceBuiltIn.h>

#include <gainput/gestures/GainputGestures.h>

#include <gainput/recorder/GainputInputRecording.h>
#include <gainput/recorder/GainputInputPlayer.h>
#include <gainput/recorder/GainputInputRecorder.h>

#endif

