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

#include "HoudiniApi.h"
#include "HoudiniEngineManager.h"
#include "HoudiniEngineUtility.h"

#include <iostream>
#include <vector>

HoudiniEngineManager::HoudiniEngineManager() : mySession{}, myCookOptions{}
{
}

bool 
HoudiniEngineManager::startSession(SessionType session_type, bool connect_to_debugger, bool use_cooking_thread)
{
    // Only start a new Session if we dont already have a valid one
    if (HAPI_RESULT_SUCCESS == HoudiniApi::IsSessionValid(&mySession))
        return true;

    // Clear the connection error before starting a new session
    HoudiniApi::ClearConnectionError();

    // Init the thrift server options
    HAPI_ThriftServerOptions server_options{ 0 };
    server_options.autoClose = true;
    server_options.timeoutMs = 3000.0f;

    mySessionType = session_type;

    HAPI_Result SessionResult = HAPI_RESULT_FAILURE;
    if (session_type == SessionType::InProcess)
    {
        std::cout << "Creating a HAPI in-process session...\n";

        // In-Process HAPI
        SessionResult = HoudiniApi::CreateInProcessSession(&mySession);
    }
    else if (session_type == SessionType::NamedPipe)
    {
        std::cout << "Creating a HAPI named-pipe session...\n";

        // Try to connect to an existing session first...this may fail
        SessionResult = HoudiniApi::CreateThriftNamedPipeSession(&mySession, "hapi");

        if (!connect_to_debugger && SessionResult != HAPI_RESULT_SUCCESS)
        {
            // start our server
            std::cout << "Starting a named-pipe server...\n";
            HAPI_ProcessId process_id;
            HOUDINI_CHECK_ERROR(HoudiniApi::StartThriftNamedPipeServer(&server_options, "hapi", &process_id, nullptr));

            // and connect to the newly started server
            std::cout << "Connecting to the named-pipe session...\n";
            SessionResult = HoudiniApi::CreateThriftNamedPipeSession(&mySession, "hapi");
        }
    }
    else
    {
        std::cout << "Creating a HAPI TCP socket session...\n";

        // Try to connect to an existing  session first...
        SessionResult = HoudiniApi::CreateThriftSocketSession(&mySession, "localhost", 9090);

        // TCP Socket server
        if (!connect_to_debugger && SessionResult != HAPI_RESULT_SUCCESS)
        {
            // start our server
            std::cout << "Starting a TCP socket server...\n";
            HAPI_ProcessId process_id;
            HOUDINI_CHECK_ERROR(HoudiniApi::StartThriftSocketServer(&server_options, 9090, &process_id, nullptr));

            // and connect to the newly started server
            std::cout << "Connecting to the TCP socket session...\n";
            SessionResult = HoudiniApi::CreateThriftSocketSession(&mySession, "localhost", 9090);
        }
    }

    if (SessionResult != HAPI_RESULT_SUCCESS)
    {
        if (session_type != SessionType::InProcess)
        {
            std::string connectionError = HoudiniEngineUtility::getConnectionError();
            if (!connectionError.empty())
                std::cout << "Houdini Engine Session failed to connect - " << connectionError << std::endl;
        }

        return false;
    }

    return true;
}

bool
HoudiniEngineManager::stopSession()
{
    std::cout << "\nCleaning up and closing session..." << std::endl;

    if (HAPI_RESULT_SUCCESS == HoudiniApi::IsSessionValid(&mySession))
    {
        // SessionPtr is valid, clean up and close the session
        HoudiniApi::Cleanup(&mySession);

        // When using an in-process session, this method must be called 
        // in order for the host process to shutdown cleanly.
        if (mySessionType == InProcess)
            HoudiniApi::Shutdown(&mySession);

        HoudiniApi::CloseSession(&mySession);
    }

    return true;
}

bool
HoudiniEngineManager::restartSession(SessionType session_type, bool connect_to_debugger, bool use_cooking_thread)
{
    HAPI_Session* SessionPtr = &mySession;

    std::cout << "Restarting the Houdini Engine session...\n";

    // Make sure we stop the current session if it is still valid
    bool bSuccess = false;
    stopSession();

    if (!startSession(session_type, connect_to_debugger, use_cooking_thread))
    {
        std::cout << "Failed to restart the Houdini Engine session - Failed to start the new Session" << std::endl;
    }
    else
    {
        // Now initialize HAPI with this session
        if (!initializeHAPI(use_cooking_thread))
        {
            std::cout << "Failed to restart the Houdini Engine session - Failed to initialize HAPI" << std::endl;
        }
        else
        {
            bSuccess = true;
        }
    }

    return bSuccess;
}

