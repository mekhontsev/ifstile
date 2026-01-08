echo off

set this=%cd%
call cmake2sln.bat
cmake --build . --config Release
cmake --build . --config Debug

cd %this%
call cmake2sln32.bat
cmake --build . --config Release
cmake --build . --config Debug

cd %this%
call cmake2slnARM.bat
cmake --build . --config Release
cmake --build . --config Debug

pause