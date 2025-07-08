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
#include "HoudiniEngineGeometry.h"
#include "HoudiniEngineManager.h"
#include "HoudiniEnginePlatform.h"
#include "HoudiniEngineUtility.h"

#include <iostream>
#include <string>

void
printCommandMenu()
{
    std::cout << "\nHoudini Engine Sample Commands" << std::endl;
    std::cout << "------------------------------" << std::endl;
    std::cout << "Working with HDAs" << std::endl;
    std::cout << "  - load: Load a Houdini Digital Asset (HDA)" << std::endl;
    std::cout << "  - cook: Create & cook the loaded HDA" << std::endl;
    std::cout << "  - parms: Fetch and print node parameters" << std::endl;
    std::cout << "  - attribs: Fetch and print node attributes" << std::endl;
    std::cout << "Working with Geometry" << std::endl;
    std::cout << "  - setgeo: Marshal mesh data to Houdini" << std::endl;
    std::cout << "  - getgeo: Read mesh data from Houdini" << std::endl;
    std::cout << "Working with Sessions" << std::endl;
    std::cout << "  - checkvalid: Check if the session is valid" << std::endl;
    std::cout << "General Commands" << std::endl;
    std::cout << "  - help: Print menu of commands"  << std::endl;
    std::cout << "  - save: Save the Houdini session to a hip file" << std::endl;
    std::cout << "  - quit: Cleanup and shutdown the Houdini session" << std::endl;
}

