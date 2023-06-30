/*
* Copyright (c) <2023> Side Effects Software Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. The name of Side Effects Software may not be used to endorse or
*    promote products derived from this software without specific prior
*    written permission.
*
* THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE "AS IS" AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
* NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "HoudiniEnginePlatform.h"

#ifdef WIN32
    #include "Windows.h"
#else
    #include <dlfcn.h>
#endif

#include <iostream>
#include <string>

// Names of HAPI libraries on different platforms.
const char* HAPI_LIB_OBJECT_WINDOWS = "libHAPIL.dll";
const char* HAPI_LIB_OBJECT_LINUX = "libHAPIL.so";
const char* HAPI_LIB_OBJECT_MAC = "libHAPIL.dylib";

void*
HoudiniEnginePlatform::LoadLibHAPIL()
{
    void* libHAPIL = nullptr;

#ifdef WIN32
    // Look up the HFS environment variable
    char *buf;
    size_t len;
    if (_dupenv_s(&buf, &len, "HFS") == 0 && buf != nullptr)
    {
        std::string libHAPIL_dir(buf);
        free(buf);

        libHAPIL_dir.append("/bin/");
        if (SetDllDirectory(libHAPIL_dir.c_str()))
        {
            libHAPIL = LoadLibrary(HAPI_LIB_OBJECT_WINDOWS);
        }
    }
    else
    {
        std::cerr << "Unable to retrieve the value of the HFS environment variable." << std::endl;
        return nullptr;
    }
#elif __linux__
    // Location of libHAPIL on Mac & Linux added to the application's RPATH
    libHAPIL = dlopen(HAPI_LIB_OBJECT_LINUX, RTLD_LAZY); 
#else
    libHAPIL = dlopen(HAPI_LIB_OBJECT_MAC, RTLD_LAZY); 
#endif

    if (libHAPIL == nullptr)
        std::cerr << "Failed to load the libHAPIL module." << std::endl;

    return libHAPIL;
}

bool
HoudiniEnginePlatform::FreeLibHAPIL(void* libHAPIL)
{
#ifdef WIN32
    return FreeLibrary((HMODULE)libHAPIL) && SetDllDirectory(nullptr);
#else
    return dlclose(libHAPIL) == 0;
#endif
}

void*
HoudiniEnginePlatform::GetDllExport(void* LibraryHandle, const char* ExportName)
{
#ifdef WIN32
    return GetProcAddress((HMODULE)LibraryHandle, ExportName);
#else
    return dlsym(LibraryHandle, ExportName);
#endif
}
