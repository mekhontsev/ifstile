rmdir _android /s /q

echo off
mkdir _android & pushd _android
cmake -G "Unix Makefiles"^
  -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake^
  -DCMAKE_MAKE_PROGRAM=%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe^
  -DANDROID_NDK=%ANDROID_NDK_HOME%^
  -DCMAKE_BUILD_TYPE=Debug^
  -DANDROID_ABI="arm64-v8a"^
  -DANDROID_PLATFORM=android-24^
  ..
popd
