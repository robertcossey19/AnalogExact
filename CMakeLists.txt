name: Build VST3

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build-macos:
    runs-on: macos-latest
    steps:
    - uses: actions/checkout@v4
    
    - name: Setup CMake
      uses: jwlawson/actions-setup-cmake@v2
      with:
        cmake-version: '3.26'
    
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    - name: Build
      run: cmake --build build --config Release
    
    - name: Upload macOS VST3
      uses: actions/upload-artifact@v4
      with:
        name: AnalogExact-macOS-VST3
        path: build/AnalogExact_artefacts/Release/VST3/*.vst3
        
    - name: Upload macOS AU
      uses: actions/upload-artifact@v4
      with:
        name: AnalogExact-macOS-AU
        path: build/AnalogExact_artefacts/Release/AU/*.component
        if-no-files-found: ignore

  build-windows:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v4
    
    - name: Setup CMake
      uses: jwlawson/actions-setup-cmake@v2
      with:
        cmake-version: '3.26'
        
    - name: Install WebView2
      shell: powershell
      run: |
        # WebView2 is included in Windows 11 and recent Windows 10
        # For build purposes, we need the development package
        nuget install Microsoft.Web.WebView2 -Version 1.0.2210.55 -OutputDirectory packages
        
    - name: Configure CMake
      run: cmake -B build -G "Visual Studio 17 2022" -A x64

    - name: Build
      run: cmake --build build --config Release
    
    - name: Upload Windows VST3
      uses: actions/upload-artifact@v4
      with:
        name: AnalogExact-Windows-VST3
        path: build/AnalogExact_artefacts/Release/VST3/*.vst3
