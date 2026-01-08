
rm -rf ./_macos
sh ./cmake2xcode.sh
cd ./_macos
cmake --build . --config Release
cd ..