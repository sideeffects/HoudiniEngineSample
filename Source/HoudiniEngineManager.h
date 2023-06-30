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

class HoudiniEngineManager
{
public:
	enum SessionType
	{
		InProcess = 1,
		NamedPipe = 2,
		TCPSocket = 3
	};

	HoudiniEngineManager();

	// Creates a new session
	bool startSession(SessionType session_type, bool connect_to_debugger, bool use_cooking_thread);
	
	// Stop the existing session if valid, and creates a new session
	bool restartSession(SessionType session_type, bool connect_to_debugger, bool use_cooking_thread);
	
	// Cleanup and shutdown an existing session
	bool stopSession();

	// Initializes the HAPI session, should be called after successfully creating a session
	bool initializeHAPI(bool use_cooking_thread);

	// Get the HAPI session
	HAPI_Session* getSession();

	// Get the cook options used to initialize the HAPI session
	HAPI_CookOptions* getCookOptions();

	// Load a new HDA asset
	bool loadAsset(const char* otl_path, HAPI_AssetLibraryId& asset_library_id, std::string& asset_name);

	// Instantiate and asynchronously cook the given node
	bool createAndCookNode(const char* operator_name, HAPI_NodeId * node_id);

	// Query and list the paramters of the given node
	bool getParameters(HAPI_NodeId node_id);

	// Query and list the point, vertex, prim and detail attributes of the given node
	bool getAttributes(HAPI_NodeId node_id, HAPI_PartId part_id);

private:
	// Wait for a cook to complete while querying its status
	bool waitForCook();

	HAPI_Session mySession;
	HAPI_CookOptions myCookOptions;
	SessionType mySessionType = InProcess;
};