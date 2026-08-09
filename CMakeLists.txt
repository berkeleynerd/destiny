# Copyright © 2023 CCP ehf.

cmake_minimum_required(VERSION 3.24)
project(destiny VERSION 1.0.0)

option(BUILD_DESTINY_EMBEDDED "Build the static Destiny integration target." OFF)
option(BUILD_DESTINY_EMBEDDED_TESTING "Build the standalone embedded contract test." OFF)
option(DESTINY_INSTALL_EMBEDDED_ONLY "Install only the embedded Destiny package surface." OFF)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build and run tests. Enabled by default." ON)
    include(CTest)
endif()

if(DESTINY_INSTALL_EMBEDDED_ONLY AND NOT BUILD_DESTINY_EMBEDDED)
    message(FATAL_ERROR "DESTINY_INSTALL_EMBEDDED_ONLY requires BUILD_DESTINY_EMBEDDED")
endif()

set(DESTINY_EMBEDDED_PACKAGE_REPRODUCIBLE OFF)
if(DESTINY_INSTALL_EMBEDDED_ONLY)
    if(MSVC)
        message(STATUS
            "MSVC embedded-package reproducibility is disabled pending real-toolchain verification (issue #23)")
    else()
        set(DESTINY_EMBEDDED_PACKAGE_REPRODUCIBLE ON)
    endif()
endif()

if(DESTINY_EMBEDDED_PACKAGE_REPRODUCIBLE AND APPLE)
    # Apple archives need both deterministic object-member headers and a
    # reproducible table of contents. This mode is isolated to the embedded
    # install package and does not change normal module or Release builds.
    set(_DESTINY_SYSTEM_RANLIB "${CMAKE_RANLIB}")
    set(_DESTINY_REPRODUCIBLE_RANLIB "${CMAKE_CURRENT_BINARY_DIR}/destiny-reproducible-ranlib")
    file(WRITE "${_DESTINY_REPRODUCIBLE_RANLIB}"
        "#!/bin/sh\nexec \"${_DESTINY_SYSTEM_RANLIB}\" -D \"$@\"\n")
    file(CHMOD "${_DESTINY_REPRODUCIBLE_RANLIB}" PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
    set(CMAKE_RANLIB "${_DESTINY_REPRODUCIBLE_RANLIB}")
    set(CMAKE_CXX_ARCHIVE_CREATE
        "\"${CMAKE_COMMAND}\" -E env ZERO_AR_DATE=1 \"${CMAKE_AR}\" qc <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH "\"${CMAKE_RANLIB}\" <TARGET>")
endif()

find_package(carbon-blue CONFIG REQUIRED)
find_package(carbon-core CONFIG REQUIRED)
find_package(carbon-math CONFIG REQUIRED)

include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpDocsGenerator.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpMonolithVendorConfig.cmake)

ccp_add_library(destiny MODULE)
target_link_libraries(destiny PUBLIC Blue CcpCore PRIVATE CcpMath)

set(PUBLIC_HEADER_FILES_INCLUDE
    include/IEveReferencePoint.h
    include/IEveBallpark.h
)

if(BUILD_DESTINY_EMBEDDED)
    list(APPEND PUBLIC_HEADER_FILES_INCLUDE include/DestinyEmbedded.h)