bool
HoudiniEngineManager::initializeHAPI(bool use_cooking_thread)
{
    // We need a Valid Session
    if (HAPI_RESULT_SUCCESS != HoudiniApi::IsSessionValid(getSession()))
    {
        std::cout << "Failed to initialize HAPI: The session is invalid." << std::endl;
        return false;
    }

    if (HoudiniApi::IsInitialized(getSession()) == HAPI_RESULT_NOT_INITIALIZED)
    {
        // Initialize HAPI
        HAPI_CookOptions cook_options = HoudiniApi::CookOptions_Create();

        cook_options.curveRefineLOD = 8.0f;
        cook_options.clearErrorsAndWarnings = false;
        cook_options.maxVerticesPerPrimitive = 3;
        cook_options.splitGeosByGroup = false;
        cook_options.refineCurveToLinear = true;
        cook_options.handleBoxPartTypes = false;
        cook_options.handleSpherePartTypes = false;
        cook_options.splitPointsByVertexAttributes = false;
        cook_options.packedPrimInstancingMode = HAPI_PACKEDPRIM_INSTANCING_MODE_FLAT;

        HAPI_Result Result = HoudiniApi::Initialize(
            getSession(),           // session
            &cook_options,
            use_cooking_thread,     // use_cooking_thread
            -1,                     // cooking_thread_stack_size
            "",                     // houdini_environment_files
            nullptr,                // otl_search_path
            nullptr,                // dso_search_path
            nullptr,                // image_dso_search_path
            nullptr                 // audio_dso_search_path
        );

        myCookOptions = cook_options;

        if (Result == HAPI_RESULT_SUCCESS)
        {
            std::cout << "Successfully intialized Houdini Engine." << std::endl;
        }
        else if (Result == HAPI_RESULT_ALREADY_INITIALIZED)
        {
            // Reused session? just notify the user
            std::cout << "Successfully intialized Houdini Engine - HAPI was already initialzed." << std::endl;
        }
        else
        {
            std::cout << "Houdini Engine API initialization failed" << std::endl;

            return false;
        }
    }

    return true;
}

HAPI_Session* 
HoudiniEngineManager::getSession()
{
    return &mySession;
}

HAPI_CookOptions* 
HoudiniEngineManager::getCookOptions()
{
    return &myCookOptions;
}

bool 
HoudiniEngineManager::loadAsset(const char* otl_path, HAPI_AssetLibraryId& asset_library_id, std::string& asset_name)
{
    if (!getSession())
        return false;

    // Load the library from file
    std::cout << "Loading asset..." << std::endl;
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::LoadAssetLibraryFromFile(getSession(), otl_path, false, &asset_library_id), false); 

    int asset_count;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetAvailableAssetCount(getSession(), asset_library_id, &asset_count), false);
    if (asset_count > 1)
    {
        std::cout << "Should only be loading 1 asset here" << std::endl;
        exit (1);
    }

    HAPI_StringHandle assetSH;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetAvailableAssets(getSession(), asset_library_id, &assetSH, asset_count),false);
    asset_name = HoudiniEngineUtility::getString(getSession(), assetSH);
    
    std::cout << "  Loaded: " << asset_name << std::endl;
    return true;
}

bool 
HoudiniEngineManager::createAndCookNode(const char* operator_name, HAPI_NodeId * node_id)
{
    std::cout << "\nCreating and cooking node: " << operator_name << "..." << std::endl;
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::CreateNode(getSession(), -1, operator_name, "hexagona_lite", false, node_id), false);

    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::CookNode(getSession(), *node_id, getCookOptions()), false);
    
    if(waitForCook())
    {
        std::cout << "Cook complete." << std::endl;
    }
    return true;
}

