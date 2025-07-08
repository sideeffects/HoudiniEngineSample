# Copyright (c) <2023> Side Effects Software Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. The name of Side Effects Software may not be used to endorse or
#    promote products derived from this software without specific prior
#    written permission.
#
# THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
# NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import hapi


class HoudiniEngineGeometry(object):

    @staticmethod
    def sendGeometryToHoudini(session):
        '''Marshal a mesh (with position, colour, normal and uv data) to Houdini as input'''

        print("\nCreating geometry input node 'input_Cube'...")
        input_cube = hapi.createInputNode(session, -1, "Cube")

        # Use the Geometry Setters API to define a cube mesh
        node_part = hapi.PartInfo()
        node_part.type = hapi.partType.Mesh
        node_part.faceCount = 6
        node_part.vertexCount = 24
        node_part.pointCount = 8

        hapi.setPartInfo(session, input_cube, 0, node_part)

        # Add 'P' (position) point attributes
        print("  Setting position (P) point attributes")

        node_point_info = hapi.AttributeInfo()
        node_point_info.count = 8
        node_point_info.tupleSize = 3
        node_point_info.exists = True
        node_point_info.storage = hapi.storageType.Float
        node_point_info.owner = hapi.attributeOwner.Point

        hapi.addAttribute(session, input_cube, 0, "P", node_point_info)

        positions = [0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0,
                     1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0]

        hapi.setAttributeFloatData(
            session, input_cube, 0, "P", node_point_info, positions, 0, 8)

        # Define the list of vertices
        vertices = [0, 2, 6, 4, 2, 3, 7, 6, 2, 0, 1,
                    3, 1, 5, 7, 3, 5, 4, 6, 7, 0, 4, 5, 1]

        print("  Setting vertex list and face count")
        hapi.setVertexList(session, input_cube, 0, vertices, 0, 24)

        # Set face counts
        face_counts = [4, 4, 4, 4, 4, 4]
        hapi.setFaceCounts(session, input_cube, 0, face_counts, 0, 6)

        print("Sending data to the Houdini cook engine")
        hapi.commitGeo(session, input_cube)

        # Add 'Cd' (Colour) point attributes
        print("  Connecting a Color SOP node")
        node_info = hapi.getNodeInfo(session, input_cube)

        parent_id = node_info.parentId
        color_node = hapi.createNode(
            session, parent_id, "color", "Cube_Color", True)

        hapi.connectNodeInput(session, color_node, 0, input_cube, 0)

        # Add 'N' (Normal) point attributes
        print("  Connecting a Normal SOP node")
        normal_node = hapi.createNode(
            session, parent_id, "normal", "Cube_Normal", True)

        hapi.connectNodeInput(session, normal_node, 0, color_node, 0)

        # Add 'UV' point attributes
        print("  Connecting a UV Project SOP node")
        uv_project_node = hapi.createNode(
            session, parent_id, "uvproject", "Cube_UV", True)

        hapi.connectNodeInput(session, uv_project_node, 0, normal_node, 0)

        # Add Output node & enable its display flag
        output_node = hapi.createNode(
            session, parent_id, "output", "OUT", True)

        hapi.connectNodeInput(session, output_node, 0, uv_project_node, 0)
        hapi.setNodeDisplay(session, output_node, 1)

        return output_node

    @staticmethod
    def readGeometryFromHoudini(session, node_id, cook_options):
        '''Read mesh data from Houdini for processing'''

        hapi.cookNode(session, node_id, cook_options)

        # Check the cook status
        status = hapi.getStatus(session, hapi.statusType.CookState)
        while (status > hapi.state.Ready):
            status = hapi.getStatus(session, hapi.statusType.CookState)

        # Get mesh geo info.
        print("\nGetting mesh geometry info:")
        mesh_geo_info = hapi.getDisplayGeoInfo(session, node_id)

        # Get mesh part info.
        mesh_part_info = hapi.getPartInfo(session, mesh_geo_info.nodeId, 0)

        # Get mesh face counts.
        mesh_face_counts = hapi.getFaceCounts(
            session,
            mesh_geo_info.nodeId,
            mesh_part_info.id,
            0, mesh_part_info.faceCount
        )
        print("  Face count: {}".format(len(mesh_face_counts)))

        # Get mesh vertex list.
        mesh_vertex_list = hapi.getVertexList(
            session,
            mesh_geo_info.nodeId,
            mesh_part_info.id,
            0, mesh_part_info.vertexCount
        )

        print("  Vertex count: {}".format(len(mesh_vertex_list)))

        # Fetch mesh attributes of the given name
        def _fetchPointAttrib(owner, attrib_name):
            mesh_attrib_info = hapi.getAttributeInfo(
                session,
                mesh_geo_info.nodeId,
                mesh_part_info.id,
                attrib_name, owner
            )

            mesh_attrib_data = hapi.getAttributeFloatData(
                session,
                mesh_geo_info.nodeId,
                mesh_part_info.id,
                attrib_name,
                mesh_attrib_info, -1,
                0, mesh_attrib_info.count
            )

            print("  {} attribute count: {}".format(
                attrib_name, len(mesh_attrib_data)))
            return mesh_attrib_data

        mesh_p_attrib_info = _fetchPointAttrib(hapi.attributeOwner.Point, "P")

        mesh_cd_attrib_data = _fetchPointAttrib(
            hapi.attributeOwner.Point, "Cd")

        mesh_N_attrib_data = _fetchPointAttrib(hapi.attributeOwner.Vertex, "N")

        mesh_uv_attrib_data = _fetchPointAttrib(
            hapi.attributeOwner.Vertex, "uv")

        # Now that you have all the required mesh data, you can now create
        # a native mesh using your DCC/engine's dedicated functions:"
        # ...

        return True
