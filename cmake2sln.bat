echo off

rmdir _sln /s /q
mkdir _sln
cd _sln

cmake .. -G "Visual Studio 18 2026" -Thost=x64  -A x64
