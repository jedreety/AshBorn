#!/bin/bash
cd "$(dirname "$0")"
echo "Setting up Engine project for macOS..."

# Initialize git submodules
echo "Initializing git submodules..."
git submodule update --init --recursive

# Make premake5 executable
chmod +x Vendor/Binaries/PreMake/Mac/premake5

# Generate project files
echo "Generating Xcode project files..."
cd Script
../Vendor/Binaries/PreMake/Mac/premake5 xcode4
cd ..

echo "Setup complete! Open Engine.xcworkspace in Xcode."
echo "You can also generate makefiles with: ./Vendor/Binaries/PreMake/Mac/premake5 gmake2"