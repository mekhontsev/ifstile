
call cmake2sln.bat
cmake --build . --config Release
cd ..

call cmake2slnARM.bat
cmake --build . --config Release
cd ..

pause