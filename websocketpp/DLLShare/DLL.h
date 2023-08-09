#pragma once
#include <string>

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
extern DLL_API string jsonmsg;
extern DLL_API string GROUPID;