bool 
HoudiniEngineManager::waitForCook()
{
    if (!getSession())
        return false;

    int status;
    HAPI_Result result;
    do
    {
        result = HoudiniApi::GetStatus(getSession(), HAPI_STATUS_COOK_STATE, &status);
    }
    while(status > HAPI_STATE_MAX_READY_STATE && result == HAPI_RESULT_SUCCESS);

    if (status != HAPI_STATE_READY || result != HAPI_RESULT_SUCCESS)
    {
        std::cout << "Cook failure: " << HoudiniEngineUtility::getLastCookError() << std::endl;
        return false;
    }
    return true;
}

bool 
HoudiniEngineManager::getParameters(HAPI_NodeId node_id)
{
    HAPI_NodeInfo node_info;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetNodeInfo(getSession(), node_id, &node_info), false);
    
    HAPI_ParmInfo * parm_infos = new HAPI_ParmInfo[node_info.parmCount];
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::GetParameters(
            getSession(), 
            node_id, 
            parm_infos, 
            0,
            node_info.parmCount
        ), false);
    
    std::cout << "\nParameters: " << std::endl;
    std::cout << "==========" << std::endl;
    for( int i = 0; i < node_info.parmCount; ++i )
    {
        std::cout << "  Name: ";
        std::cout << HoudiniEngineUtility::getString(getSession(), parm_infos[i].nameSH) << std::endl;
        std::cout << "  Values: (";

        if (HoudiniApi::ParmInfo_IsInt(&parm_infos[i]))
        {
            int parm_int_count = HoudiniApi::ParmInfo_GetIntValueCount(&parm_infos[i]);
            int * parm_int_values = new int[parm_int_count];
        
            HOUDINI_CHECK_ERROR_RETURN(
                HoudiniApi::GetParmIntValues(
                    getSession(),
                    node_id, parm_int_values,
                    parm_infos[i].intValuesIndex,
                    parm_int_count
                ), false);

            for (int v = 0; v < parm_int_count; ++v)
            {
                if (v != 0)
                    std::cout << ", ";
                std::cout << parm_int_values[v];
            }
        
            delete [] parm_int_values;
        }
        else if (HoudiniApi::ParmInfo_IsFloat(&parm_infos[i]))
        {
            int parm_float_count = HoudiniApi::ParmInfo_GetFloatValueCount(&parm_infos[i]);
            float * parm_float_values = new float[parm_float_count];
        
            HOUDINI_CHECK_ERROR_RETURN(
                HoudiniApi::GetParmFloatValues(
                    getSession(),
                    node_id, parm_float_values,
                    parm_infos[i].floatValuesIndex,
                    parm_float_count
                ), false);

            for (int v = 0; v < parm_float_count; ++v)
            {
                if (v != 0)
                    std::cout << ", ";
                std::cout << parm_float_values[v];
            }
        
            delete [] parm_float_values;
        }
        else if (HoudiniApi::ParmInfo_IsString(&parm_infos[i]))
        {
            int parm_string_count = HoudiniApi::ParmInfo_GetStringValueCount(&parm_infos[i]);
            HAPI_StringHandle * parmSH_values = new HAPI_StringHandle[parm_string_count];
    
            HOUDINI_CHECK_ERROR_RETURN(
                HoudiniApi::GetParmStringValues(
                    getSession(),
                    node_id,
                    true, parmSH_values,
                    parm_infos[ i ].stringValuesIndex,
                    parm_string_count
                ), false);
        
            for (int v = 0; v < parm_string_count; ++v)
            {
                if (v != 0)
                    std::cout << ", ";
        
                std::cout << HoudiniEngineUtility::getString(getSession(), parmSH_values[v]);
            }
            delete [] parmSH_values;
        }
        std::cout << ")" << std::endl;
    }
    delete [] parm_infos;

    return true;
}

