# Houdini Engine - Getting Started

Welcome to the Houdini Engine Sample Application.

This app is intended for studios to use as reference and inspiration when writing Houdini integrations for their own DCC application or game engine using Houdini Engine. Overtime we hope to update the source code you find here with best practices and code samples to handle use-cases such as working with multiparms, heightfields, leveraging PDG and more.

### Documentation

Please visit our [Houdini Engine Documentation](https://www.sidefx.com/docs/hengine/index.html) for information on how to maintain and extend the sample code provided to build your own Houdini Engine integration.

### Compiling

Set the following environment variables to build with your local copy of the Houdini Engine headers and import library:

Windows:
```
export HOUDINI_INSTALL_DIR=`cygpath -ms "<HOUDINI_INSTALL_DIR>"`
export HOUDINI_HAPI_HEADERS=$HOUDINI_INSTALL_DIR/toolkit/include/
export HOUDINI_HAPI_LIB=$HOUDINI_INSTALL_DIR/custom/houdini/dsolib/libHAPIL.lib
```

Mac:
```
export HOUDINI_HAPI_HEADERS=$HOUDINI_INSTALL_DIR/Frameworks/Houdini.framework/Resources/toolkit/include/
export HOUDINI_HAPI_LIB=$HOUDINI_INSTALL_DIR/Frameworks/Houdini.framework/Libraries/libHAPIL.dylib
```

Linux:
```
export HOUDINI_HAPI_HEADERS=$HOUDINI_INSTALL_DIR/toolkit/include/
export HOUDINI_HAPI_LIB=$HOUDINI_INSTALL_DIR/dsolib/libHAPIL.so
```

Run cmake & Build:
```
mkdir build
cd build
cmake ..
cmake --build . --target install
```

This will place the HoudiniEngineSample execuatble in the project's /bin folder.

### Project Structure

* HoudiniEngineManager - How to start/cleanup sessions, load HDAs and query parameters & attributes
* HoudiniEngineGeometry - How to marshal geometry in and out of Houdini.
* HoudiniEngineUtility - Utility functions for string conversion, fetching errors etc.
* HDA/hexagona_lite.hda - Sample HDA for generating hexagonal terrain

### Version Compatibility

The HoudiniEngineSample application is compatible with HAPI version 5.0. For more details please see:https://www.sidefx.com/docs/hengine/_h_a_p_i__migration.html

### More Resources

* Houdini Engine for Unreal: https://github.com/sideeffects/HoudiniEngineForUnreal
* Houdini Engine for Unity: https://github.com/sideeffects/HoudiniEngineForUnity
* Houdini Engine for 3dsMax: https://github.com/sideeffects/HoudiniEngineFor3dsMax
* Houdini Engine for Maya: https://github.com/sideeffects/HoudiniEngineForMaya