@echo off

call E:/"Visual Studio 2017"/VC/Auxiliary/Build/vcvarsall.bat x64
mkdir ..\Build
pushd ..\Build
cl -Zi ..\Code\win32_theproject.cpp user32.lib Gdi32.lib
popd