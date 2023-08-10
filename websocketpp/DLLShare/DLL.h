#pragma once
#include <string>
//#include "json.hpp"

#ifdef _EXPORTING
#define DLL_API    __declspec(dllexport)
#else
#define DLL_API    __declspec(dllimport)
#endif

using namespace std;
using namespace nlohmann;

class DLL_API msgAPI
{
public:
	void privateMsg(string QQnum, string msg);
	void groupMsg(string group_id, string msg);
	/// void sendBack(string msgType, string id, string groupId, string msg);
};

DLL_API bool OpCheck(string userid, string role);

DLL_API extern json info;
DLL_API extern string jsonmsg;
DLL_API extern json BindID;