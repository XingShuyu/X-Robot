cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

project(X-Robot CXX)

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()
message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86"))
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_DEBUG "")
    set(CMAKE_CXX_FLAGS_RELEASE "")
endif()

################################################################################
# Global linker options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################
add_subdirectory(Management)
add_subdirectory(Template)



























cmake_minimum_required(VERSION 3.21)
project(TemplatePlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

file(GLOB_RECURSE SRC_FILES_DIR
        ${PROJECT_SOURCE_DIR}/SDK/Header/*.cpp
        ${PROJECT_SOURCE_DIR}/SDK/Header/*.hpp
        ${PROJECT_SOURCE_DIR}/SDK/Header/*.h
        ${PROJECT_SOURCE_DIR}/Template/*.cpp
        ${PROJECT_SOURCE_DIR}/Template/*.hpp
        ${PROJECT_SOURCE_DIR}/Template/*.h
        )

include_directories(SDK/Header)
include_directories(SDK/Header/third-party)
include_directories(Template)

link_directories(.)

add_definitions(-D"NDEBUG" -D"TEMPLATE_EXPORTS" -D"WIN32_LEAN_AND_MEAN" -D"_CRT_SECURE_NO_WARNINGS" -D"_WINDOWS"
        -D"_USRDLL" -D"_AMD64_" -D"NOMINMAX" -D"_WINDLL" -D"_UNICODE" -D"UNICODE")

add_compile_options(
        /permissive- /GS /GL /W3 /Gy /Zc:wchar_t /Zi /Gm- /sdl /Zc:inline /fp:precise /errorReport:prompt /WX-
        /Zc:forScope /Gd /Oi /MD /std:c++17 /FC /EHsc /nologo /diagnostics:column
)

add_link_options(
        /MANIFEST /LTCG:incremental /NXCOMPAT /DEBUG /DLL /MACHINE:X64 /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:WINDOWS
        /MANIFESTUAC:NO /OPT:ICF /ERRORREPORT:PROMPT /NOLOGO /DELAYLOAD:"bedrock_server.dll" /TLBID:1
)

add_library(TemplatePlugin SHARED ${SRC_FILES_DIR})

add_custom_command(TARGET TemplatePlugin PRE_BUILD
        COMMAND cmd /c ${PROJECT_SOURCE_DIR}/prepareLib.cmd ${PROJECT_SOURCE_DIR}
        COMMENT "Preparing Library"
        )
