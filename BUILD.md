# Building IFStile

IFStile uses the [CMake](http://cmake.org/) build system (v>=4.2) and [Git](https://git-scm.com/) for bootstrapping.

`git clone https://github.com/mekhontsev/ifstile.git`

## Windows
**Pre-requisites:**

- [Microsoft Visual Studio 2026](https://visualstudio.microsoft.com/)  (C/C++ Compiler, v145 toolset for x64, arm64).
- [Inno Setup](https://jrsoftware.org/isdl.php) - If you want to create an installer.

**Run:**
1) `bootstrap.bat` to download 3rd party dependencies.
2) `windows_rebuild.bat` to build IFStile.
3) `Setup/create_windows.bat` to create Setup Files.


## Linux (Ubuntu as example)
**Pre-requisites:**

- GCC v>=13
- CMake:`sudo apt install cmake`.

**Run:**
1) `sudo apt install libx11-dev xorg-dev libwayland-dev libgl1-mesa-dev libgl1-mesa-dev`
2) `bootstrap.sh` to download 3rd party dependencies.
3) `linux_rebuild.sh` to build IFStile.
4) `Setup/create_linux.sh` to create binary tar.gz


## macOS
**Pre-requisites:**
- [Xcode](https://developer.apple.com/xcode/) - Install and run at least once, then close.
- [brew](https://brew.sh/)
- `sudo xcode-select --reset`
- `brew install cmake`
- `brew install create-dmg`  - If you want to create an installer.

For HighDPI: add the following lines to the file `/opt/homebrew/Cellar/cmake/4.x.y/share/cmake/Modules/MacOSXBundleInfo.plist.in`
```
<key>NSPrincipalClass</key>
<string>NSApplication</string>
<key>NSHighResolutionCapable</key>
<string>True</string>
```
**Run:**
1) `bootstrap.sh` to download 3rd party dependencies.
2) `macos_rebuild.sh` to build IFStile.
3) `Setup/create_macos.sh` to create Setup (dmg file).

## Android (cross-compiling on Windows)
**Pre-requisites:**
- [Android Studio](https://developer.android.com/studio)
- Install SDK and NDK using Android Studio SDK Manager.
- Setup environment variables:
```
ANDROID_HOME=C:/Users/***/AppData/Local/Android/Sdk
ANDROID_NDK_HOME=C:/Users/***/AppData/Local/Android/Sdk/ndk/*.*.*
```

**Run:**
1) `bootstrap.bat` to download 3rd party dependencies (same as for Windows).
2) `android_configure_release.bat`
3) `android_build.bat` to build IFStile.
4) `Setup/create_android.bat` to create Setup (apk and aab).

## WebAssembly (cross-compiling on Windows)
**Pre-requisites:**
- [Python 3](https://www.python.org/)
- [Emscripten](https://emscripten.org/docs/getting_started/downloads.html)
- Set environment variable: `EMSDK=path/to/emsdk`

**Run:**
1) `bootstrap.bat` to download 3rd party dependencies (same as for Windows).
2) `wasm_configure_release.bat`
3) `wasm_build.bat` to build IFStile

#### Some additional information

- All executable files will be generated in the `bin` folder.
- All setup files will be generated in the `Setup` folder.
- The project is compatible with well-known IDEs.
