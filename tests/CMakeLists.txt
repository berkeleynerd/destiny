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

    add_custom_command(TARGET DestinyTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:destiny>
            $<TARGET_FILE_DIR:DestinyTest>
    )
endif ()

set_target_properties(DestinyTest PROPERTIES FOLDER "Tests")
gtest_discover_tests(DestinyTest myListOfTests)

option(RUN_PYTHON_TESTS "As running the python tests relies on game-code, these are off by default" OFF)
if (RUN_PYTHON_TESTS)
    if (DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "Release")
        set(EXEFILE_TOOL_NAME exefile_${CMAKE_BUILD_TYPE})
    else()
        set(EXEFILE_TOOL_NAME exefile)
    endif()
    find_program(exefile_tool ${EXEFILE_TOOL_NAME} NO_CACHE)
    if (WIN32)
        add_custom_command(TARGET DestinyTest POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${exefile_tool} $<TARGET_FILE_DIR:DestinyTest>
        )
        set(exefile_tool  $<TARGET_FILE_DIR:DestinyTest>/${EXEFILE_TOOL_NAME})
    endif ()
    add_test(NAME PythonTests
            COMMAND ${exefile_tool} /inherit /buildflavor=$<LOWER_CASE:$<CONFIG>> /py -m unittest discover -v ${CMAKE_CURRENT_SOURCE_DIR}/../python/destiny/test
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../python/destiny/test
            COMMAND_EXPAND_LISTS
    )
    set_tests_properties(PythonTests PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../python\;$<TARGET_FILE_DIR:DestinyTest>\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib\;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/python\;${Python3_STDLIB};BUILDFLAVOR=$<LOWER_CASE:$<CONFIG>>")
endif()
