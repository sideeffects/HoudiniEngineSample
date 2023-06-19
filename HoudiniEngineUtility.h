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

#pragma once

#include <HAPI/HAPI.h>

#include <string>

// Error checking - this macro will check the status and return specified parameter in case of failure.
#define HOUDINI_CHECK_ERROR_RETURN( HAPI_PARAM_CALL, HAPI_PARAM_RETURN ) \
    do \
    { \
        HAPI_Result ResultVariable = HAPI_PARAM_CALL; \
        if ( ResultVariable != HAPI_RESULT_SUCCESS ) \
        { \
            std::cout << "HAPI failed: " << HoudiniEngineUtility::getLastError() << "  (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            return HAPI_PARAM_RETURN; \
        } \
    } \
    while ( 0 )

// Simple Error checking - this macro will check the status and log the error if any.
#define HOUDINI_CHECK_ERROR( HAPI_PARAM_CALL ) \
    do \
    { \
        HAPI_Result ResultVariable = HAPI_PARAM_CALL; \
        if ( ResultVariable != HAPI_RESULT_SUCCESS ) \
        { \
            std::cout << "HAPI failed: " << HoudiniEngineUtility::getLastError() << "  (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        } \
    } \
    while ( 0 )

// Error checking - this macro will check the status and returns it in the param.
#define HOUDINI_CHECK_ERROR_GET( HAPI_PARAM_RESULT, HAPI_PARAM_CALL ) \
    do \
    { \
        *HAPI_PARAM_RESULT = HAPI_PARAM_CALL; \
        if ( *HAPI_PARAM_RESULT != HAPI_RESULT_SUCCESS ) \
        { \
            std::cout << "HAPI failed: " << HoudiniEngineUtility::getLastError() << "  (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        } \
    } \
    while ( 0 )


struct HoudiniEngineUtility
{
public:
	// Helper method to retrieve the last error message
	static std::string getLastError(HAPI_Session* session = nullptr);

    // Helper method to retrieve the last cook error message
    static std::string getLastCookError(HAPI_Session* session = nullptr);

    // Helper method to retrieve the last connection error message
    static std::string getConnectionError();

	// Helper method to retrieve a string from a HAPI_StringHandle
	static std::string getString(const HAPI_Session* session, HAPI_StringHandle string_handle);

	// Helper for handling exceptions on a failed result
	static void ensureSuccess(HAPI_Result result);

	// Save the session to a .hip file in the application directory
	static bool saveToHip(const HAPI_Session* session, const std::string& filename);
};