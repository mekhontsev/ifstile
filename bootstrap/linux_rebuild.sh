cwd=$(pwd)

cd ../external

rm -r ./_linux
mkdir ./_linux
cd ./_linux

cmake $cwd -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_C_COMPILER=gcc
cmake --build . --config Release

#cmake $cwd -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_COMPILER=gcc
#cmake --build . --config Debug

