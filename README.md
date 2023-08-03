# Houdini Engine - Getting Started

Welcome to the Houdini Engine Sample Application.

This app is intended for studios to use as reference and inspiration when writing Houdini integrations for their own DCC application or game engine. Over time we hope to update the project with code samples to handle use cases such as working with multiparms, heightfields, leveraging PDG and more.

### Documentation

Please visit our [Houdini Engine Documentation](https://www.sidefx.com/docs/hengine/index.html) for information on how to maintain and extend the sample code provided to build your own Houdini Engine integration.

### Prerequisites

The HoudiniEngineSample is built using CMake which can be downloaded and installed from here: https://cmake.org/download/ (minimum required version is 3.1).

### Building the Sample

Initialize your envirornment by navigating to your Houdini installation directory and sourcing the `houdini_setup` script.

```
# Windows:
cd "C:\\Program Files\\Side Effects Software\\Houdini x.x.x"
source houdini_setup

# Mac:
cd /Applications/Houdini/Houdinix.x.x/Frameworks/Houdini.framework/Resources
source houdini_setup

# Linux:
cd /opt/hfsx.x.x
source houdini_setup
```

Navigate to a directory with write permissions and fetch a copy of the HoudiniEngineSample project to build.
```
git clone https://github.com/sideeffects/HoudiniEngineSample.git
cd HoudiniEngineSample

mkdir build
cd build
cmake ..
cmake --build . --target install
```

This will place an executable named `HoudiniEngineSample.exe` in the project's `/bin` folder.

### Project Structure

* HoudiniEngineManager - How to start/cleanup sessions, load HDAs and query parameters & attributes
* HoudiniEngineGeometry - How to marshal geometry in and out of Houdini
* HoudiniEngineUtility - Utility functions for string conversion, fetching errors etc.
* HoudiniEnginePlatform - Contains OS-specific code for loading the libHAPIL library
* HoudiniApi - This file is generated (do not modify directly). Initializes the HAPI API with functions exported from libHAPIL.
* HDA/hexagona_lite.hda - Sample HDA for generating hexagonal terrain (provided by @christosstavridis)

### Version Compatibility

The HoudiniEngineSample application is compatible with HAPI version 5.0. For more details please see: https://www.sidefx.com/docs/hengine/_h_a_p_i__migration.html

### More Resources

* Houdini Engine for Unreal: https://github.com/sideeffects/HoudiniEngineForUnreal
* Houdini Engine for Unity: https://github.com/sideeffects/HoudiniEngineForUnity
* Houdini Engine for 3dsMax: https://github.com/sideeffects/HoudiniEngineFor3dsMax
* Houdini Engine for Maya: https://github.com/sideeffects/HoudiniEngineForMaya