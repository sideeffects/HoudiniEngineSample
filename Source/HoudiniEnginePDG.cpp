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
#include "HoudiniEnginePDG.h"
#include "HoudiniEngineUtility.h"

#include <cstring>
#include <iostream>

namespace
{
bool waitForCook(HAPI_Session* session)
{
    if (!session)
        return false;

    int status = 0;
    HAPI_Result result = HAPI_RESULT_SUCCESS;
    do
    {
        result = HoudiniApi::GetStatus(session, HAPI_STATUS_COOK_STATE, &status);
    }
    while (status > HAPI_STATE_MAX_READY_STATE && result == HAPI_RESULT_SUCCESS);

    if (status != HAPI_STATE_READY || result != HAPI_RESULT_SUCCESS)
    {
        std::cout << "Cook failure: " << HoudiniEngineUtility::getLastCookError() << std::endl;
        return false;
    }

    return true;
}
}

bool
HoudiniEnginePDG::loadPDGOutputFiles(HAPI_Session* session, HAPI_CookOptions* cook_options, const std::vector<std::string>& bgeo_files, std::vector<HAPI_NodeId>& loaded_node_ids)
{
    // Loads all the bgeo files into Houdini, returning the nodes for each file.

    loaded_node_ids.clear();

    auto hasSuffix = [](const std::string& value, const char* suffix)
    {
        const size_t suffix_length = std::strlen(suffix);
        return value.size() >= suffix_length &&
            value.compare(value.size() - suffix_length, suffix_length, suffix) == 0;
    };

    for (const std::string& output_file : bgeo_files)
    {
        if (!hasSuffix(output_file, ".bgeo.sc"))
            continue;

        HAPI_NodeId loaded_node_id = -1;
        std::string node_name = "PDG_Output_" + std::to_string(loaded_node_ids.size());

        std::cout << "Loading PDG output file: " << output_file << std::endl;

        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::CreateInputNode(session, -1, &loaded_node_id, node_name.c_str()), false);

        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::LoadGeoFromFile(session, loaded_node_id, output_file.c_str()), false);

        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::CookNode(session, loaded_node_id, cook_options), false);

        if (!waitForCook(session))
            return false;

        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::SetNodeDisplay(session, loaded_node_id, 1), false);

        loaded_node_ids.push_back(loaded_node_id);

        std::cout << "Loaded to node: " << loaded_node_id << std::endl;
    }

    if (loaded_node_ids.empty())
    {
        std::cout << "No .bgeo.sc PDG output files were found to load." << std::endl;
    }
    else
    {
        std::cout << "Loaded " << loaded_node_ids.size() << " PDG output file(s) into the Houdini session." << std::endl;
    }

    return true;
}

bool
HoudiniEnginePDG::printPDGOutputNodeSummary(HAPI_Session* session, HAPI_NodeId node_id)
{
    HAPI_GeoInfo geo_info = HoudiniApi::GeoInfo_Create();
    HOUDINI_CHECK_ERROR_RETURN(
        HoudiniApi::GetDisplayGeoInfo(session, node_id, &geo_info),
        false);

    int point_count = 0;
    int primitive_count = 0;
    int vertex_count = 0;

    for (int part_index = 0; part_index < geo_info.partCount; ++part_index)
    {
        HAPI_PartInfo part_info = HoudiniApi::PartInfo_Create();
        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::GetPartInfo(session, geo_info.nodeId, part_index, &part_info),
            false);

        point_count += part_info.pointCount;
        primitive_count += part_info.faceCount;
        vertex_count += part_info.vertexCount;
    }

    std::cout << "PDG output node " << node_id << ": points=" << point_count
              << " primitives=" << primitive_count << " nverts=" << vertex_count
              << std::endl;

    return true;
}

bool
HoudiniEnginePDG::cookPDGNode(HAPI_Session* session, HAPI_CookOptions* cook_options, HAPI_NodeId hda_node_id, const char* node_path)
{
    static_cast<void>(hda_node_id);

    // Convert node path to node id and start cooking
    HAPI_NodeId node_id = -1;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetNodeFromPath(session, -1, node_path, &node_id), false);

    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::CookPDG(session, node_id, 0, 0), false);

    HAPI_PDG_GraphContextId context_id = -1;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetPDGGraphContextId(session, node_id, &context_id), false);

    // PDG is now cooking asynchronously. Scan for state changes and events

    bool done = false;
    int prev_pdg_state = -1;

    while (!done)
    {
        int pdg_state = 0;
        HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetPDGState(session, context_id, &pdg_state), false);

        if (pdg_state != prev_pdg_state)
        {
            std::cout << "PDG State transitioned from " << getPDGStateString(prev_pdg_state);
            std::cout << " " << getPDGStateString(pdg_state) << std::endl;
            prev_pdg_state = pdg_state;
        }

        // PDG Events tend to come in batches, its much more efficient to fetch several at once.
        constexpr int max_events = 64;
        HAPI_PDG_EventInfo events[max_events];
        int event_count = 0;
        int remaining_events = 0;

        HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetPDGEvents(
            session,
            context_id,
            events,
            max_events,
            &event_count,
            &remaining_events), false);

        for (int i = 0; i < event_count; ++i)
        {
            const HAPI_PDG_EventInfo& event_info = events[i];

            printPDGEvent(session, event_info);

            switch (event_info.eventType)
            {
            case HAPI_PDG_EVENT_WORKITEM_STATE_CHANGE:
            {
                if (event_info.currentState == HAPI_PDG_WORKITEM_COOKED_SUCCESS || event_info.currentState == HAPI_PDG_WORKITEM_COOKED_CACHE)
                {
                    // We can fetch the output files and process them now. In
                    // this sample, we just wait for the PDG graph to complete,
                    // and process then.
                }
                break;
            }

            case HAPI_PDG_EVENT_COOK_COMPLETE:
            case HAPI_PDG_EVENT_COOK_ERROR:
                done = true;
                break;

            default:
                break;
            }
        }
    }

    // Fetch the results of all the completed work items
    HAPI_PDG_GraphContextId pdg_context = 0;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetPDGGraphContextId(session, node_id, &pdg_context), false);

    int num_work_items = 0;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetNumWorkItems(session, node_id, &num_work_items), false);

    std::vector<HAPI_PDG_WorkItemId> work_item_ids(num_work_items);
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetWorkItems(session, node_id, work_item_ids.data(), num_work_items), false);

    // Compile a list of all output paths for the work item files
    std::vector<std::string> output_paths;

    for (HAPI_PDG_WorkItemId work_item_id : work_item_ids)
    {
        HAPI_PDG_WorkItemInfo info{};
        HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetWorkItemInfo(session, pdg_context, work_item_id, &info), false);

        std::vector<HAPI_PDG_WorkItemOutputFile> outputs(info.outputFileCount);

        HOUDINI_CHECK_ERROR_RETURN(
            HoudiniApi::GetWorkItemOutputFiles(session, node_id, work_item_id, outputs.data(), info.outputFileCount), false);

        for (const HAPI_PDG_WorkItemOutputFile& output : outputs)
        {
            const std::string path = HoudiniEngineUtility::getHAPIString(session, output.filePathSH);
            const std::string tag = HoudiniEngineUtility::getHAPIString(session, output.tagSH);

            std::cout << "Output tag: " << tag << " path: " << path << std::endl;
            output_paths.push_back(path);
        }
    }

    // Load all output files into Houdini as nodes
    std::vector<HAPI_NodeId> file_node_ids;
    if (!loadPDGOutputFiles(session, cook_options, output_paths, file_node_ids))
        return false;

    // Print a summary of each loaded node. This is where you would do HAPI calls to extract data
    for (HAPI_NodeId file_node_id : file_node_ids)
    {
        if (!printPDGOutputNodeSummary(session, file_node_id))
            return false;
    }

    return true;
}

bool
HoudiniEnginePDG::dirtyPDGNode(HAPI_Session* session, const char* node_path, bool clean_results)
{
    HAPI_NodeId node_id = -1;
    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::GetNodeFromPath(session, -1, node_path, &node_id), false);

    std::cout << "Dirtying PDG node: " << node_path << std::endl;

    HOUDINI_CHECK_ERROR_RETURN(HoudiniApi::DirtyPDGNode(session, node_id, clean_results), false);
    return true;
}


std::string
HoudiniEnginePDG::getPDGEventTypeString(HAPI_PDG_EventType eventType)
{
    switch (eventType)
    {
        case HAPI_PDG_EVENT_NULL:
            return "HAPI_PDG_EVENT_NULL";

        case HAPI_PDG_EVENT_WORKITEM_ADD:
            return "HAPI_PDG_EVENT_WORKITEM_ADD";

        case HAPI_PDG_EVENT_WORKITEM_REMOVE:
            return "HAPI_PDG_EVENT_WORKITEM_REMOVE";

        case HAPI_PDG_EVENT_WORKITEM_STATE_CHANGE:
            return "HAPI_PDG_EVENT_WORKITEM_STATE_CHANGE";

        case HAPI_PDG_EVENT_WORKITEM_ADD_DEP:
            return "HAPI_PDG_EVENT_WORKITEM_ADD_DEP";

        case HAPI_PDG_EVENT_WORKITEM_REMOVE_DEP:
            return "HAPI_PDG_EVENT_WORKITEM_REMOVE_DEP";

        case HAPI_PDG_EVENT_WORKITEM_ADD_PARENT:
            return "HAPI_PDG_EVENT_WORKITEM_ADD_PARENT";

        case HAPI_PDG_EVENT_WORKITEM_REMOVE_PARENT:
            return "HAPI_PDG_EVENT_WORKITEM_REMOVE_PARENT";

        case HAPI_PDG_EVENT_NODE_CLEAR:
            return "HAPI_PDG_EVENT_NODE_CLEAR";

        case HAPI_PDG_EVENT_WORKITEM_RESULT:
            return "HAPI_PDG_EVENT_WORKITEM_RESULT";

        case HAPI_PDG_EVENT_COOK_ERROR:
            return "HAPI_PDG_EVENT_COOK_ERROR";

        case HAPI_PDG_EVENT_COOK_WARNING:
            return "HAPI_PDG_EVENT_COOK_WARNING";

        case HAPI_PDG_EVENT_COOK_COMPLETE:
            return "HAPI_PDG_EVENT_COOK_COMPLETE";

        case HAPI_PDG_EVENT_DIRTY_START:
            return "HAPI_PDG_EVENT_DIRTY_START";

        case HAPI_PDG_EVENT_DIRTY_STOP:
            return "HAPI_PDG_EVENT_DIRTY_STOP";

        case HAPI_PDG_EVENT_DIRTY_ALL:
            return "HAPI_PDG_EVENT_DIRTY_ALL";

        case HAPI_PDG_EVENT_UI_SELECT:
            return "HAPI_PDG_EVENT_UI_SELECT";

        case HAPI_PDG_EVENT_NODE_CREATE:
            return "HAPI_PDG_EVENT_NODE_CREATE";

        case HAPI_PDG_EVENT_NODE_REMOVE:
            return "HAPI_PDG_EVENT_NODE_REMOVE";

        case HAPI_PDG_EVENT_NODE_RENAME:
            return "HAPI_PDG_EVENT_NODE_RENAME";

        case HAPI_PDG_EVENT_NODE_CONNECT:
            return "HAPI_PDG_EVENT_NODE_CONNECT";

        case HAPI_PDG_EVENT_NODE_DISCONNECT:
            return "HAPI_PDG_EVENT_NODE_DISCONNECT";

        case HAPI_PDG_EVENT_NODE_FIRST_COOK:
            return "HAPI_PDG_EVENT_NODE_FIRST_COOK";

        case HAPI_PDG_EVENT_WORKITEM_SET_INT:
            return "HAPI_PDG_EVENT_WORKITEM_SET_INT";

        case HAPI_PDG_EVENT_WORKITEM_SET_FLOAT:
            return "HAPI_PDG_EVENT_WORKITEM_SET_FLOAT";

        case HAPI_PDG_EVENT_WORKITEM_SET_STRING:
            return "HAPI_PDG_EVENT_WORKITEM_SET_STRING";

        case HAPI_PDG_EVENT_WORKITEM_SET_FILE:
            return "HAPI_PDG_EVENT_WORKITEM_SET_FILE";

        case HAPI_PDG_EVENT_WORKITEM_SET_DICT:
            return "HAPI_PDG_EVENT_WORKITEM_SET_DICT";

        case HAPI_PDG_EVENT_WORKITEM_SET_PYOBJECT:
            return "HAPI_PDG_EVENT_WORKITEM_SET_PYOBJECT";

        case HAPI_PDG_EVENT_WORKITEM_SET_GEOMETRY:
            return "HAPI_PDG_EVENT_WORKITEM_SET_GEOMETRY";

        case HAPI_PDG_EVENT_WORKITEM_MERGE:
            return "HAPI_PDG_EVENT_WORKITEM_MERGE";

        case HAPI_PDG_EVENT_WORKITEM_PRIORITY:
            return "HAPI_PDG_EVENT_WORKITEM_PRIORITY";

        case HAPI_PDG_EVENT_COOK_START:
            return "HAPI_PDG_EVENT_COOK_START";

        case HAPI_PDG_EVENT_WORKITEM_ADD_STATIC_ANCESTOR:
            return "HAPI_PDG_EVENT_WORKITEM_ADD_STATIC_ANCESTOR";

        case HAPI_PDG_EVENT_WORKITEM_REMOVE_STATIC_ANCESTOR:
            return "HAPI_PDG_EVENT_WORKITEM_REMOVE_STATIC_ANCESTOR";

        case HAPI_PDG_EVENT_NODE_PROGRESS_UPDATE:
            return "HAPI_PDG_EVENT_NODE_PROGRESS_UPDATE";

        case HAPI_PDG_EVENT_BATCH_ITEM_INITIALIZED:
            return "HAPI_PDG_EVENT_BATCH_ITEM_INITIALIZED";

        case HAPI_PDG_EVENT_ALL:
            return "HAPI_PDG_EVENT_ALL";

        case HAPI_PDG_EVENT_LOG:
            return "HAPI_PDG_EVENT_LOG";

        case HAPI_PDG_EVENT_SCHEDULER_ADDED:
            return "HAPI_PDG_EVENT_SCHEDULER_ADDED";

        case HAPI_PDG_EVENT_SCHEDULER_REMOVED:
            return "HAPI_PDG_EVENT_SCHEDULER_REMOVED";

        case HAPI_PDG_EVENT_SET_SCHEDULER:
            return "HAPI_PDG_EVENT_SET_SCHEDULER";

        case HAPI_PDG_EVENT_SERVICE_MANAGER_ALL:
            return "HAPI_PDG_EVENT_SERVICE_MANAGER_ALL";

        case HAPI_PDG_EVENT_NODE_COOKED:
            return "HAPI_PDG_EVENT_NODE_COOKED";

        case HAPI_PDG_EVENT_NODE_GENERATED:
            return "HAPI_PDG_EVENT_NODE_GENERATED";

        case HAPI_PDG_EVENT_WORKITEM_FRAME:
            return "HAPI_PDG_EVENT_WORKITEM_FRAME";

        case HAPI_PDG_CONTEXT_EVENTS:
            return "HAPI_PDG_CONTEXT_EVENTS";

        default:
            return "Unknown HAPI_PDG_EventType";
    }
}

