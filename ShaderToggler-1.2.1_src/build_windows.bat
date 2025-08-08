@echo off
setlocal
set GEN=Visual Studio 17 2022
rem Build x64
cmake -S . -B build\x64 -G "%GEN%" -A x64 || goto :eof
cmake --build build\x64 --config Release || goto :eof
rem Build Win32
cmake -S . -B build\x86 -G "%GEN%" -A Win32 || goto :eof
cmake --build build\x86 --config Release || goto :eof
echo Builds complete. Find addon at:
echo   build\x64\Release\ShaderToggler.addon
echo   build\x86\Release\ShaderToggler.addon
endlocal
