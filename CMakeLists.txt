cmake_minimum_required(VERSION 3.20)

project(cmsUtils LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(CMS_ENABLE_ADDRESS_SANITIZER "Enable AddressSanitizer" OFF)
option(CMS_ENABLE_UNDEFINED_SANITIZER "Enable UndefinedBehaviorSanitizer" OFF)

function(cms_configure_target target_name)
    set_target_properties(${target_name} PROPERTIES
        CXX_EXTENSIONS OFF
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /source-charset:utf-8
        )

        if(CMS_ENABLE_ADDRESS_SANITIZER)
            target_compile_options(${target_name} PRIVATE /fsanitize=address)
            target_link_options(${target_name} PRIVATE /fsanitize=address)
        endif()

        if(CMS_ENABLE_UNDEFINED_SANITIZER)
            message(FATAL_ERROR
                "UndefinedBehaviorSanitizer is not supported by the MSVC toolchain")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )

        if(CMS_ENABLE_ADDRESS_SANITIZER)
            target_compile_options(${target_name} PRIVATE
                -fsanitize=address
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE -fsanitize=address)
        endif()

        if(CMS_ENABLE_UNDEFINED_SANITIZER)
            target_compile_options(${target_name} PRIVATE
                -fsanitize=undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE -fsanitize=undefined)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )

        if(CMS_ENABLE_ADDRESS_SANITIZER)
            target_compile_options(${target_name} PRIVATE
                -fsanitize=address
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE -fsanitize=address)
        endif()

        if(CMS_ENABLE_UNDEFINED_SANITIZER)
            target_compile_options(${target_name} PRIVATE
                -fsanitize=undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE -fsanitize=undefined)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )

        if(CMS_ENABLE_ADDRESS_SANITIZER)
            target_compile_options(${target_name} PRIVATE
                -fsanitize=address
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE -fsanitize=address)
        endif()

        if(CMS_ENABLE_UNDEFINED_SANITIZER)
            target_compile_options(${target_name} PRIVATE
                -fsanitize=undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE -fsanitize=undefined)
        endif()
    else()
        if(CMS_ENABLE_ADDRESS_SANITIZER OR CMS_ENABLE_UNDEFINED_SANITIZER)
            message(FATAL_ERROR
                "Sanitizers are not configured for compiler: "
                "${CMAKE_CXX_COMPILER_ID}")
        endif()

        message(STATUS
            "No project warning flags configured for compiler: "
            "${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

add_library(cms_utils STATIC
    src/format.cpp
    src/log/ansi_formatter.cpp
    src/log/formatter.cpp
    src/log/utc_offset_formatter.cpp
    src/log/styled_ansi_formatter.cpp
    src/parse.cpp
    src/string_ops.cpp
    src/utf8.cpp
)

add_library(cms::utils ALIAS cms_utils)

set_target_properties(cms_utils PROPERTIES
    EXPORT_NAME utils
)

target_compile_features(cms_utils PUBLIC cxx_std_17)

target_include_directories(cms_utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

cms_configure_target(cms_utils)

set(cmsUtils_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/cmsUtils"
)

install(TARGETS cms_utils
    EXPORT cmsUtilsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/cms/util
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cms
)

configure_package_config_file(
    cmake/cmsUtilsConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/cmsUtilsConfig.cmake"
    INSTALL_DESTINATION ${cmsUtils_INSTALL_CMAKEDIR}
)

install(EXPORT cmsUtilsTargets
    FILE cmsUtilsTargets.cmake
    NAMESPACE cms::
    DESTINATION ${cmsUtils_INSTALL_CMAKEDIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmsUtilsConfig.cmake"
    DESTINATION ${cmsUtils_INSTALL_CMAKEDIR}
)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
