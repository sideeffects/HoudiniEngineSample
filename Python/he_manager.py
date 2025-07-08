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

from enum import Enum
import he_utility


class SessionType(Enum):
    InProcess = 1
    NewNamedPipe = 2
    NewTCPSocket = 3
    ExistingNamedPipe = 4
    ExistingTCPSocket = 5
    ExistingSharedMemory = 6


class HoudiniEngineManager(object):
    DEFAULT_NAMED_PIPE = "hapi"
    DEFAULT_HOST_NAME = "127.0.0.1"
    DEFAULT_TCP_PORT = 9090

    def __init__(self):
        self.session = None
        self.cook_options = None
        self.session_type = SessionType.InProcess
        self.named_pipe = self.DEFAULT_NAMED_PIPE
        self.tcp_port = self.DEFAULT_TCP_PORT

    def startSession(self, session_type, named_pipe, tcp_port, shared_mem_name="", log_file="./he_log.txt"):
        ''' Creates a new session'''
        # Only start a new Session if we dont already have a valid one
        if self.session and hapi.isSessionValid(self.session):
            return True

        # Clear the connection error before starting a new session
        hapi.clearConnectionError()

        # Init the thrift server options
        server_options = hapi.ThriftServerOptions()
        server_options.autoClose = True
        server_options.timeoutMs = 3000.0

        self.session_type = session_type
        self.named_pipe = named_pipe
        self.tcp_port = tcp_port

        if session_type == SessionType.InProcess.value:
            # In-Process HAPI
            print("Creating a HAPI in-process session...")
            session_info = hapi.SessionInfo()
            self.session = hapi.createInProcessSession(session_info)
        elif session_type == SessionType.NewNamedPipe.value:
            # Start our named-pipe server
            print("Starting a named-pipe server...")
            hapi.startThriftNamedPipeServer(
                server_options, named_pipe, log_file)

            # Connect to the newly started server
            print("Connecting to the named-pipe session...")
            session_info = hapi.SessionInfo()
            self.session = hapi.createThriftNamedPipeSession(
                named_pipe, session_info)
        elif session_type == SessionType.NewTCPSocket.value:
            # Start our socket server
            print("Starting a TCP socket server...")
            hapi.startThriftSocketServer(server_options, tcp_port, log_file)

            # Connect to the newly started server
            print("Connecting to the TCP socket session...")
            session_info = hapi.SessionInfo()
            self.session = hapi.createThriftSocketSession(
                HoudiniEngineManager.DEFAULT_HOST_NAME, tcp_port, session_info)
        elif session_type == SessionType.ExistingNamedPipe.value:
            # Existing named-pipe
            print("Connecting to an existing HAPI named pipe session...")
            session_info = hapi.SessionInfo()
            self.session = hapi.createThriftNamedPipeSession(
                named_pipe, session_info)
        elif session_type == SessionType.ExistingTCPSocket.value:
            # Existing socket server
            print("Connecting to an existing HAPI TCP socket session...")
            session_info = hapi.SessionInfo()
            self.session = hapi.createThriftSocketSession(
                HoudiniEngineManager.DEFAULT_HOST_NAME, tcp_port, session_info)
        elif session_type == SessionType.ExistingSharedMemory.value:
            # Shared memory session
            print("Connecting to an existing HAPI shared memory session...")
            session_info = hapi.SessionInfo()
            self.session = hapi.CreateThriftSharedMemorySession(
                shared_mem_name, session_info)
        else:
            print("Cannot connect to unknown session type ({})".format(session_type))
            return False

        if not self.session:
            connectionError = he_utility.getConnectionError()
            if connectionError:
                print(
                    "Houdini Engine Session failed to connect - {}".format(connectionError))
            return False

        return True

    def restartSession(self, session_type, use_cooking_thread):
        '''Stop the existing session if valid, and creates a new session'''

        print("Restarting the Houdini Engine session...\n")

        # Make sure we stop the current session if it is still valid
        self.stopSession()

        success = False
        if not self.startSession(session_type, self.named_pipe, self.tcp_port):
            print(
                "Failed to restart the Houdini Engine session - Failed to start the new Session")
        else:
            # Now initialize HAPI with this session
            if not self.initializeHAPI(use_cooking_thread):
                print(
                    "Failed to restart the Houdini Engine session - Failed to initialize HAPI")
            else:
                success = True

        return success

    def stopSession(self):
        '''Cleanup and shutdown an existing session'''
        if self.session and hapi.isSessionValid(self.session):
            # SessionPtr is valid, clean up and close the session
            print("\nCleaning up and closing session...")

            hapi.cleanup(self.session)

            # When using an in-process session, this method must be called
            # in order for the host process to shutdown cleanly.
            if self.session_type == SessionType.InProcess:
                hapi.shutdown(self.session)

            hapi.closeSession(self.session)

        return True

    def initializeHAPI(self, use_cooking_thread):
        '''Initializes the HAPI session, should be called after successfully creating a session'''
        # We need a valid Session
        if self.session is None or not hapi.isSessionValid(self.session):
            print("Failed to initialize HAPI: The session is invalid.")
            return False

        # TODO: Currently throwing a hapi.NotInitializedError
        # if hapi.isInitialized(self.session) == hapi.result.NotInitialized:

        # Initialize HAPI
        self.cook_options = hapi.CookOptions()

        self.cook_options.curveRefineLOD = 8.0
        self.cook_options.clearErrorsAndWarnings = False
        self.cook_options.maxVerticesPerPrimitive = 3
        self.cook_options.splitGeosByGroup = False
        self.cook_options.refineCurveToLinear = True
        self.cook_options.handleBoxPartTypes = False
        self.cook_options.handleSpherePartTypes = False
        self.cook_options.splitPointsByVertexAttributes = False
        self.cook_options.packedPrimInstancingMode = hapi.packedPrimInstancingMode.Flat

        success = hapi.initialize(
            self.session,
            self.cook_options,
            use_cooking_thread,
            -1,                     # cooking_thread_stack_size
            "",                     # houdini_environment_files
            "",                     # otl_search_path
            "",                     # dso_search_path
            "",                     # image_dso_search_path
            ""                      # audio_dso_search_path
        )

        if not success:
            print("Houdini Engine API initialization failed")
            return False

        print("Successfully initialized Houdini Engine.")
        return True

    def getSession(self):
        '''Get the HAPI session'''
        return self.session

    def getCookOptions(self):
        '''Get the cook options used to initialize the HAPI session'''
        return self.cook_options

    def loadAsset(self, otl_path):
        '''Load a new HDA asset'''

        if self.getSession() is None:
            return False, ""

        # Load the library from file
        print("Loading asset...")
        asset_library_id = hapi.loadAssetLibraryFromFile(
            self.session, otl_path, False)

        asset_count = hapi.getAvailableAssetCount(
            self.session, asset_library_id)

        if asset_count > 1:
            print("Should only be loading 1 asset here")
            return False, ""

        asset_names_array = hapi.getAvailableAssets(
            self.session, asset_library_id, asset_count)
        asset_name = he_utility.getString(
            self.session, asset_names_array[0])

        print("  Loaded: {}".format(asset_name))
        return True, asset_name

    def _waitForCook(self):
        if self.session is None:
            return False

        status = status = hapi.getStatus(
            self.session, hapi.statusType.CookState)
        while (status > hapi.state.Ready):
            status = hapi.getStatus(self.session, hapi.statusType.CookState)

        if status != hapi.state.Ready:
            print("Cook failure: {}".format(he_utility.getLastCookError()))
            return False

        return True

    def createAndCookNode(self, operator_name, node_id):
        '''Instantiate and asynchronously cook the given node'''

        print("\nCreating and cooking node: {}...".format(operator_name))
        node_id = hapi.createNode(
            self.session, -1, operator_name, "sample_HDA", False)

        hapi.cookNode(self.session, node_id, self.cook_options)

        if self._waitForCook():
            print("Cook complete.")

        return True

    def getParameters(self, node_id):
        '''Query and list the paramters of the given node'''
        node_info = hapi.getNodeInfo(self.session, node_id)

        parm_infos = hapi.getParameters(
            self.session,
            node_id,
            0,
            node_info.parmCount
        )

        print("\nParameters: ")
        print("==========")
        for i in range(node_info.parmCount-1):
            print("  Name: ", end='')
            print(he_utility.getString(
                self.session, parm_infos[i].nameSH))
            print("  Values: (", end='')

            if parm_infos[i].type == hapi.parmType.Int:
                parm_int_count = parm_infos[i].size

                parm_int_values = hapi.getParmIntValues(
                    self.session,
                    node_id,
                    parm_infos[i].intValuesIndex,
                    parm_int_count
                )

                for v in range(parm_int_count):
                    if v != 0:
                        print(", ", end='')
                    print(parm_int_values[v], end='')
            elif parm_infos[i].type == hapi.parmType.Float:
                parm_float_count = parm_infos[i].size

                parm_float_values = hapi.getParmFloatValues(
                    self.session,
                    node_id,
                    parm_infos[i].floatValuesIndex,
                    parm_float_count
                )

                for v in range(parm_float_count):
                    if v != 0:
                        print(", ", end='')
                    print(parm_float_values[v], end='')
            elif parm_infos[i].type == hapi.parmType.String:
                parm_string_count = parm_infos[i].size

                parmSH_values = hapi.getParmStringValues(
                    self.session,
                    node_id,
                    True,
                    parm_infos[i].stringValuesIndex,
                    parm_string_count
                )

                for v in range(parm_string_count):
                    if v != 0:
                        print(", ", end='')
                    print(he_utility.getString(
                        self.session, parmSH_values[v]), end='')
            print(")")

        return True

    def getAttributes(self, node_id, part_id):
        '''Query and list the point, vertex, prim and detail attributes of the given node'''
        part_info = hapi.getPartInfo(self.session, node_id, part_id)

        vertex_attr_count = part_info.attributeCounts[hapi.attributeOwner.Vertex]
        point_attr_count = part_info.attributeCounts[hapi.attributeOwner.Point]
        prim_attr_count = part_info.attributeCounts[hapi.attributeOwner.Prim]
        detail_attr_count = part_info.attributeCounts[hapi.attributeOwner.Detail]

        print("\nAttributes: ")
        print("==========")

        # Point attributes
        point_attr_nameSH = hapi.getAttributeNames(
            self.session,
            node_id, part_id,
            hapi.attributeOwner.Point,
            point_attr_count
        )

        print("\n  Point Attributes: {}".format(point_attr_count))
        print("  ----------")
        for i in range(point_attr_count):
            attr_name = he_utility.getString(
                self.session, point_attr_nameSH[i])
            print("  Name: {}".format(attr_name))

            attr_info = hapi.getAttributeInfo(
                self.session,
                node_id, part_id,
                attr_name,
                hapi.attributeOwner.Point,
            )
            print("  Count: {} Storage type: {}".format(
                attr_info.count, attr_info.storage))

        # Vertex attributes
        vertex_attr_nameSH = hapi.getAttributeNames(
            self.session,
            node_id,
            part_id,
            hapi.attributeOwner.Vertex,
            vertex_attr_count
        )

        print("\n  Vertex Attributes: {}".format(vertex_attr_count))
        print("  ----------")
        for i in range(vertex_attr_count):
            attr_name = he_utility.getString(
                self.session, vertex_attr_nameSH[i])
            print("  Name: {}".format(attr_name))

            attr_info = hapi.getAttributeInfo(
                self.session,
                node_id, part_id,
                attr_name,
                hapi.attributeOwner.Vertex,
            )
            print("  Count: {} Storage type: {}".format(
                attr_info.count, attr_info.storage))

        # Primitive attributes
        prim_attr_nameSH = hapi.getAttributeNames(
            self.session,
            node_id, part_id,
            hapi.attributeOwner.Prim,
            prim_attr_count
        )

        print("\n  Primitive Attributes: {}".format(prim_attr_count))
        print("  ----------")
        for i in range(prim_attr_count):
            attr_name = he_utility.getString(
                self.session, prim_attr_nameSH[i])
            print("  Name: {}".format(attr_name))

            attr_info = hapi.getAttributeInfo(
                self.session,
                node_id, part_id,
                attr_name,
                hapi.attributeOwner.Prim,
            )
            print("  Count: {} Storage type: {}".format(
                attr_info.count, attr_info.storage))

        # Detail attributes
        detail_attr_nameSH = hapi.getAttributeNames(
            self.session,
            node_id, part_id,
            hapi.attributeOwner.Detail,
            detail_attr_count
        )

        print("\n  Detail Attributes: {}".format(detail_attr_count))
        print("  ----------")
        for i in range(detail_attr_count):
            attr_name = he_utility.getString(
                self.session, detail_attr_nameSH[i])
            print("  {}".format(attr_name))

        return True
