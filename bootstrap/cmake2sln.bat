echo off

set this=%cd%
cd ../external

rmdir _sln /s /q
mkdir _sln
cd _sln

cmake %this% -G "Visual Studio 18 2026" -A x64 -Thost=x64 

