cwd=$(pwd)

cd ../external

rm -r ./_macos
mkdir ./_macos
cd ./_macos

cmake $cwd -G "Xcode"

cmake --build . --config Release


