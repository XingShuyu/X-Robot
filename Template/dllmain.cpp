/**
 * @file DLLMain.cpp
 * @note DO NOT modify or remove this file!
 */

#include <llapi/LoggerAPI.h>
#include <llapi/ServerAPI.h>

#include "version.h"

void PluginInit();

//Logger logger(PLUGIN_NAME);



BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ll::registerPlugin(
            PLUGIN_NAME,
            PLUGIN_INTRODUCTION,
            ll::Version(PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_REVISION, PLUGIN_LLVERSION_STATUS),
            std::map<std::string, std::string>{
                {"Author", PLUGIN_AUTHOR},
        });
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C"
{
    _declspec(dllexport) void onPostInit()
    {

        std::ios::sync_with_stdio(false);
        //CheckProtocolVersion();
        PluginInit();
    }
}