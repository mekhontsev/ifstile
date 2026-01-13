echo off

set this=%cd%
call ../wasm_path.bat

cd ../external
rmdir _wasm /s /q
mkdir _wasm
cd _wasm
cmake %this% -G "MinGW Makefiles"  -DCMAKE_BUILD_TYPE=DEBUG   -DCMAKE_TOOLCHAIN_FILE=%EMSDK%\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake

