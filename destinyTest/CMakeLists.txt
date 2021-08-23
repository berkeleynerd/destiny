add_executable(DestinyTest)
target_link_libraries(DestinyTest PRIVATE destinyStatic GTest::GTest GTest::Main)

target_sources(DestinyTest PRIVATE
    destinyTest.cpp
    StdAfx.cpp
    TestAABB.cpp
    TestBoxShape.cpp
    TestCollision.cpp
    TestPlane.cpp
    TestTriangle.cpp
    TestVector3d.cpp
)

target_precompile_headers(DestinyTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)
target_include_directories(DestinyTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(DestinyTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(DestinyTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../carbon/src)
if(APPLE)
    target_link_libraries(DestinyTest PRIVATE "-framework CoreFoundation")
endif()

set_target_properties(DestinyTest PROPERTIES FOLDER "Tests")