echo off

rmdir _wasm32 /s /q
call wasm_path.bat
mkdir _wasm32 & pushd _wasm32
cmake .. -G "MinGW Makefiles"  -DCMAKE_BUILD_TYPE=DEBUG   -DCMAKE_TOOLCHAIN_FILE=%EMSDK%\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake
popd
