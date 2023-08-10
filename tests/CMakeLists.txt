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
        "${Python_ROOT}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/python38.dll"
        $<TARGET_FILE_DIR:DestinyTest>
    )
    add_custom_command(TARGET DestinyTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Blue_DIR}/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<IF:$<CONFIG:Release>,blue.pyd,blue_$<LOWER_CASE:$<CONFIG>>.pyd>"
        $<TARGET_FILE_DIR:DestinyTest>
    )
elseif(APPLE)
    add_custom_command(TARGET DestinyTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Python_ROOT}/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/libpython2.7.dylib"
        $<TARGET_FILE_DIR:DestinyTest>
    )
    add_custom_command(TARGET DestinyTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Blue_DIR}/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<IF:$<CONFIG:Release>,blue.so,blue_$<LOWER_CASE:$<CONFIG>>.so>"
        $<TARGET_FILE_DIR:DestinyTest>
    )
endif()

set_target_properties(DestinyTest PROPERTIES FOLDER "Tests")
gtest_discover_tests(DestinyTest myListOfTests)

if(WIN32)
    # Discover and run Python tests as a single CTest test.
    set(PYTHON_INTERPRETER_PATH "${BRANCH_ROOT_DIR}/eve/server/pythonInterpreter.bat")
    set(PYTHON_TEST PythonTests)
    add_test(NAME ${PYTHON_TEST}
        COMMAND ${PYTHON_INTERPRETER_PATH} -m unittest discover -v
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../python
        COMMAND_EXPAND_LISTS
    )
    set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:destiny>")
endif()