std::string
HoudiniEnginePDG::getPDGWorkItemStateString(
        HAPI_PDG_WorkItemState workItemState)
{
    switch (workItemState)
    {
        case HAPI_PDG_WORKITEM_UNDEFINED:
            return "HAPI_PDG_WORKITEM_UNDEFINED";

        case HAPI_PDG_WORKITEM_UNCOOKED:
            return "HAPI_PDG_WORKITEM_UNCOOKED";

        case HAPI_PDG_WORKITEM_WAITING:
            return "HAPI_PDG_WORKITEM_WAITING";

        case HAPI_PDG_WORKITEM_SCHEDULED:
            return "HAPI_PDG_WORKITEM_SCHEDULED";

        case HAPI_PDG_WORKITEM_COOKING:
            return "HAPI_PDG_WORKITEM_COOKING";

        case HAPI_PDG_WORKITEM_COOKED_SUCCESS:
            return "HAPI_PDG_WORKITEM_COOKED_SUCCESS";

        case HAPI_PDG_WORKITEM_COOKED_CACHE:
            return "HAPI_PDG_WORKITEM_COOKED_CACHE";

        case HAPI_PDG_WORKITEM_COOKED_FAIL:
            return "HAPI_PDG_WORKITEM_COOKED_FAIL";

        case HAPI_PDG_WORKITEM_COOKED_CANCEL:
            return "HAPI_PDG_WORKITEM_COOKED_CANCEL";

        case HAPI_PDG_WORKITEM_DIRTY:
            return "HAPI_PDG_WORKITEM_DIRTY";

        default:
            return "Unknown HAPI_PDG_WorkItemState";
    }
}


void
HoudiniEnginePDG::printPDGEvent(
        const HAPI_Session* session,
        const HAPI_PDG_EventInfo& ev)
{
    std::cout << "Event: ";
    std::cout << " currentState "
              << getPDGWorkItemStateString(
                         HAPI_PDG_WorkItemState(ev.currentState));
    std::cout << " lastState "
              << getPDGWorkItemStateString(
                         HAPI_PDG_WorkItemState(ev.lastState));
    std::cout << " eventType "
              << getPDGEventTypeString(HAPI_PDG_EventType(ev.eventType));
    std::cout << " workItemId " << ev.workItemId;
    std::cout << " msgSH "
              << HoudiniEngineUtility::getHAPIString(session, ev.msgSH);
    std::cout << std::endl;
}

std::string
HoudiniEnginePDG::getPDGStateString(int pdgState)
{
    switch (pdgState)
    {
        case HAPI_PDG_STATE_READY:
            return "HAPI_PDG_STATE_READY";

        case HAPI_PDG_STATE_COOKING:
            return "HAPI_PDG_STATE_COOKING";

        case HAPI_PDG_STATE_MAX:
            return "HAPI_PDG_STATE_MAX";

        default:
            return "Unknown HAPI_PDG_State (" + std::to_string(pdgState) + ")";
    }
}
