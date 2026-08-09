# Copyright (c) 2026 CCP Games

foreach(_required IN ITEMS
    DESTINY_SOURCE_DIR
    DESTINY_TEST_ROOT
    DESTINY_GENERATOR)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

set(_DESTINY_CONFIGURE_PASSTHROUGH "")
foreach(_mapping IN ITEMS
    "DESTINY_TOOLCHAIN_FILE|CMAKE_TOOLCHAIN_FILE"
    "DESTINY_TARGET_TRIPLET|VCPKG_TARGET_TRIPLET"
    "DESTINY_HOST_TRIPLET|VCPKG_HOST_TRIPLET"
    "DESTINY_VCPKG_INSTALLED_DIR|VCPKG_INSTALLED_DIR"
    "DESTINY_VCPKG_OVERLAY_TRIPLETS|VCPKG_OVERLAY_TRIPLETS"
    "DESTINY_VCPKG_CHAINLOAD_TOOLCHAIN_FILE|VCPKG_CHAINLOAD_TOOLCHAIN_FILE"
    "DESTINY_MAKE_PROGRAM|CMAKE_MAKE_PROGRAM"
    "DESTINY_OSX_ARCHITECTURES|CMAKE_OSX_ARCHITECTURES"
    "DESTINY_OSX_DEPLOYMENT_TARGET|CMAKE_OSX_DEPLOYMENT_TARGET"
    "DESTINY_CMAKE_CXX_STANDARD|CMAKE_CXX_STANDARD"
    "DESTINY_CMAKE_CXX_STANDARD_REQUIRED|CMAKE_CXX_STANDARD_REQUIRED"
    "DESTINY_CMAKE_CXX_EXTENSIONS|CMAKE_CXX_EXTENSIONS"
    "DESTINY_CMAKE_PREFIX_PATH|CMAKE_PREFIX_PATH"
    "DESTINY_CMAKE_FIND_ROOT_PATH|CMAKE_FIND_ROOT_PATH")
    string(REPLACE "|" ";" _mapping_parts "${_mapping}")
    list(GET _mapping_parts 0 _source_variable)
    list(GET _mapping_parts 1 _cache_variable)
    if(DEFINED ${_source_variable} AND NOT "${${_source_variable}}" STREQUAL "")
        list(APPEND _DESTINY_CONFIGURE_PASSTHROUGH
            "-D${_cache_variable}=${${_source_variable}}")
    endif()
endforeach()

function(_destiny_run)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE _result
        COMMAND_ECHO STDOUT)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Command failed with exit code ${_result}: ${ARGN}")
    endif()
endfunction()

function(_destiny_configure_and_install _name)
    set(_build "${DESTINY_TEST_ROOT}/${_name}-build")
    set(_install "${DESTINY_TEST_ROOT}/${_name}-install")
    _destiny_run(
        "${CMAKE_COMMAND}"
        -S "${DESTINY_SOURCE_DIR}"
        -B "${_build}"
        -G "${DESTINY_GENERATOR}"
        ${_DESTINY_CONFIGURE_PASSTHROUGH}
        -DBUILD_DESTINY_EMBEDDED=ON
        -DBUILD_DESTINY_EMBEDDED_TESTING=OFF
        -DBUILD_TESTING=OFF
        -DDESTINY_INSTALL_EMBEDDED_ONLY=ON
        -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON)
    _destiny_run("${CMAKE_COMMAND}" --build "${_build}" --config Debug --target destinyEmbedded)
    _destiny_run("${CMAKE_COMMAND}" --install "${_build}" --config Debug --prefix "${_install}")
    set(${_name}_BUILD "${_build}" PARENT_SCOPE)
    set(${_name}_INSTALL "${_install}" PARENT_SCOPE)
endfunction()

function(_destiny_manifest _prefix _manifest_out _hash_out)
    file(GLOB_RECURSE _files LIST_DIRECTORIES FALSE RELATIVE "${_prefix}" "${_prefix}/*")
    list(SORT _files)
    list(LENGTH _files _file_count)
    if(NOT _file_count EQUAL 7)
        message(FATAL_ERROR "Expected seven installed files in ${_prefix}; found ${_file_count}: ${_files}")
    endif()
    set(_manifest "")
    foreach(_relative IN LISTS _files)
        file(SHA256 "${_prefix}/${_relative}" _file_hash)
        string(APPEND _manifest "${_file_hash}  ${_relative}\n")
    endforeach()
    string(SHA256 _aggregate "${_manifest}")
    set(${_manifest_out} "${_manifest}" PARENT_SCOPE)
    set(${_hash_out} "${_aggregate}" PARENT_SCOPE)
endfunction()

function(_destiny_compare_packages _left _right)
    _destiny_manifest("${_left}" _left_manifest _left_hash)
    _destiny_manifest("${_right}" _right_manifest _right_hash)
    if(NOT _left_manifest STREQUAL _right_manifest)
        message(FATAL_ERROR
            "Installed packages differ:\nLEFT (${_left_hash})\n${_left_manifest}RIGHT (${_right_hash})\n${_right_manifest}")
    endif()
    set(DESTINY_PACKAGE_HASH "${_left_hash}" PARENT_SCOPE)
endfunction()

function(_destiny_run_consumer _prefix)
    set(_consumer_build "${DESTINY_TEST_ROOT}/consumer-build")
    _destiny_run(
        "${CMAKE_COMMAND}"
        -S "${DESTINY_SOURCE_DIR}/tests/installed-package-consumer"
        -B "${_consumer_build}"
        -G "${DESTINY_GENERATOR}"
        ${_DESTINY_CONFIGURE_PASSTHROUGH}
        -DCMAKE_CONFIGURATION_TYPES=Debug
        "-Dcarbon-destiny_DIR=${_prefix}/share/carbon-destiny")
    _destiny_run("${CMAKE_COMMAND}" --build "${_consumer_build}" --config Debug)
    _destiny_run("${_consumer_build}/bin/DestinyEmbeddedPackageConsumer")
endfunction()

file(REMOVE_RECURSE "${DESTINY_TEST_ROOT}")
file(MAKE_DIRECTORY "${DESTINY_TEST_ROOT}")

_destiny_configure_and_install(root_a)
_destiny_configure_and_install(root_b)
_destiny_compare_packages("${root_a_INSTALL}" "${root_b_INSTALL}")

_destiny_run("${CMAKE_COMMAND}" --build "${root_a_BUILD}" --target clean)
_destiny_run("${CMAKE_COMMAND}" --build "${root_a_BUILD}" --config Debug --target destinyEmbedded)
set(_clean_install "${DESTINY_TEST_ROOT}/root-a-clean-install")
_destiny_run("${CMAKE_COMMAND}" --install "${root_a_BUILD}" --config Debug --prefix "${_clean_install}")
_destiny_compare_packages("${root_a_INSTALL}" "${_clean_install}")
_destiny_run_consumer("${_clean_install}")

message(STATUS "Destiny embedded package reproducible: files=7 aggregate=${DESTINY_PACKAGE_HASH}")
