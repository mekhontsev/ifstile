rem set EMCC_DEBUG=2
rem set EMCC_TEMP_DIR=C:/temp

call wasm_path.bat

pushd _wasm32
cmake --build .
popd

pause