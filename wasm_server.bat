call wasm_path.bat
pushd bin\wasm
call %EMSDK%\upstream\emscripten\emrun --no_browser index.html
popd