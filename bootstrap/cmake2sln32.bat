echo off

set this=%cd%
cd ../external

rmdir _sln32 /s /q
mkdir _sln32
cd _sln32

cmake %this% -G "Visual Studio 18 2026" -A Win32 -Thost=x64 
