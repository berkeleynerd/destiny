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

set_target_properties(DestinyTest PROPERTIES FOLDER "Tests")
gtest_discover_tests(DestinyTest myListOfTests)

if(WIN32)
    # Discover and run Python tests as a single CTest test.
    set(PYTHON_INTERPRETER_PATH "${BRANCH_ROOT_DIR}/eve/server/pythonInterpreter.bat")
    add_test(NAME PythonTests
        COMMAND ${PYTHON_INTERPRETER_PATH} -m unittest discover -v    
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../python
        COMMAND_EXPAND_LISTS
    )
endif()

