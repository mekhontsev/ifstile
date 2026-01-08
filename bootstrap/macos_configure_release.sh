cwd=$(pwd)

cd ../external
rm -r ./_macos
mkdir ./_macos
cd ./_macos

cmake $cwd -G "Xcode" -DCMAKE_BUILD_TYPE=Release


