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

#include "HoudiniEngineGeometry.h"
#include "HoudiniEngineUtility.h"

#include <iostream>
#include <vector>

bool 
HoudiniEngineGeometry::sendGeometryToHoudini(const HAPI_Session * session, const HAPI_CookOptions * cook_options, HAPI_NodeId * output_node)
{
    HAPI_NodeId input_cube = -1;
    std::cout << "\nCreating geometry input node 'input_Cube'..." << std::endl;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CreateInputNode(session, &input_cube, "Cube"), false);

    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CookNode(session, input_cube, cook_options), false);

    int cook_status = HAPI_STATE_MAX;
    HAPI_Result result;
    do
    {
        result = HAPI_GetStatus(session, HAPI_STATUS_COOK_STATE, &cook_status);
    }
    while (cook_status > HAPI_STATE_MAX_READY_STATE && result == HAPI_RESULT_SUCCESS);
    
    HOUDINI_CHECK_ERROR_RETURN(result, false);
    
    // Use the Geometry Setters API to define a cube mesh
    HAPI_PartInfo node_part = HAPI_PartInfo_Create();
    node_part.type = HAPI_PARTTYPE_MESH;
    node_part.faceCount = 6;
    node_part.vertexCount = 24;
    node_part.pointCount = 8;

    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_SetPartInfo(session, input_cube, 0, &node_part), false);
    
    // Add 'P' (position) point attributes
    std::cout << "  Setting position (P) point attributes" << std::endl;

    HAPI_AttributeInfo node_point_info = HAPI_AttributeInfo_Create();
    node_point_info.count = 8;
    node_point_info.tupleSize = 3;
    node_point_info.exists = true;
    node_point_info.storage = HAPI_STORAGETYPE_FLOAT;
    node_point_info.owner = HAPI_ATTROWNER_POINT;
    
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_AddAttribute(session, input_cube, 0, "P", &node_point_info), false);
    
    float positions[24] = { 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f,
                            0.0f, 1.0f, 0.0f,
                            0.0f, 1.0f, 1.0f,
                            1.0f, 0.0f, 0.0f,
                            1.0f, 0.0f, 1.0f,
                            1.0f, 1.0f, 0.0f,
                            1.0f, 1.0f, 1.0f };
    
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_SetAttributeFloatData(
            session, input_cube,
            0, "P", &node_point_info,
            positions, 0, 8), false );
    
    // Define the list of vertices
    int vertices[24] = { 0, 2, 6, 4,
                         2, 3, 7, 6,
                         2, 0, 1, 3,
                         1, 5, 7, 3,
                         5, 4, 6, 7,
                         0, 4, 5, 1 };
    
    std::cout << "  Setting vertex list and face count" << std::endl;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_SetVertexList(session, input_cube, 0, vertices, 0, 24), false);
   
    // Set face counts
    int face_counts [ 6 ] = { 4, 4, 4, 4, 4, 4 };
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_SetFaceCounts(session, input_cube, 0, face_counts, 0, 6), false);
    
    std::cout << "Sending data to the Houdini cook engine" << std::endl;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CommitGeo(session, input_cube), false);

    // Add 'Cd' (Colour) point attributes
    std::cout << "  Connecting a Color SOP node" << std::endl;

    HAPI_NodeInfo node_info = HAPI_NodeInfo_Create();
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_GetNodeInfo(session, input_cube, &node_info), false);

    HAPI_NodeId parent_id = node_info.parentId;
    HAPI_NodeId color_node = -1;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CreateNode(session, parent_id, "color", "Cube_Color", true, &color_node), false);

    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_ConnectNodeInput(session, color_node, 0, input_cube, 0), false);

    // Add 'N' (Normal) point attributes
    std::cout << "  Connecting a Normal SOP node" << std::endl;

    HAPI_NodeId normal_node = -1;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CreateNode(session, parent_id, "normal", "Cube_Normal", true, &normal_node), false);

    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_ConnectNodeInput(session, normal_node, 0, color_node, 0), false);

    // Add 'UV' point attributes
    std::cout << "  Connecting a UV Project SOP node" << std::endl;

    HAPI_NodeId uv_project_node = -1;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CreateNode(session, parent_id, "uvproject", "Cube_UV", true, &uv_project_node), false);
    
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_ConnectNodeInput(session, uv_project_node, 0, normal_node, 0), false);

    // Add Output node & enable its display flag
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CreateNode(session, parent_id, "output", "OUT", true, output_node), false);

    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_ConnectNodeInput(session, *output_node, 0, uv_project_node, 0), false);

    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_SetNodeDisplay(session, *output_node, 1), false);

    return true;
}

