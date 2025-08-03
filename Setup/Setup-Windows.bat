@echo off
pushd %~dp0
echo Setting up Engine project for Windows...

:: Change to repository root (assuming setup/ is one level down)
cd ..

:: Initialize git submodules
echo Initializing git submodules...
git submodule update --init --recursive

:: Generate project files (adjust path since we're now in root)
echo Generating Visual Studio 2022 project files...
pushd setup\
Vendor\Binaries\PreMake\Windows\premake5.exe --file=PremakeScript\Build.lua vs2022
popd

echo Setup complete! Open Engine.sln in Visual Studio 2022.
pause
popd