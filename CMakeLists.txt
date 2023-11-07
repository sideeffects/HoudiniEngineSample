cmake_minimum_required( VERSION 3.1 )

project( HoudiniEngineSample )

IF(NOT DEFINED ENV{HFS})
    message(FATAL_ERROR "The HFS environment variable must be set to your "
                         "Houdini installation directory to build "
                         "the HoudiniEngineSample.")
ENDIF()

set( HE_SAMPLE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set( SOURCES
    ${HE_SAMPLE_ROOT}/Source/HoudiniApi.cpp
    ${HE_SAMPLE_ROOT}/Source/HoudiniEngineGeometry.cpp
    ${HE_SAMPLE_ROOT}/Source/HoudiniEngineManager.cpp
    ${HE_SAMPLE_ROOT}/Source/HoudiniEnginePlatform.cpp
    ${HE_SAMPLE_ROOT}/Source/HoudiniEngineSample.cpp
    ${HE_SAMPLE_ROOT}/Source/HoudiniEngineUtility.cpp
)

set( HEADERS
    ${HE_SAMPLE_ROOT}/Source/HoudiniApi.h
    ${HE_SAMPLE_ROOT}/Source/HoudiniEngineGeometry.h
    ${HE_SAMPLE_ROOT}/Source/HoudiniEngineManager.h
    ${HE_SAMPLE_ROOT}/Source/HoudiniEnginePlatform.h
    ${HE_SAMPLE_ROOT}/Source/HoudiniEngineUtility.h
)

set( FILES
    ${HE_SAMPLE_ROOT}/HDA/hexagona_lite.hda
)

add_executable( ${PROJECT_NAME} ${SOURCES} )

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( HOUDINI_HAPI_HEADERS "$ENV{HFS}/toolkit/include/" )

    # Setup rpath to locate libHAPIL.so at runtime
    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        INSTALL_RPATH "$ENV{HFS}/dsolib/"
    )
    target_link_libraries( ${PROJECT_NAME} ${CMAKE_DL_LIBS} )

elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" )
    set( HOUDINI_HAPI_HEADERS "$ENV{HFS}/toolkit/include/" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" )
   
     # Setup rpath to locate libHAPIL.dylib at runtime
     set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        MACOSX_RPATH TRUE
        INSTALL_RPATH "$ENV{HFS}/../Libraries/"
    )
    target_link_libraries( ${PROJECT_NAME} ${CMAKE_DL_LIBS} )

elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
    set( HOUDINI_HAPI_HEADERS "$ENV{HFS}/toolkit/include/" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17" )

    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES 
        VS_DEBUGGER_ENVIRONMENT "HFS=$ENV{HFS}"
    )
endif ()

# Include Houdini Engine headers
target_include_directories( ${PROJECT_NAME} PUBLIC ${HOUDINI_HAPI_HEADERS} )

# Install step
set( CMAKE_INSTALL_PREFIX "${HE_SAMPLE_ROOT}" )

install( TARGETS ${PROJECT_NAME} DESTINATION "bin" )
install( FILES ${FILES} DESTINATION "bin/HDA" )

# Set the HDA_INSTALL_PATH constant referenced in main()
add_definitions(-DHDA_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/bin/HDA")