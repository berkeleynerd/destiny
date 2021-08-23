project(destiny VERSION 1.0.0)

add_library(destiny SHARED)
target_link_libraries(destiny PUBLIC Blue CcpCore PRIVATE CcpMath)

set(_SOURCES
    AABB.cpp
    AABB.h
    autoversion.h
    Ball.cpp
    Ball.h
    Ball_Blue.cpp
    Ballpark.cpp
    Ballpark.h
    Ballpark_Blue.cpp
    Box.cpp
    Box.h
    BoxShape.cpp
    BoxShape.h
    Capsule.cpp
    Capsule.h
    Capsule_Blue.cpp
    Collision.cpp
    Collision.h
    destiny.cpp
    DstConstants.cpp
    DstConstants.h
    MapOfBalls.h
    MapOfLongLongs.h
    MiniBall.cpp
    MiniBall.h
    Miniball_Blue.cpp
    MiniBox.cpp
    MiniBox.h
    MiniBox_Blue.cpp
    MiniCapsule.cpp
    MiniCapsule.h
    MiniCapsule_Blue.cpp
    OrientedBox.cpp
    OrientedBox.h
    OrientedBox_Blue.cpp
    Partition.cpp
    Partition.h
    Partitionable.cpp
    Partitionable.h
    Plane.cpp
    Plane.h
    Random.cpp
    Random.h
    resource.h
    SortedSets.h
    StaticCollidable.cpp
    StaticCollidable.h
    StdAfx.cpp
    Thunkers.cpp
    Triangle.cpp
    Triangle.h
    Vector3d.h
    version.h
)
target_sources(destiny PRIVATE ${_SOURCES})

target_precompile_headers(destiny PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
target_include_directories(destiny PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(destiny PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../carbon/src)
if(APPLE)
    # this is temporary so that we don't drown in warnings
    set_property(TARGET destiny APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-nonportable-include-path -Wno-inconsistent-missing-override")
endif()

if(APPLE)
    set_target_properties(
        destiny
        PROPERTIES 
            PREFIX ""
            SUFFIX ".so"
    )
endif()
set_target_properties(destiny PROPERTIES OUTPUT_NAME "_destiny")

add_library(destinyStatic STATIC)
target_link_libraries(destinyStatic PUBLIC Blue CcpCore CcpMath)
target_sources(destinyStatic PRIVATE ${_SOURCES})
target_precompile_headers(destinyStatic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
target_include_directories(destinyStatic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(destinyStatic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../carbon/src)
if(APPLE)
    # this is temporary so that we don't drown in warnings
    set_property(TARGET destinyStatic APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-nonportable-include-path -Wno-inconsistent-missing-override")
endif()
