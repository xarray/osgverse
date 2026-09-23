#!/bin/sh
# Start an Android emulator (AVD), install the generated APK and launch it.
# The AVD and its system image are created automatically on first use.
#
# Usage: ./run_emulator.sh [options]
#   --apk <file>       APK to install (default: app/build/outputs/apk/debug/app-debug.apk)
#   --avd <name>       AVD name (default: osgverse_test)
#   --image <package>  System image (default: system-images;android-33;google_apis;x86_64)
#   --activity <c>     Activity to launch (default: com.osgverse.demoapp/.MainActivity)
#   --window           Show the emulator window (default: running without a window)
#   --gpu <mode>       Emulator GPU mode (default: swiftshader_indirect)
#   --logcat           Follow the application log after launching
#   --screenshot <f>   Save a screenshot after launching
#   --kill             Stop the running emulator and exit
#   -h, --help         Show this help
#
# Notes:
#   * The SDK is looked up from $ANDROID_SDK, then from android/local.properties.
#   * On Linux the current user has to be able to access /dev/kvm, otherwise the
#     emulator falls back to software emulation and is very slow. If hardware
#     acceleration is unavailable, use: sudo usermod -aG kvm $USER (then re-login).
#   * The APK and the emulator must share the CPU architecture. An emulator can
#     only boot a system image matching the host architecture (x86_64 on a PC),
#     so an arm64-v8a APK has to be tested on a real device. Set ABI_FILTER in
#     android/build.gradle to the emulator ABI to build an APK that runs here.

AndroidAvdName="osgverse_test"
AndroidSystemImage="system-images;android-33;google_apis;x86_64"
AndroidGpuMode="swiftshader_indirect"
AndroidActivityName="com.osgverse.demoapp/.MainActivity"
AndroidShowWindow=0
AndroidFollowLogcat=0
AndroidScreenshotFile=""
AndroidApkFile=""
DoKillOnly=0

while [ $# -gt 0 ]; do
    case "$1" in
        --apk)        AndroidApkFile=$2; shift 2;;
        --avd)        AndroidAvdName=$2; shift 2;;
        --image)      AndroidSystemImage=$2; shift 2;;
        --activity)   AndroidActivityName=$2; shift 2;;
        --window)     AndroidShowWindow=1; shift;;
        --gpu)        AndroidGpuMode=$2; shift 2;;
        --logcat)     AndroidFollowLogcat=1; shift;;
        --screenshot) AndroidScreenshotFile=$2; shift 2;;
        --kill)       DoKillOnly=1; shift;;
        -h|--help)    sed -n '2,/^$/p' "$0"; exit 0;;
        *)            echo "Unknown option: $1"; exit 1;;
    esac
done

case "$0" in
    */*) ScriptDir=$(cd "$(dirname "$0")" && pwd);;
    *)   ScriptDir=$(pwd);;
esac
CurrentDir=$(cd "$ScriptDir/.." && pwd)
AndroidDir=$CurrentDir/android

# Locate the Android SDK
if [ "$ANDROID_SDK" = "" ] && [ -f "$AndroidDir/local.properties" ]; then
    ANDROID_SDK=$(sed -n 's/^sdk\.dir=//p' "$AndroidDir/local.properties" | head -n 1)
fi
if [ "$ANDROID_SDK" = "" ] && [ -d "$HOME/Android/Sdk" ]; then
    ANDROID_SDK=$HOME/Android/Sdk
fi
if [ "$ANDROID_SDK" = "" ] || [ ! -d "$ANDROID_SDK" ]; then
    echo "Android SDK not found. Please set ANDROID_SDK or sdk.dir in android/local.properties."
    exit 1
fi

AdbExe=$ANDROID_SDK/platform-tools/adb
EmulatorExe=$ANDROID_SDK/emulator/emulator
CmdToolsDir=$ANDROID_SDK/cmdline-tools/latest
if [ ! -x "$CmdToolsDir/bin/sdkmanager" ]; then
    CmdToolsDir=""
    for CmdToolsCandidate in "$ANDROID_SDK"/cmdline-tools/*; do
        if [ -x "$CmdToolsCandidate/bin/sdkmanager" ]; then
            CmdToolsDir=$CmdToolsCandidate
        fi
    done
fi

if [ ! -x "$AdbExe" ]; then
    echo "adb not found in $ANDROID_SDK/platform-tools. Please install platform-tools."
    exit 1
fi
if [ ! -x "$EmulatorExe" ]; then
    echo "emulator not found in $ANDROID_SDK/emulator. Please install it with sdkmanager."
    exit 1
fi

# Stop all running emulators and exit
if [ "$DoKillOnly" = 1 ]; then
    for EmulatorDevice in $("$AdbExe" devices | sed -n 's/^\(emulator-[0-9]*\)[[:space:]]*device.*/\1/p'); do
        echo "Stopping $EmulatorDevice..."
        "$AdbExe" -s "$EmulatorDevice" emu kill
    done
    exit 0
fi

# Pick the APK to install
if [ "$AndroidApkFile" = "" ]; then
    AndroidApkFile=$AndroidDir/app/build/outputs/apk/debug/app-debug.apk
    if [ ! -f "$AndroidApkFile" ]; then
        AndroidApkFile=$AndroidDir/app/build/outputs/apk/release/app-release-unsigned.apk
    fi
fi
if [ ! -f "$AndroidApkFile" ]; then
    echo "APK file not found: $AndroidApkFile"
    echo "Please build it first, for example: ./Setup.sh ANDROID"
    exit 1
