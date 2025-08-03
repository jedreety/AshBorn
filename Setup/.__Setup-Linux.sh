#!/bin/bash
cd "$(dirname "$0")"
echo "Setting up Engine project for Linux..."

# Initialize git submodules
echo "Initializing git submodules..."
git submodule update --init --recursive

# Make premake5 executable
chmod +x Vendor/Binaries/PreMake/Linux/premake5

# Generate project files
echo "Generating Makefiles..."
cd Script
../Vendor/Binaries/PreMake/Linux/premake5 gmake2
cd ..

echo "Setup complete! Run 'make' in the root directory to build."
echo "Available configurations: Debug, Release, Dist"
echo "Example: make config=release"