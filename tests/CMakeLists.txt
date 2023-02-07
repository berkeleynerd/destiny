ccp_add_executable(DestinyTest)
find_package(GTest REQUIRED)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
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
target_include_directories(DestinyTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_include_directories(DestinyTest PRIVATE ${BRANCH_ROOT_DIR}/carbon/src/trinity/Include)

if(APPLE)
    target_link_libraries(DestinyTest PRIVATE "-framework CoreFoundation")
endif()

# Copy shared library dependencies to folder containing test executable.
if(WIN32)
add_custom_command(TARGET DestinyTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${BRANCH_ROOT_DIR}/vendor/python/2.7.1+ccp-stackless/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/python27.dll"
    ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)
add_custom_command(TARGET DestinyTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${BRANCH_ROOT_DIR}/vendor/CCP/Blue/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<IF:$<CONFIG:Release>,blue.dll,blue_$<LOWER_CASE:$<CONFIG>>.dll>"
    ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)
elseif(APPLE)
add_custom_command(TARGET DestinyTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${BRANCH_ROOT_DIR}/vendor/python/2.7.1+ccp-stackless/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/libpython2.7.dylib"
    ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)
add_custom_command(TARGET DestinyTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${BRANCH_ROOT_DIR}/vendor/CCP/Blue/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<IF:$<CONFIG:Release>,blue.so,blue_$<LOWER_CASE:$<CONFIG>>.so>"
    ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)
endif()

set_target_properties(DestinyTest PROPERTIES FOLDER "Tests")
gtest_discover_tests(DestinyTest myListOfTests)
