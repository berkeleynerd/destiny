cmake_minimum_required(VERSION 3.16)
project(destiny VERSION 1.0.0)

if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
else()
    message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
endif()

file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
include(CcpGlobalSettings)

find_package(Blue REQUIRED NO_CMAKE_PATH)
find_package(CcpCore REQUIRED NO_CMAKE_PATH)
find_package(CcpMath REQUIRED NO_CMAKE_PATH)

ccp_add_library(destiny SHARED)
target_link_libraries(destiny PUBLIC Blue CcpCore PRIVATE CcpMath)

set(PUBLIC_HEADER_FILES_INCLUDE
    include/IEveReferencePoint.h
    include/IEveBallpark.h
)

set(_SOURCES
    src/AABB.cpp
    src/AABB.h
    src/autoversion.h
    src/Ball.cpp
    src/Ball.h
    src/Ball_Blue.cpp
    src/Ballpark.cpp
    src/Ballpark.h
    src/Ballpark_Blue.cpp
    src/Box.cpp
    src/Box.h
    src/BoxShape.cpp
    src/BoxShape.h
    src/Capsule.cpp
    src/Capsule.h
    src/Capsule_Blue.cpp
    src/Collision.cpp
    src/Collision.h
    src/CollisionStructures.cpp
    src/CollisionStructures.h
    src/CollisionBallProperties.cpp
    src/CollisionBallProperties.h
    src/destiny.cpp
    src/DstConstants.h
    src/MapOfBalls.h
    src/MapOfLongLongs.h
    src/MiniBall.cpp
    src/MiniBall.h
    src/Miniball_Blue.cpp
    src/MiniBox.cpp
    src/MiniBox.h
    src/MiniBox_Blue.cpp
    src/MiniCapsule.cpp
    src/MiniCapsule.h
    src/MiniCapsule_Blue.cpp
    src/OrientedBox.cpp
    src/OrientedBox.h
    src/OrientedBox_Blue.cpp
    src/Partition.cpp
    src/Partition.h
    src/Partition_Blue.cpp
    src/Partitionable.cpp
    src/Partitionable.h
    src/Plane.cpp
    src/Plane.h
    src/Random.cpp
    src/Random.h
    src/resource.h
    src/Settings.h
    src/Settings.cpp
    src/Settings_Blue.cpp
    src/SortedSets.h
    src/StaticCollidable.cpp
    src/StaticCollidable.h
    src/StdAfx.cpp
    src/Thunkers.cpp
    src/Triangle.cpp
    src/Triangle.h
    src/Vector3d.h
    src/version.h
    ${PUBLIC_HEADER_FILES_INCLUDE}
)
target_sources(destiny PRIVATE ${_SOURCES})

target_precompile_headers(destiny PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/StdAfx.h)
target_include_directories(destiny
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(APPLE)
    # this is temporary so that we don't drown in warnings
    set_property(TARGET destiny APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-nonportable-include-path -Wno-inconsistent-missing-override")
endif()

set_target_properties(destiny PROPERTIES OUTPUT_NAME "_destiny")

ccp_add_library(destinyStatic STATIC)
target_link_libraries(destinyStatic PUBLIC Blue CcpCore CcpMath)
target_sources(destinyStatic PRIVATE ${_SOURCES})
target_precompile_headers(destinyStatic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/StdAfx.h)
target_include_directories(destinyStatic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(APPLE)
    # this is temporary so that we don't drown in warnings
    set_property(TARGET destinyStatic APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-nonportable-include-path -Wno-inconsistent-missing-override")
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build and run tests. Enabled by default." ON)
    include(CTest)
    if(BUILD_TESTING)
	    add_subdirectory(tests)
    endif()
endif()

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
        TARGETS destiny
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    # Install rule for available public headers
    install(FILES ${PUBLIC_HEADER_FILES_INCLUDE} DESTINATION include)

    # Install a custom DestinyConfig.cmake file, since as a dependency we are
    # basically a header-only library, not something that gets linked against.
    install(FILES ${PROJECT_SOURCE_DIR}/cmake/DestinyConfig.cmake DESTINATION .)

    # Install python package
    install(DIRECTORY python/ DESTINATION python
            FILES_MATCHING
            PATTERN "*.py"
            PATTERN "package.yaml"
    )
endif()
