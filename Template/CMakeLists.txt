set(PROJECT_NAME X-Robot)

################################################################################
# Source groups
################################################################################
set(Header
    "framework.h"
    "httplib.h"
    "pch.h"
    "SysInfo.h"
    "openssl/include/openssl/ssl3.h"
)
source_group("Header" FILES ${Header})

set(Json
    "../Json/BindID.json"
    "../Json/Message.json"
    "../Json/op.json"
    "../Json/RobotInfo.json"
)
source_group("Json" FILES ${Json})

set(Source
    "dllmain.cpp"
    "pch.cpp"
    "Plugin.cpp"
    "version.h"
)
source_group("Source" FILES ${Source})

set(ALL_FILES
    ${Header}
    ${Json}
    ${Source}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE X-Robot)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION         "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION         "TRUE"
    )
endif()
################################################################################
# Include directories
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/Header/third-party;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/Header;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Template/openssl/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../websocketpp;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../boost;"
        "$ENV{VC_IncludePath};"
        "$ENV{WindowsSDK_IncludePath}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/Header/third-party;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../SDK/Header;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Template/openssl/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../websocketpp;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../boost;"
        "$ENV{VC_IncludePath};"
        "$ENV{WindowsSDK_IncludePath}"
    )
endif()

################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "NDEBUG;"
        "TEMPLATE_EXPORTS;"
        "WIN32_LEAN_AND_MEAN;"
        "_CRT_SECURE_NO_WARNINGS;"
        "_WINDOWS;"
        "_USRDLL;"
        "_AMD64_;"
        "NOMINMAX;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "NDEBUG;"
        "TEMPLATE_EXPORTS;"
        "WIN32_LEAN_AND_MEAN;"
        "_CRT_SECURE_NO_WARNINGS;"
        "_WINDOWS;"
        "_USRDLL;"
        "_AMD64_;"
        "NOMINMAX;"
        "UNICODE;"
        "_UNICODE"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /permissive-;
            /Oi;
            /sdl;
            /Gy;
            /std:c++17;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Y-
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /permissive-;
            /Oi;
            /sdl;
            /Gy;
            /std:c++17;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Y-
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:bedrock_server.dll;
            /DEBUG;
            /SUBSYSTEM:WINDOWS;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:bedrock_server.dll;
            /DEBUG;
            /SUBSYSTEM:WINDOWS;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        )
    endif()
endif()

################################################################################
# Pre build events
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_api.lib goto process
        COMMAND   $<CONFIG:Debug> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_var.lib goto process
        COMMAND   $<CONFIG:Debug> goto end
        COMMAND   $<CONFIG:Debug> :process
        COMMAND   $<CONFIG:Debug> cd /d $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>\\SDK\\Tools\\
        COMMAND   $<CONFIG:Debug> if exist $ENV{LocalDebuggerWorkingDirectory}\\bedrock_server.pdb (
        COMMAND   $<CONFIG:Debug> LibraryBuilder.exe -o ..\\Lib\\ $ENV{LocalDebuggerWorkingDirectory}
        COMMAND   $<CONFIG:Debug> ) else (
        COMMAND   $<CONFIG:Debug> LibraryBuilder.exe -o ..\\Lib\\
        COMMAND   $<CONFIG:Debug> )
        COMMAND   $<CONFIG:Debug> :end
        COMMAND $<CONFIG:Release> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_api.lib goto process
        COMMAND $<CONFIG:Release> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_var.lib goto process
        COMMAND $<CONFIG:Release> goto end
        COMMAND $<CONFIG:Release> :process
        COMMAND $<CONFIG:Release> cd /d $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>\\SDK\\Tools\\
        COMMAND $<CONFIG:Release> if exist $ENV{LocalDebuggerWorkingDirectory}\\bedrock_server.pdb (
        COMMAND $<CONFIG:Release> LibraryBuilder.exe -o ..\\Lib\\ $ENV{LocalDebuggerWorkingDirectory}
        COMMAND $<CONFIG:Release> ) else (
        COMMAND $<CONFIG:Release> LibraryBuilder.exe -o ..\\Lib\\
        COMMAND $<CONFIG:Release> )
        COMMAND $<CONFIG:Release> :end
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_api.lib goto process
        COMMAND   $<CONFIG:Debug> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_var.lib goto process
        COMMAND   $<CONFIG:Debug> goto end
        COMMAND   $<CONFIG:Debug> :process
        COMMAND   $<CONFIG:Debug> cd /d $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>\\SDK\\Tools\\
        COMMAND   $<CONFIG:Debug> if exist $ENV{LocalDebuggerWorkingDirectory}\\bedrock_server.pdb (
        COMMAND   $<CONFIG:Debug> LibraryBuilder.exe -o ..\\Lib\\ $ENV{LocalDebuggerWorkingDirectory}
        COMMAND   $<CONFIG:Debug> ) else (
        COMMAND   $<CONFIG:Debug> LibraryBuilder.exe -o ..\\Lib\\
        COMMAND   $<CONFIG:Debug> )
        COMMAND   $<CONFIG:Debug> :end
        COMMAND $<CONFIG:Release> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_api.lib goto process
        COMMAND $<CONFIG:Release> if not exist $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>SDK\\Lib\\bedrock_server_var.lib goto process
        COMMAND $<CONFIG:Release> goto end
        COMMAND $<CONFIG:Release> :process
        COMMAND $<CONFIG:Release> cd /d $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>\\SDK\\Tools\\
        COMMAND $<CONFIG:Release> if exist $ENV{LocalDebuggerWorkingDirectory}\\bedrock_server.pdb (
        COMMAND $<CONFIG:Release> LibraryBuilder.exe -o ..\\Lib\\ $ENV{LocalDebuggerWorkingDirectory}
        COMMAND $<CONFIG:Release> ) else (
        COMMAND $<CONFIG:Release> LibraryBuilder.exe -o ..\\Lib\\
        COMMAND $<CONFIG:Release> )
        COMMAND $<CONFIG:Release> :end
    )
