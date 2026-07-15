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
#include <vector>

class HoudiniEnginePDG
{
public:
    // Cook the PDG node and process results
    static bool cookPDGNode(
        HAPI_Session* session, 
        HAPI_CookOptions* cook_options, 
        HAPI_NodeId hda_node_id, 
        const char* node_path);

    // Dirties the PDG Node
    static bool dirtyPDGNode(
        HAPI_Session* session, 
        const char* node_path, 
        bool clean_results);

    // Loads the PDG file outputs into Houdini
    static bool loadPDGOutputFiles(
        HAPI_Session* session, 
        HAPI_CookOptions* cook_options, 
        const std::vector<std::string>& output_files, 
        std::vector<HAPI_NodeId>& loaded_node_ids);

    static bool printPDGOutputNodeSummary(HAPI_Session* session, HAPI_NodeId node_id);

    // Makes a string from the HAPI_PDG_WorkItemState enum
    static std::string getPDGWorkItemStateString(HAPI_PDG_WorkItemState workItemState);

    // Makes a string from the HAPI_PDG_EventType enum
    static std::string getPDGEventTypeString(HAPI_PDG_EventType eventType);

    // Makes a string from the PDG state
    static std::string getPDGStateString(int pdgState);

    // Prints the event to std::cout
    static void printPDGEvent(const HAPI_Session* session, const HAPI_PDG_EventInfo& ev);

};
