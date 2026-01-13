rm -rf ./_linux

mkdir ./_linux
cd ./_linux

cmake .. -DCMAKE_BUILD_TYPE=RELEASE
cmake --build . --config Release

#cmake .. -DCMAKE_BUILD_TYPE=DEBUG
#cmake --build . --config Debug

cd ..