endif()

################################################################################
# Post build events
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> if exist $ENV{LocalDebuggerWorkingDirectory}\\plugins\\ (
        COMMAND   $<CONFIG:Debug> copy /Y $<SHELL_PATH:$<TARGET_FILE:${PROJECT_NAME}>> $ENV{LocalDebuggerWorkingDirectory}\\plugins\\
        COMMAND   $<CONFIG:Debug> )
        COMMAND $<CONFIG:Release> if exist $ENV{LocalDebuggerWorkingDirectory}\\plugins\\ (
        COMMAND $<CONFIG:Release> copy /Y $<SHELL_PATH:$<TARGET_FILE:${PROJECT_NAME}>> $ENV{LocalDebuggerWorkingDirectory}\\plugins\\
        COMMAND $<CONFIG:Release> )
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> if exist $ENV{LocalDebuggerWorkingDirectory}\\plugins\\ (
        COMMAND   $<CONFIG:Debug> copy /Y $<SHELL_PATH:$<TARGET_FILE:${PROJECT_NAME}>> $ENV{LocalDebuggerWorkingDirectory}\\plugins\\
        COMMAND   $<CONFIG:Debug> )
        COMMAND $<CONFIG:Release> if exist $ENV{LocalDebuggerWorkingDirectory}\\plugins\\ (
        COMMAND $<CONFIG:Release> copy /Y $<SHELL_PATH:$<TARGET_FILE:${PROJECT_NAME}>> $ENV{LocalDebuggerWorkingDirectory}\\plugins\\
        COMMAND $<CONFIG:Release> )
    )
endif()

################################################################################
# Dependencies
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "$(CoreLibraryDependencies);"
        "Ws2_32;"
        "crypt32;"
        "$(SolutionDir)SDK\\lib\\bedrock_server_api;"
        "$(SolutionDir)SDK\\lib\\bedrock_server_var;"
        "$(SolutionDir)SDK\\lib\\SymDBHelper;"
        "$(SolutionDir)SDK\\lib\\LiteLoader;"
        "$(SolutionDir)Template\\openssl\\libssl-3-x64;"
        "$(SolutionDir)Template\\openssl\\libcrypto-3-x64"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "$(CoreLibraryDependencies);"
        "Ws2_32;"
        "crypt32;"
        "$(SolutionDir)SDK\\lib\\bedrock_server_api;"
        "$(SolutionDir)SDK\\lib\\bedrock_server_var;"
        "$(SolutionDir)SDK\\lib\\SymDBHelper;"
        "$(SolutionDir)SDK\\lib\\LiteLoader;"
        "$(SolutionDir)Template\\openssl\\libssl-3-x64;"
        "$(SolutionDir)Template\\openssl\\libcrypto-3-x64"
    )
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_SOURCE_DIR}/Template/openssl;"
        "${CMAKE_SOURCE_DIR}/SDK/lib"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_SOURCE_DIR}/Template/openssl;"
        "${CMAKE_SOURCE_DIR}/SDK/lib"
    )
endif()

