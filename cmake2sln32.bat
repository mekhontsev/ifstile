echo off

rmdir _sln32 /s /q
mkdir _sln32
cd _sln32

cmake .. -G "Visual Studio 18 2026" -Thost=x64 -A Win32
