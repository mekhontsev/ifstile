echo off

set this=%cd%
cd ../external

rmdir _slnARM /s /q
mkdir _slnARM
cd _slnARM

cmake %this% -G "Visual Studio 18 2026"  -Thost=x64 -A ARM64
