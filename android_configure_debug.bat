cmake -S . -B build/android ^
  -G "Unix Makefiles"^
  -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
  -DCMAKE_MAKE_PROGRAM=%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe^
  -DANDROID_NDK=%ANDROID_NDK_HOME%^
  -DANDROID_ABI=arm64-v8a ^
  -DANDROID_PLATFORM=android-24 ^
  -DCMAKE_BUILD_TYPE=Debug