endif()

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
    src/ParkStream.cpp
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(APPLE)
    # this is temporary so that we don't drown in warnings
    set_property(TARGET destiny APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-nonportable-include-path -Wno-inconsistent-missing-override")
endif()

if(BUILD_DESTINY_EMBEDDED)
    set(_EMBEDDED_SOURCES ${_SOURCES})
    list(REMOVE_ITEM _EMBEDDED_SOURCES
        src/Ball_Blue.cpp
        src/Ballpark_Blue.cpp
        src/Capsule_Blue.cpp
        src/destiny.cpp
        src/Miniball_Blue.cpp
        src/MiniBox_Blue.cpp
        src/MiniCapsule_Blue.cpp
        src/OrientedBox_Blue.cpp
        src/Partition_Blue.cpp
        src/Settings_Blue.cpp
        src/Thunkers.cpp
    )
    list(APPEND _EMBEDDED_SOURCES
        include/DestinyEmbedded.h
        src/DestinyEmbedded.cpp
        src/DestinyEmbeddedBlue.cpp
        src/DestinyEmbeddedInternal.h
    )

    ccp_add_library(destinyEmbedded STATIC)
    target_sources(destinyEmbedded PRIVATE ${_EMBEDDED_SOURCES})
    target_link_libraries(destinyEmbedded PUBLIC Blue CcpCore CcpMath)
    if(DESTINY_INSTALL_EMBEDDED_ONLY)
        set_property(TARGET destinyEmbedded PROPERTY INTERPROCEDURAL_OPTIMIZATION OFF)
        if(MSVC)
            target_compile_options(destinyEmbedded PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/StdAfx.h")
        else()
            # Keep the legacy forced-include contract without embedding a
            # compiler-specific PCH artifact in the reproducible package.
            target_compile_options(destinyEmbedded PRIVATE
                -include "${CMAKE_CURRENT_SOURCE_DIR}/src/StdAfx.h")
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" _DESTINY_SOURCE_REAL)
            file(REAL_PATH "${CMAKE_CURRENT_BINARY_DIR}" _DESTINY_BINARY_REAL)
            target_compile_options(destinyEmbedded PRIVATE
                "-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=."
                "-fdebug-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=."
                "-ffile-prefix-map=${_DESTINY_SOURCE_REAL}=."
                "-fdebug-prefix-map=${_DESTINY_SOURCE_REAL}=."
                "-ffile-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=.build"
                "-fdebug-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=.build"
                "-ffile-prefix-map=${_DESTINY_BINARY_REAL}=.build"
                "-fdebug-prefix-map=${_DESTINY_BINARY_REAL}=.build")
            if(VCPKG_INSTALLED_DIR)
                file(REAL_PATH "${VCPKG_INSTALLED_DIR}" _DESTINY_VCPKG_INSTALLED_REAL)
                target_compile_options(destinyEmbedded PRIVATE
                    "-ffile-prefix-map=${VCPKG_INSTALLED_DIR}=.vcpkg"
                    "-fdebug-prefix-map=${VCPKG_INSTALLED_DIR}=.vcpkg"
                    "-ffile-prefix-map=${_DESTINY_VCPKG_INSTALLED_REAL}=.vcpkg"
                    "-fdebug-prefix-map=${_DESTINY_VCPKG_INSTALLED_REAL}=.vcpkg")
            endif()
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(destinyEmbedded PRIVATE -fdebug-compilation-dir=.)
        endif()
    else()
        target_precompile_headers(destinyEmbedded PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/StdAfx.h)
    endif()
    target_include_directories(destinyEmbedded
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_compile_definitions(destinyEmbedded PRIVATE
        DESTINY_WITH_PYTHON=0
        DESTINY_EMBEDDED=1
        g_moduleName=g_destinyEmbeddedModuleName
    )
    if(APPLE)
        # D-03 symbol gate: fail the build itself if the archive imports any
        # CPython symbol beyond the pinned Blue-ABI whitelist.
        add_custom_command(TARGET destinyEmbedded POST_BUILD
            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/check-embedded-python-symbols.sh
                $<TARGET_FILE:destinyEmbedded>
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/embedded-python-symbol-whitelist.txt
            VERBATIM)
    endif()
    if(DESTINY_INSTALL_EMBEDDED_ONLY AND APPLE)
        # Apple's archiver writes a fresh table-of-contents header even when
        # ZERO_AR_DATE normalized every object member. A final reproducible
        # ranlib pass is required after all archive consumers have run.
        add_custom_command(TARGET destinyEmbedded POST_BUILD
            COMMAND "${CMAKE_RANLIB}" $<TARGET_FILE:destinyEmbedded>
            VERBATIM)
    endif()
    if(APPLE)
        set_property(TARGET destinyEmbedded APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-nonportable-include-path -Wno-inconsistent-missing-override")
    endif()

    if(BUILD_DESTINY_EMBEDDED_TESTING)
        ccp_add_executable(DestinyEmbeddedContractTest)
        target_sources(DestinyEmbeddedContractTest PRIVATE tests/DestinyEmbeddedContractTest.cpp)
        target_include_directories(DestinyEmbeddedContractTest PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
        target_link_libraries(DestinyEmbeddedContractTest PRIVATE
            "$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>"
        )
        if(APPLE)
            target_link_libraries(DestinyEmbeddedContractTest PRIVATE "-framework CoreFoundation")
        endif()
        if(BUILD_TESTING)
            add_test(NAME DestinyEmbeddedContract COMMAND DestinyEmbeddedContractTest)
        endif()

		ccp_add_executable(DestinyEmbeddedMotionContractTest)
		target_sources(DestinyEmbeddedMotionContractTest PRIVATE tests/DestinyEmbeddedMotionContractTest.cpp)
		target_include_directories(DestinyEmbeddedMotionContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src
		)
		target_compile_definitions(DestinyEmbeddedMotionContractTest PRIVATE
			DESTINY_MOTION_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
		)
		target_link_libraries(DestinyEmbeddedMotionContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>"
		)
		if(APPLE)
			target_link_libraries(DestinyEmbeddedMotionContractTest PRIVATE "-framework CoreFoundation")
		endif()
			if(BUILD_TESTING)
				add_test(NAME DestinyEmbeddedMotionContract COMMAND DestinyEmbeddedMotionContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedOrbitContractTest)
		target_sources(DestinyEmbeddedOrbitContractTest PRIVATE tests/DestinyEmbeddedOrbitContractTest.cpp)
		target_include_directories(DestinyEmbeddedOrbitContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_compile_definitions(DestinyEmbeddedOrbitContractTest PRIVATE
			DESTINY_MOTION_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
		target_link_libraries(DestinyEmbeddedOrbitContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedOrbitContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedOrbitContract COMMAND DestinyEmbeddedOrbitContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedWarpContractTest)
		target_sources(DestinyEmbeddedWarpContractTest PRIVATE tests/DestinyEmbeddedWarpContractTest.cpp)
		target_include_directories(DestinyEmbeddedWarpContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_compile_definitions(DestinyEmbeddedWarpContractTest PRIVATE
			DESTINY_MOTION_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
		target_link_libraries(DestinyEmbeddedWarpContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedWarpContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedWarpContract COMMAND DestinyEmbeddedWarpContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedApproachContractTest)
		target_sources(DestinyEmbeddedApproachContractTest PRIVATE tests/DestinyEmbeddedApproachContractTest.cpp)
		target_include_directories(DestinyEmbeddedApproachContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_compile_definitions(DestinyEmbeddedApproachContractTest PRIVATE
			DESTINY_MOTION_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
		target_link_libraries(DestinyEmbeddedApproachContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedApproachContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedApproachContract COMMAND DestinyEmbeddedApproachContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedCelestialContractTest)
		target_sources(DestinyEmbeddedCelestialContractTest PRIVATE tests/DestinyEmbeddedCelestialContractTest.cpp)
		target_include_directories(DestinyEmbeddedCelestialContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_compile_definitions(DestinyEmbeddedCelestialContractTest PRIVATE
			DESTINY_MOTION_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
		target_link_libraries(DestinyEmbeddedCelestialContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedCelestialContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedCelestialContract COMMAND DestinyEmbeddedCelestialContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedRecorderTest)
		target_sources(DestinyEmbeddedRecorderTest PRIVATE tests/DestinyEmbeddedRecorderTest.cpp)
		target_include_directories(DestinyEmbeddedRecorderTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_link_libraries(DestinyEmbeddedRecorderTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedRecorderTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedRecorder COMMAND DestinyEmbeddedRecorderTest)
		endif()

		ccp_add_executable(DestinyEmbeddedSeamTest)
		target_sources(DestinyEmbeddedSeamTest PRIVATE tests/DestinyEmbeddedSeamTest.cpp)
		target_include_directories(DestinyEmbeddedSeamTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_link_libraries(DestinyEmbeddedSeamTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedSeamTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedSeam COMMAND DestinyEmbeddedSeamTest)
		endif()

		ccp_add_executable(DestinyEmbeddedFullStateContractTest)
		target_sources(DestinyEmbeddedFullStateContractTest PRIVATE
			tests/DestinyEmbeddedFullStateContractTest.cpp)
		target_include_directories(DestinyEmbeddedFullStateContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_link_libraries(DestinyEmbeddedFullStateContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedFullStateContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedFullStateContract COMMAND DestinyEmbeddedFullStateContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedMultiBallContractTest)
		target_sources(DestinyEmbeddedMultiBallContractTest PRIVATE
			tests/DestinyEmbeddedMultiBallContractTest.cpp)
		target_include_directories(DestinyEmbeddedMultiBallContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_link_libraries(DestinyEmbeddedMultiBallContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedMultiBallContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedMultiBallContract COMMAND DestinyEmbeddedMultiBallContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedMissileContractTest)
		target_sources(DestinyEmbeddedMissileContractTest PRIVATE
			tests/DestinyEmbeddedMissileContractTest.cpp)
		target_include_directories(DestinyEmbeddedMissileContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_link_libraries(DestinyEmbeddedMissileContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedMissileContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedMissileContract COMMAND DestinyEmbeddedMissileContractTest)
		endif()

		ccp_add_executable(DestinyEmbeddedLegacyOptionsContractTest)
		target_sources(DestinyEmbeddedLegacyOptionsContractTest PRIVATE
			tests/DestinyEmbeddedLegacyOptionsContractTest.cpp)
		target_include_directories(DestinyEmbeddedLegacyOptionsContractTest PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/src)
		target_link_libraries(DestinyEmbeddedLegacyOptionsContractTest PRIVATE
			"$<LINK_LIBRARY:WHOLE_ARCHIVE,destinyEmbedded>")
		if(APPLE)
			target_link_libraries(DestinyEmbeddedLegacyOptionsContractTest PRIVATE "-framework CoreFoundation")
		endif()
		if(BUILD_TESTING)
			add_test(NAME DestinyEmbeddedLegacyOptionsContract COMMAND DestinyEmbeddedLegacyOptionsContractTest)
		endif()

		if(BUILD_TESTING AND APPLE)
			add_test(NAME DestinyEmbeddedSymbolGate
				COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/check-embedded-python-symbols.sh
					$<TARGET_FILE:destinyEmbedded>
					${CMAKE_CURRENT_SOURCE_DIR}/tests/data/embedded-python-symbol-whitelist.txt)
		endif()

			if(BUILD_TESTING AND NOT MSVC)
				string(REPLACE ";" "\\;" _DESTINY_VCPKG_OVERLAY_TRIPLETS
					"${VCPKG_OVERLAY_TRIPLETS}")
				string(REPLACE ";" "\\;" _DESTINY_OSX_ARCHITECTURES
					"${CMAKE_OSX_ARCHITECTURES}")
				string(REPLACE ";" "\\;" _DESTINY_CMAKE_PREFIX_PATH
				"${CMAKE_PREFIX_PATH}")
			string(REPLACE ";" "\\;" _DESTINY_CMAKE_FIND_ROOT_PATH
				"${CMAKE_FIND_ROOT_PATH}")
			add_test(NAME DestinyEmbeddedPackageReproducibility
				COMMAND ${CMAKE_COMMAND}
					-DDESTINY_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
					-DDESTINY_TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/embedded-package-reproducibility
					-DDESTINY_GENERATOR=${CMAKE_GENERATOR}
					-DDESTINY_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
					-DDESTINY_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}
					-DDESTINY_HOST_TRIPLET=${VCPKG_HOST_TRIPLET}
					-DDESTINY_VCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}
						"-DDESTINY_VCPKG_OVERLAY_TRIPLETS=${_DESTINY_VCPKG_OVERLAY_TRIPLETS}"
					-DDESTINY_VCPKG_CHAINLOAD_TOOLCHAIN_FILE=${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}
					-DDESTINY_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
						"-DDESTINY_OSX_ARCHITECTURES=${_DESTINY_OSX_ARCHITECTURES}"
					-DDESTINY_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
					-DDESTINY_CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
					-DDESTINY_CMAKE_CXX_STANDARD_REQUIRED=${CMAKE_CXX_STANDARD_REQUIRED}
					-DDESTINY_CMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
					"-DDESTINY_CMAKE_PREFIX_PATH=${_DESTINY_CMAKE_PREFIX_PATH}"
					"-DDESTINY_CMAKE_FIND_ROOT_PATH=${_DESTINY_CMAKE_FIND_ROOT_PATH}"
					-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/VerifyEmbeddedPackageReproducibility.cmake)
			set_tests_properties(DestinyEmbeddedPackageReproducibility PROPERTIES
				RUN_SERIAL TRUE
				TIMEOUT 1200)
		elseif(BUILD_TESTING AND MSVC)
			message(STATUS
				"Skipping embedded-package reproducibility test pending real MSVC verification (issue #23)")
		endif()

		# Embedded test translation units must see the exact ABI of the
		# destinyEmbedded archive they link: MapOfBalls and the Ballpark
		# bubble bookkeeping change layout under these flags.
		foreach(_embeddedTest IN ITEMS
			DestinyEmbeddedContractTest
			DestinyEmbeddedMotionContractTest
			DestinyEmbeddedOrbitContractTest
			DestinyEmbeddedWarpContractTest
			DestinyEmbeddedApproachContractTest
			DestinyEmbeddedCelestialContractTest
			DestinyEmbeddedSeamTest
			DestinyEmbeddedRecorderTest
			DestinyEmbeddedFullStateContractTest
			DestinyEmbeddedMultiBallContractTest
			DestinyEmbeddedMissileContractTest
			DestinyEmbeddedLegacyOptionsContractTest)
			target_compile_definitions(${_embeddedTest} PRIVATE
				DESTINY_WITH_PYTHON=0
				DESTINY_EMBEDDED=1)
		endforeach()
    endif()
endif()

set_target_properties(destiny PROPERTIES OUTPUT_NAME "_destiny")

ccp_add_library(destinyStatic STATIC)
target_link_libraries(destinyStatic PUBLIC Blue CcpCore CcpMath)
target_sources(destinyStatic PRIVATE ${_SOURCES})
target_precompile_headers(destinyStatic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/StdAfx.h)
target_include_directories(destinyStatic
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(APPLE)
    # this is temporary so that we don't drown in warnings
    set_property(TARGET destinyStatic APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-nonportable-include-path -Wno-inconsistent-missing-override")
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if(BUILD_TESTING)
        find_package(carbon-exefile-interpreter REQUIRED CONFIG)
        # Interpreter must be created in this file, since it creates a POST_BUILD target on destiny.
        ccp_prepare_interpreter(
            TARGET destiny
            BUILD_TYPE ${CMAKE_BUILD_TYPE}
            COMMAND_OUT EXEFILE_INTERPRETER
            PYTHONPATH_OUT PYTHONPATH
            RUN_DIR $<TARGET_FILE_DIR:destiny>
            BLUE_DIR_OUT BLUE_LOCATION_DIR
        )
	    add_subdirectory(tests)
    endif()
endif()

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)
    if(INSTALL_TO_MONOLITH)
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
    else()
        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        if(DESTINY_INSTALL_EMBEDDED_ONLY)
            set(_DESTINY_INSTALL_TARGETS destinyEmbedded)
        else()
            set(_DESTINY_INSTALL_TARGETS destiny)
        endif()
        if(BUILD_DESTINY_EMBEDDED AND NOT DESTINY_INSTALL_EMBEDDED_ONLY)
            list(APPEND _DESTINY_INSTALL_TARGETS destinyEmbedded)
        endif()
        install(
                TARGETS ${_DESTINY_INSTALL_TARGETS}
                EXPORT DestinyTargets
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                ARCHIVE DESTINATION lib/
                LIBRARY DESTINATION lib/
                RUNTIME DESTINATION bin/
        )

        install(EXPORT DestinyTargets
                DESTINATION share/carbon-destiny
                FILE carbon-destiny.cmake
        )

        install(FILES carbon-destinyConfig.cmake DESTINATION share/carbon-destiny)

        # Install rule for available public headers
        install(FILES ${PUBLIC_HEADER_FILES_INCLUDE} DESTINATION include)

        # Install python package
        if(NOT DESTINY_INSTALL_EMBEDDED_ONLY)
            install(DIRECTORY python/destiny DESTINATION bin/python
                    FILES_MATCHING
                    PATTERN "*.py"
                    PATTERN "package.yaml"
            )
        endif()
    endif()
endif()
