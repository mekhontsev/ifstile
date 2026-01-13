echo off

set this=%cd%
cd ../external
rmdir _android /s /q
mkdir _android 
cd _android

cmake -G "Unix Makefiles"^
  -DCMAKE_BUILD_TYPE=Release ^
  -DANDROID_NDK=%ANDROID_NDK_HOME%^
  -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake^
  -DCMAKE_MAKE_PROGRAM=%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe^
  -DANDROID_ABI="arm64-v8a"^
  -DANDROID_PLATFORM=android-24^
  %this%

pause