call cmake2sln.bat
cmake --build build/msvc --config Release

call cmake2slnARM.bat
cmake --build build/msvcARM --config Release