fi

# Reuse the emulator which is already running, if any
AndroidDevice=$("$AdbExe" devices | sed -n 's/^\(emulator-[0-9]*\)[[:space:]]*device.*/\1/p' | head -n 1)

if [ "$AndroidDevice" = "" ]; then
    # Create the AVD (and download the system image) when it does not exist yet
    if ! "$EmulatorExe" -list-avds 2>/dev/null | grep -qx "$AndroidAvdName"; then
        if [ "$CmdToolsDir" = "" ]; then
            echo "cmdline-tools not found in $ANDROID_SDK. Unable to create AVD '$AndroidAvdName'."
            exit 1
        fi
        if [ ! -d "$ANDROID_SDK/$(echo "$AndroidSystemImage" | tr ';' '/')" ]; then
            echo "Installing system image '$AndroidSystemImage' (this may take a while)..."
            yes | "$CmdToolsDir/bin/sdkmanager" --install "$AndroidSystemImage" || exit 1
        fi
        echo "Creating AVD '$AndroidAvdName'..."
        echo no | "$CmdToolsDir/bin/avdmanager" create avd -n "$AndroidAvdName" \
            -k "$AndroidSystemImage" --force || exit 1
    fi

    # Launch the emulator in the background
    AndroidEmulatorOptions="-avd $AndroidAvdName -no-audio -no-boot-anim -gpu $AndroidGpuMode"
    if [ "$AndroidShowWindow" = 0 ]; then
        AndroidEmulatorOptions="$AndroidEmulatorOptions -no-window"
    fi
    echo "Starting emulator '$AndroidAvdName' (log: android/emulator.log)..."
    cd "$AndroidDir"
    nohup "$EmulatorExe" $AndroidEmulatorOptions > "$AndroidDir/emulator.log" 2>&1 &
    cd "$CurrentDir"

    echo "Waiting for the emulator to boot (usually 1-3 minutes)..."
    # The serial of the new emulator is not known yet and adb fails as soon as
    # another device (a phone, for example) is connected too, so the emulator is
    # looked up on every round until it is booted
    AndroidWaitTime=0
    while :; do
        if [ "$AndroidDevice" = "" ]; then
            AndroidDevice=$("$AdbExe" devices \
                | sed -n 's/^\(emulator-[0-9]*\)[[:space:]]*device.*/\1/p' | head -n 1)
        fi
        if [ "$AndroidDevice" != "" ] && [ "$("$AdbExe" -s "$AndroidDevice" \
                shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; then
            break
        fi
        if [ $AndroidWaitTime -ge 600 ]; then
            echo "Timeout while waiting for the emulator. Please check android/emulator.log."
            exit 1
        fi
        sleep 5
        AndroidWaitTime=$((AndroidWaitTime + 5))
    done
fi

# An emulator can only run native libraries matching its own architecture
AndroidDeviceAbi=$("$AdbExe" -s "$AndroidDevice" shell getprop ro.product.cpu.abi 2>/dev/null | tr -d '\r')
AndroidApkAbis=""
if command -v unzip >/dev/null 2>&1; then
    AndroidApkAbis=$(unzip -Z1 "$AndroidApkFile" 2>/dev/null \
        | sed -n 's#^lib/\([^/]*\)/.*#\1#p' | sort -u | tr '\n' ' ')
fi
case " $AndroidApkAbis " in
    *" $AndroidDeviceAbi "*|"  ")
        ;;
    *)
        echo "The APK has no native libraries for '$AndroidDeviceAbi' (found: $AndroidApkAbis)."
        echo "An emulator only boots a system image matching the host architecture, so its ABI"
        echo "cannot be changed. Either set ABI_FILTER = '$AndroidDeviceAbi' in android/build.gradle"
        echo "and build again with ./Setup.sh ANDROID, or install the APK on a real device with:"
        echo "  $AdbExe install -r $AndroidApkFile"
        exit 1
        ;;
esac

echo "Installing $(basename "$AndroidApkFile") on $AndroidDevice..."
"$AdbExe" -s "$AndroidDevice" install -r -d "$AndroidApkFile" || exit 1

"$AdbExe" -s "$AndroidDevice" logcat -c
echo "Launching $AndroidActivityName..."
"$AdbExe" -s "$AndroidDevice" shell am start -n "$AndroidActivityName" || exit 1

if [ "$AndroidScreenshotFile" != "" ]; then
    sleep 5
    "$AdbExe" -s "$AndroidDevice" exec-out screencap -p > "$AndroidScreenshotFile"
    echo "Screenshot saved to $AndroidScreenshotFile"
fi

echo ""
echo "The APK is running on $AndroidDevice. Useful commands:"
echo "  adb -s $AndroidDevice logcat | grep -iE 'SDL|osgVerse|DEBUG'"
echo "  adb -s $AndroidDevice shell screencap -p /sdcard/shot.png && adb -s $AndroidDevice pull /sdcard/shot.png"
echo "  $0 --kill   # stop the emulator"

if [ "$AndroidFollowLogcat" = 1 ]; then
    AndroidAppPid=$("$AdbExe" -s "$AndroidDevice" shell pidof -s com.osgverse.demoapp | tr -d '\r')
    if [ "$AndroidAppPid" != "" ]; then
        "$AdbExe" -s "$AndroidDevice" logcat --pid="$AndroidAppPid"
    else
        "$AdbExe" -s "$AndroidDevice" logcat
    fi
fi