bool 
HoudiniEngineGeometry::readGeometryFromHoudini(const HAPI_Session * session, const HAPI_NodeId node_id, const HAPI_CookOptions * cook_options)
{
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_CookNode(session, node_id, cook_options), false);

    // Get mesh geo info.
    std::cout << "\nGetting mesh geometry info:" << std::endl;
    HAPI_GeoInfo mesh_geo_info;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_GetDisplayGeoInfo(session, node_id, &mesh_geo_info), false);

    // Get mesh part info.
    HAPI_PartInfo mesh_part_info;
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_GetPartInfo(session, mesh_geo_info.nodeId, 0, &mesh_part_info), false);

    // Get mesh face counts.
    std::vector<int> mesh_face_counts(mesh_part_info.faceCount);
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_GetFaceCounts(
            session,
            mesh_geo_info.nodeId,
            mesh_part_info.id,
            mesh_face_counts.data(),
            0, mesh_part_info.faceCount
        ),
        false);

    std::cout << "  Face count: " << mesh_face_counts.size() << std::endl;

    // Get mesh vertex list.
    std::vector<int> mesh_vertex_list(mesh_part_info.vertexCount);
    HOUDINI_CHECK_ERROR_RETURN(
        HAPI_GetVertexList(
            session, 
            mesh_geo_info.nodeId,
            mesh_part_info.id,
            mesh_vertex_list.data(),
            0, mesh_part_info.vertexCount
        ), 
        false);

    std::cout << "  Vertex count: " << mesh_vertex_list.size() << std::endl;

    // Fetch mesh attributes of the given name
    auto fetchPointAttrib = [&](HAPI_AttributeOwner owner, 
                                const char* attrib_name,
                                std::vector<float>& mesh_attrib_data)
    {
        HAPI_AttributeInfo mesh_attrib_info;
        HOUDINI_CHECK_ERROR(
            HAPI_GetAttributeInfo(
                session,
                mesh_geo_info.nodeId, 
                mesh_part_info.id,
                attrib_name, owner,
                &mesh_attrib_info
            ));
        
        mesh_attrib_data.resize(mesh_attrib_info.count * mesh_attrib_info.tupleSize);
        HOUDINI_CHECK_ERROR(
            HAPI_GetAttributeFloatData(
                session, 
                mesh_geo_info.nodeId, 
                mesh_part_info.id, 
                attrib_name,
                &mesh_attrib_info, -1, 
                mesh_attrib_data.data(),
                0, mesh_attrib_info.count
            ));

        std::cout << "  " <<attrib_name << " attribute count: " << mesh_attrib_data.size() << std::endl;
    };

    std::vector<float> mesh_p_attrib_info;
    fetchPointAttrib(HAPI_ATTROWNER_POINT, "P", mesh_p_attrib_info);

    std::vector<float> mesh_cd_attrib_data;
    fetchPointAttrib(HAPI_ATTROWNER_POINT, "Cd", mesh_cd_attrib_data);

    std::vector<float> mesh_N_attrib_data;
    fetchPointAttrib(HAPI_ATTROWNER_VERTEX , "N", mesh_N_attrib_data);

    std::vector<float> mesh_uv_attrib_data;
    fetchPointAttrib(HAPI_ATTROWNER_VERTEX , "uv", mesh_uv_attrib_data);

    // Now  that you have all the required mesh data, you can now create
    // a native mesh using your DCC/engine's dedicated functions:"
    // ...

    return true;
}