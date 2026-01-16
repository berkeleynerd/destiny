ccp_add_executable(DestinyTest)
find_package(GTest REQUIRED)
find_package(Tracy REQUIRED NO_CMAKE_PATH)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
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

if (WIN32)
    # Copy shared library dependencies to folder containing test executable.
    add_custom_command(TARGET DestinyTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Blue>
        $<TARGET_FILE_DIR:DestinyTest>
    )

    add_custom_command(TARGET DestinyTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:DestinyTest>
            $<TARGET_FILE_DIR:DestinyTest>
            COMMAND_EXPAND_LISTS
    )

    add_custom_command(TARGET DestinyTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Tracy::TracyClient>
            $<TARGET_FILE_DIR:DestinyTest>
    )
endif ()

set_target_properties(DestinyTest PROPERTIES FOLDER "Tests")
gtest_discover_tests(DestinyTest myListOfTests)

option(RUN_PYTHON_TESTS "As running the python tests relies on game-code, these are off by default" OFF)
if (RUN_PYTHON_TESTS)
    if(WIN32)
        # Discover and run Python tests as a single CTest test.
        set(PYTHON_INTERPRETER_PATH "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.bat")
    elseif(APPLE)
        set(PYTHON_INTERPRETER_PATH "${BRANCH_ROOT_DIR}/eve/client/pythonInterpreter.sh")
    endif()
    set(PYTHON_TEST PythonTests)
    add_test(NAME ${PYTHON_TEST}
            COMMAND ${PYTHON_INTERPRETER_PATH} -m unittest discover -v
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../python
            COMMAND_EXPAND_LISTS
    )
    set_tests_properties(${PYTHON_TEST} PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:destiny>;BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
endif()