bool 
HoudiniEngineManager::getAttributes(HAPI_NodeId node_id, HAPI_PartId part_id)
{
    HAPI_PartInfo part_info;
    HoudiniApi::PartInfo_Init(&part_info);
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::GetPartInfo(getSession(), node_id, part_id, &part_info), false);

    int vertex_attr_count = part_info.attributeCounts[HAPI_ATTROWNER_VERTEX];
    int point_attr_count = part_info.attributeCounts[HAPI_ATTROWNER_POINT];
    int prim_attr_count = part_info.attributeCounts[HAPI_ATTROWNER_PRIM];
    int detail_attr_count = part_info.attributeCounts[HAPI_ATTROWNER_DETAIL];

    std::cout << "\nAttributes: " << std::endl;
    std::cout << "==========" << std::endl;

    // Point attributes
    std::vector <HAPI_StringHandle> point_attr_nameSH;
    point_attr_nameSH.resize(point_attr_count);
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::GetAttributeNames(
            getSession(), 
            node_id, part_id, 
            HAPI_ATTROWNER_POINT, 
            point_attr_nameSH.data(), 
            point_attr_count
        ), false);

    std::cout << "\n  Point Attributes: " << point_attr_count << std::endl;
    std::cout << "  ----------" << std::endl;
    for (int i = 0; i < point_attr_count; ++i)
    {
        std::string attr_name = HoudiniEngineUtility::getString(getSession(), point_attr_nameSH[i]);
        std::cout << "  Name: " << attr_name << std::endl;
        
        HAPI_AttributeInfo attr_info;
        HoudiniApi::AttributeInfo_Init(&attr_info);
        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::GetAttributeInfo(
                getSession(), 
                node_id, part_id, 
                attr_name.c_str(), 
                HAPI_ATTROWNER_POINT, 
                &attr_info
            ), false);

        std::cout << "  Count: " << attr_info.count << " Storage type: " << attr_info.storage << std::endl;
    }

    // Vertex attributes
    std::vector <HAPI_StringHandle> vertex_attr_nameSH;
    vertex_attr_nameSH.resize(vertex_attr_count);
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::GetAttributeNames(
            getSession(), 
            node_id, 
            part_id, 
            HAPI_ATTROWNER_VERTEX, 
            vertex_attr_nameSH.data(), 
            vertex_attr_count
        ), false);

    std::cout << "\n  Vertex Attributes: " << vertex_attr_count << std::endl;
    std::cout << "  ----------" << std::endl;
    for (int i = 0; i < vertex_attr_count; ++i)
    {
        std::string attr_name = HoudiniEngineUtility::getString(getSession(), vertex_attr_nameSH[i]);
        std::cout << "  Name: " << attr_name << std::endl;
        
        HAPI_AttributeInfo attr_info;
        HoudiniApi::AttributeInfo_Init(&attr_info);
        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::GetAttributeInfo(
                getSession(), 
                node_id, part_id, 
                attr_name.c_str(), 
                HAPI_ATTROWNER_VERTEX, 
                &attr_info
            ), false);

        std::cout << "  Count: " << attr_info.count << " Storage type: " << attr_info.storage << std::endl;
    }
   
   // Primitive attributes
    std::vector <HAPI_StringHandle> prim_attr_nameSH;
    prim_attr_nameSH.resize(prim_attr_count);
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::GetAttributeNames(
            getSession(),
            node_id, part_id, 
            HAPI_ATTROWNER_PRIM, 
            prim_attr_nameSH.data(), 
            prim_attr_count
        ), false);

    std::cout << "\n  Primitive Attributes: " << prim_attr_count << std::endl;
    std::cout << "  ----------" << std::endl;
    for (int i = 0; i < prim_attr_count; ++i)
    {
        std::string attr_name = HoudiniEngineUtility::getString(getSession(), prim_attr_nameSH[i]);
        std::cout << "  Name: " << attr_name << std::endl;

        HAPI_AttributeInfo attr_info;
        HoudiniApi::AttributeInfo_Init(&attr_info);
        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::GetAttributeInfo(
                getSession(), 
                node_id, part_id, 
                attr_name.c_str(), 
                HAPI_ATTROWNER_PRIM, 
                &attr_info
            ), false);

        std::cout << "  Count: " << attr_info.count << " Storage type: " << attr_info.storage << std::endl;
    }

    // Detail attributes
    std::vector <HAPI_StringHandle> detail_attr_nameSH;
    detail_attr_nameSH.resize(detail_attr_count);
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::GetAttributeNames(
            getSession(),
            node_id, part_id, 
            HAPI_ATTROWNER_DETAIL, 
            detail_attr_nameSH.data(), 
            detail_attr_count
        ), false);

    std::cout << "\n  Detail Attributes: " << detail_attr_count << std::endl;
    std::cout << "  ----------" << std::endl;
    for (int i = 0; i < detail_attr_count; ++i)
    {
        std::string attr_name = HoudiniEngineUtility::getString(getSession(), detail_attr_nameSH[i]);
        std::cout << "  " << attr_name << std::endl;
    };

    return true;
}