int
main(int argc, char ** argv)
{
    std::cout << "===================================" << std::endl;
    std::cout << " Houdini Engine Sample Application " << std::endl;
    std::cout << "===================================\n" << std::endl;

    // Dynamically load the libHAPIL and load the HAPI
    // functions exported from the dll
    void* libHAPIL = HoudiniEnginePlatform::LoadLibHAPIL();
    if (libHAPIL != nullptr)
        HoudiniApi::InitializeHAPI(libHAPIL);

    if (!HoudiniApi::IsHAPIInitialized())
    {
        std::cerr << "Failed to load and initialize the "
                     "Houdini Engine API from libHAPIL." << std::endl;
        return 1;
    }  

    std::cout << "Start a new Houdini Engine Session via HARS:" << std::endl;
    std::cout << "  1: In-Process Session" << std::endl;
    std::cout << "  2: Named-Pipe Session" << std::endl;
    std::cout << "  3: TCP Socket Session\n" << std::endl;
    std::cout << "Connect to an existing Houdini Engine Session via SessionSync:" << std::endl;
    std::cout << "  4: Existing Named-Pipe Session" << std::endl;
    std::cout << "  5: Existing TCP Socket Session" << std::endl;
    std::cout << "  6: Existing Shared Memory Session\n" << std::endl;
    std::cout << ">> ";
    
    int session_type;
    std::cin >> session_type;

    bool use_cooking_thread = true; // Enables asynchronous cooking of nodes.
    std::string named_pipe = DEFAULT_NAMED_PIPE;
    int tcp_port = DEFAULT_TCP_PORT;
    std::string shared_mem_name;

    if (session_type == HoudiniEngineManager::SessionType::ExistingNamedPipe)
    {
        std::cout << "Please specify the pipe name:" << std::endl;
        std::cout << ">> ";
        std::cin >> named_pipe;
    }
    else if (session_type == HoudiniEngineManager::SessionType::ExistingTCPSocket)
    {
        std::cout << "Please specify the TCP port:" << std::endl;
        std::cout << ">> ";
        std::cin >> tcp_port;
    }
    else if (session_type == HoudiniEngineManager::SessionType::ExistingSharedMemory)
    {
        std::cout << "Please specify the shared memory name:" << std::endl;
        std::cout << ">> ";
        std::cin >> shared_mem_name;
    }

    HoudiniEngineManager* he_manager = new HoudiniEngineManager();
    if (!he_manager)
    {
        std::cerr << "Failed to create the Houdini Engine Manager." << std::endl;
        return 1;
    }

    if (!he_manager->startSession(
        (HoudiniEngineManager::SessionType)session_type, named_pipe, tcp_port, shared_mem_name))
    {
        std::cerr << "Failed to create a Houdini Engine session." << std::endl;
        return 1;
    }

    if (!he_manager->initializeHAPI(use_cooking_thread))
    {
        std::cerr << "Failed to initialize HAPI." << std::endl;
        return 1;
    }

    std::string user_cmd;

    bool hda_loaded = false;
    std::string asset_name;

    bool hda_cooked = false;
    HAPI_NodeId hda_node_id = 0;
    HAPI_PartId hda_part_id = 0;

    bool mesh_data_generated = false;
    HAPI_NodeId input_mesh_node_id = 0;

    printCommandMenu();
    while (user_cmd != "quit")
    {   
        std::cout << ">> ";
        std::cin >> user_cmd;

        if (user_cmd == "load")
        {
            std::cout << "\nEnter an absolute path to the HDA to load "
                            "(or press enter to load the Hexagona HDA): " << std::endl;
            std::cout << ">> ";

            std::string otl_path;
            std::cin.ignore(); // Ignore the trailing newline character
            std::getline(std::cin, otl_path);
            if (otl_path.empty())
            {
                std::cout << "\nLoading the hexagona sample HDA: " << std::endl;
                otl_path = HDA_INSTALL_PATH + std::string("/hexagona_lite.hda");
            }
            
            hda_loaded = he_manager->loadAsset(otl_path.c_str(), asset_name);
            if (!hda_loaded)
            {
                std::cerr << "Failed to load the HDA (" << otl_path << ")." << std::endl;
                return 1;
            }
        }
        else if (user_cmd == "cook")
        {
            if (hda_loaded)
                hda_cooked = he_manager->createAndCookNode(asset_name.c_str(), &hda_node_id);
            else
                std::cerr << "\nThe sample HDA must be loaded before "
                                "it can be cooked (cmd load)." << std::endl;
        }
        else if (user_cmd == "parms")
        {
            if (hda_loaded && hda_cooked)
                he_manager->getParameters(hda_node_id);
            else
                std::cerr << "\nThe sample HDA must be loaded and cooked before "
                            "you can query its parameters (cmd load, cmd cook)." << std::endl;
        }
        else if (user_cmd == "attribs")
        {
            if (hda_loaded && hda_cooked)
                he_manager->getAttributes(hda_node_id, hda_part_id);
            else
                std::cerr << "\nThe sample HDA must be loaded and cooked before "
                             "you can query its attributes (cmd load, cmd cook)." << std::endl;
        }
        else if (user_cmd == "setgeo")
        {
            mesh_data_generated = HoudiniEngineGeometry::sendGeometryToHoudini(
                he_manager->getSession(), he_manager->getCookOptions(), &input_mesh_node_id);
        }
        else if (user_cmd == "getgeo")
        {
            if (mesh_data_generated)
                HoudiniEngineGeometry::readGeometryFromHoudini(
                    he_manager->getSession(), 
                    input_mesh_node_id,
                    he_manager->getCookOptions()
                );
            else
                std::cerr << "\nMesh data must be set and sent to Houdini to "
                             "cook before it can be queried (cmd setgeo)." << std::endl;
        }
        else if (user_cmd == "checkvalid")
        {
            HAPI_Session* session = he_manager->getSession();
            
            if (!session)
            {
                std::cerr << "No session exists." << std::endl;
            }

            HAPI_Result result = HoudiniApi::IsSessionValid(session);

            if (result == HAPI_RESULT_SUCCESS)
            {
                std::cout << "The session is VALID." << std::endl;
            }
            else if (result == HAPI_RESULT_INVALID_SESSION)
            {
                std::cout << "The session is INVALID." << std::endl;
            }
            else
            {
                std::cerr << "Invalid result when checking session validity. "
                    << "Something went wrong." << std::endl;
            }
        }
        else if(user_cmd == "help")
        {
            printCommandMenu();
        }
        else if(user_cmd == "save")
        {
            std::string filename;
            std::cout << "\nFilename (.hip) to save the session to: ";
            std::cin >> filename;
            bool success = HoudiniEngineUtility::saveToHip(he_manager->getSession(), filename);
            
            if (success)
                std::cout << "Hip file saved successfully." << std::endl;
            else
                std::cout << HoudiniEngineUtility::getLastError() << std::endl;
        }
    }
    he_manager->stopSession();
    delete he_manager;

    HoudiniApi::FinalizeHAPI();
    HoudiniEnginePlatform::FreeLibHAPIL(libHAPIL);

    return 0;
}
