cmake_minimum_required( VERSION 3.1 )

project( HoudiniEngineSample )

IF(NOT DEFINED ENV{HOUDINI_HAPI_LIB})
    message(FATAL_ERROR "The HOUDINI_HAPI_LIB environment variable must be "
                         "set to build the HoudiniEngineSample.")
ENDIF()

IF(NOT DEFINED ENV{HOUDINI_HAPI_HEADERS})
    message(FATAL_ERROR "The HOUDINI_HAPI_LIB environment variable must be "
                        "set to build the HoudiniEngineSample.")
ENDIF()

set( HE_SAMPLE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set( SOURCES
    ${HE_SAMPLE_ROOT}/HoudiniEngineGeometry.cpp
    ${HE_SAMPLE_ROOT}/HoudiniEngineManager.cpp
    ${HE_SAMPLE_ROOT}/HoudiniEngineSample.cpp
    ${HE_SAMPLE_ROOT}/HoudiniEngineUtility.cpp
)

set( HEADERS
    ${HE_SAMPLE_ROOT}/HoudiniEngineGeometry.h
    ${HE_SAMPLE_ROOT}/HoudiniEngineManager.h
    ${HE_SAMPLE_ROOT}/HoudiniEngineUtility.h
)

set ( FILES
    ${HE_SAMPLE_ROOT}/HDA/hexagona_lite.hda
)

add_executable( ${PROJECT_NAME} ${SOURCES} )

# Link against the import library libHAPIL.lib
target_link_libraries( ${PROJECT_NAME} PUBLIC $ENV{HOUDINI_HAPI_LIB} )

# Include Houdini Engine headers
target_include_directories( ${PROJECT_NAME} PUBLIC $ENV{HOUDINI_HAPI_HEADERS} )

set( CMAKE_INSTALL_PREFIX "${HE_SAMPLE_ROOT}" )

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    # Setup rpath
    get_filename_component( HAPI_LIB_DIR ${HOUDINI_HAPI_LIB} DIRECTORY )
    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        INSTALL_RPATH "$ENV{HAPI_LIB_DIR}"
    )
elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" )

    # Setup rpath
    get_filename_component( HAPI_LIB_DIR ${HOUDINI_HAPI_LIB} DIRECTORY )
    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        MACOSX_RPATH TRUE
        INSTALL_RPATH "$ENV{HOUDINI_HAPI_LIB}"
    )
endif ()

install( TARGETS ${PROJECT_NAME} DESTINATION "bin" )
install( FILES ${FILES} DESTINATION "bin/HDA" )
