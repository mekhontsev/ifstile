rmdir _wasm32 /s /q
call wasm_path.bat
call %EMSDK%\upstream\emscripten\emcc.bat --clear-cache 
call %EMSDK%\emsdk.bat activate latest-upstream
