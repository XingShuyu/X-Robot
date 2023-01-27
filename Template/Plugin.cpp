/**
 * @file plugin.cpp
 * @brief The main file of the plugin
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define CPPHTTPLIB_OPENSSL_SUPPORT
#pragma comment(lib, "libssl-3-x64.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "libcrypto-3-x64.lib")

#include <openssl/ssl3.h>
#include "httplib.h"
#include <llapi/ServerAPI.h>
#include <llapi/EventAPI.h>
#include <llapi/LoggerAPI.h>
#include "version.h"
#include <llapi/mc/Level.hpp>
#include <llapi/mc/BlockInstance.hpp>
#include <llapi/mc/Block.hpp>
#include <llapi/mc/BlockSource.hpp>
#include <llapi/mc/Actor.hpp>
#include <llapi/mc/Player.hpp>
#include <llapi/mc/ItemStack.hpp>
#include <llapi/LLAPI.h>
#include <Nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <algorithm>
#include <sysinfoapi.h>
#include "SysInfo.h"
#include <regex>


using namespace nlohmann;
INT64 GROUPIDINT = 452675761;
int messageTime = 60;
string GROUPID = std::to_string(GROUPIDINT);//QQ群号
string serverName = "服务器";//服务器名称
json BindID;//绑定
json op;//op鉴定权限
string port;//服务器端口
bool with_chat,join_escape,QQforward,MCforward,whitelistAdd,listCommand,SrvInfoCommand,CommandForward;//配置选择
string cmdMsg;//控制台消息
DWORD Start;//获取服务器启动时间
DWORD timeStart;//防刷屏
string message;//收到的消息
string http;//http头(别问我为什么要弄成全局)
string accessToken;//通信密钥
string back_ip;

using namespace std;

Logger FileLog("X-Robot");
Logger XLog("X-Robot");

//原来在dllmain中的版本检查

void CheckProtocolVersion()
{
	Logger logger("X-Robot");
#ifdef TARGET_BDS_PROTOCOL_VERSION

	auto current_protocol = ll::getServerProtocolVersion();
	if (TARGET_BDS_PROTOCOL_VERSION != current_protocol)
	{
		logger.warn("Protocol version mismatched! Target version: {}. Current version: {}.",
			TARGET_BDS_PROTOCOL_VERSION, current_protocol);
		logger.warn("This may result in crash. Please switch to the version matching the BDS version!");
	}

#endif // TARGET_BDS_PROTOCOL_VERSION
}



//go-cqhttp的API封装
class msgAPI
{
public:
	void privateMsg(string QQnum, string msg);
	void groupMsg(string group_id, string msg);
	/// void sendBack(string msgType, string id, string groupId, string msg);
	json groupList(string group_id, bool no_cache);
};
void msgAPI::privateMsg(string QQnum, string msg)
{
	httplib::Client cli(back_ip);
	http = "/send_private_msg?user_id=" + QQnum + "&message=" + msg;
	const char* path = http.c_str();
	auto res = cli.Get(path);

}
void groupMsgSend(string group_id, string msg)
{
	httplib::Client cli(back_ip);
	http = "/send_group_msg?group_id=" + group_id + "&message=" + msg;
	if (accessToken != "")
	{
		http = http + "&access_token=" + accessToken;
	}
	const char* path = http.c_str();
	auto res = cli.Get(path);
}
void msgAPI::groupMsg(string group_id, string msg)
{
	thread groupMsgTh(groupMsgSend, group_id, msg);
	groupMsgTh.detach();
}
json msgAPI::groupList(string group_id, bool no_cache)
{
	httplib::Client cli(back_ip);
	string a;
	if (no_cache) { a = "true"; }else{ a = "false"; }
	http = "/get_group_member_list?group_id=" + group_id + "&no_cache=" + a;
	if (accessToken != "")
	{
		http = http + "&access_token=" + accessToken;
	}
	auto res = cli.Get(http.c_str());
	json resJson = json::parse(res->body.begin(), res->body.end());
	return resJson;
}

inline void msgCut(string message,string username)
{
	string sendmsg;
	msgcut:if (message.find("[CQ:image") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:image"), message.length());
		int CQlocate = (int)message.find("[CQ:image");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = CQlocate+ (int)cqat.length()+1;
		cqat = "[图片]";
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	else if (message.find("[CQ:at,qq=") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:at,qq="), message.length());
		int CQlocate = (int)message.find("[CQ:at,qq=");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = (int)cqat.length() + CQlocate + 1;
		int CQLEN = CQlen - 1;
		cqat = cqat.substr(10, CQLEN);
		cqat = "@" + cqat;
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	else if (message.find("[CQ:face,id=") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:face,id="), message.length());
		int CQlocate = (int)message.find("[CQ:face,id=");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = (int)cqat.length() + CQlocate + 1;
		cqat = "[QQ表情]";
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	if (message.find("CQ:") == message.npos)
	{
		sendmsg = serverName+":<" + username + ">" + message;
		XLog.info("转发消息:" + sendmsg);
		FileLog.info("[转发消息]" + sendmsg);
		Level::broadcastText(sendmsg, (TextType)0);

	}

}//QQ消息处理
inline void listPlayer()
{
	vector<Player*> allPlayer = Level::getAllPlayers();
	int playerNum = (int)allPlayer.size();
	int i = 0;
	string msg = serverName + "服务器在线玩家:%0A" + to_string(playerNum) + "位在线玩家%0A";
	if (playerNum != 0)
	{
		while (i < playerNum)
		{
			Player* player = allPlayer[i];
			i++;
			msg = msg + player->getRealName() + "%0A";
		}
	}
	msgAPI sendMsg;
	sendMsg.groupMsg(GROUPID, msg);
}//获取在线玩家
inline void customMsg(string message,string username,string cmdMsg,string userid)
{
	fstream messageFile;
	messageFile.open(".\\plugins\\X-Robot\\Message.json");
	json customMessage;
	messageFile >> customMessage;
	int num = 0;
	while (1)
	{
		string fileMsg;
		try
		{
			fileMsg = customMessage[to_string(num)]["QQ"];
		}
		catch (...)
		{
			break;
		}
		if (fileMsg == message)
		{
			
			if (customMessage[to_string(num)]["cmd"] != "")
			{
				Level::runcmd(customMessage[to_string(num)]["cmd"]);
			}
			if (customMessage[to_string(num)]["mc"] != "")
			{
				string sendmsg = customMessage[to_string(num)]["mc"];
				sendmsg = serverName+":<" + username + ">" + sendmsg;
				Level::broadcastText(sendmsg, (TextType)0);
			}
			if (customMessage[to_string(num)]["QQback"] != "")
			{
				string sendmsg = customMessage[to_string(num)]["QQback"];
				string newStr = customMessage[to_string(num)]["QQback"];
				msgAPI sendMsg;
				regexStart:if (sendmsg.find("#") != sendmsg.npos && sendmsg.find("$") != sendmsg.npos)
				{
					string cutMsg = sendmsg.substr(sendmsg.find("$"), 3);
					regex pattern(sendmsg.substr(sendmsg.find("#") + 1, sendmsg.find("$") - sendmsg.find("#")-1));
					if (cutMsg == "$QQ") { cutMsg = userid; }
					if (cutMsg == "$MC") { cutMsg = cmdMsg; }
					if (cutMsg == "$QM") { cutMsg = message; }
					if (cutMsg == "$QN") { cutMsg = username; }
					smatch result;
					string::const_iterator iterStart = cutMsg.begin();
					string::const_iterator iterEnd = cutMsg.end();
					regex_search(iterStart, iterEnd, result, pattern);
					newStr.replace(newStr.find("#"), newStr.find("$") - newStr.find("#") + 3, result[0]);
					cout << newStr << endl;
					sendmsg = sendmsg.substr(sendmsg.find("$") + 3, sendmsg.length());
					goto regexStart;
				}
				sendMsg.groupMsg(GROUPID, newStr);
			}
		}
		num++;
	}
}
inline bool OpCheck(string userid,string role)
{
	if (op["OP"] == 0)
	{
		if (role != "member")//QQ执行指令
		{
			return true;
		}
		else if (role == "member")
		{
			return false;
		}
	}
	else if (op["OP"] == 1&&op[userid].empty())
	{
		return false;
	}
	else if (op["OP"] == 1 && !op[userid].empty())
	{
		return true;
	}
}//检查是否有op权限

inline bool timeChecker()
{
	DWORD nowTime = unsigned(GetTickCount64());
	if ((nowTime - timeStart) > unsigned(1000 * messageTime))
	{
		timeStart = unsigned(GetTickCount64());
		return true;
	}
	else
	{
		return false;
	}
}//防刷屏

//获取云黑信息
inline json BlackBEGet(string userid) {
	httplib::Client BlackBe("https://api.blackbe.work");
	http = "/openapi/v3/check/?qq=" + userid;
	string body;
	auto res = BlackBe.Get(http,
		[&](const char* data, size_t data_length) {
			body.append(data, data_length);
	return true;
		});
	json BlackJson = json::parse(body.begin(), body.end());
	return BlackJson;
}
inline json BlackBEGet(string userid,string XboxId) {
	httplib::Client BlackBe("https://api.blackbe.work");
	http = "/openapi/v3/check/?qq=" + userid + "&name=" + XboxId;
	string body;
	auto res = BlackBe.Get(http,
		[&](const char* data, size_t data_length) {
			body.append(data, data_length);
	return true;
		});
	json BlackJson = json::parse(body.begin(), body.end());
	return BlackJson;
}
inline void BlackBeCheck(string message)
{
	if (message.substr(9, 1) == " " && message.length() > 10) { message = message.substr(10, message.length()); }
	else if (message.substr(9, 1) != " ") { message = message.substr(9, message.length()); }
	json BlackBeJson;
	msgAPI sendMsg;
	try {
		if (message.find("[CQ:at,qq=") != message.npos)//查找QQ号
		{
			message = message.substr(10, message.length() - 11);
			if (!BindID[message].empty())//QQ已绑定
			{
				string XboxName;
				XboxName = BindID[message];
				BlackBeJson = BlackBEGet(message, XboxName);
			}
			else {//QQ无绑定
				BlackBeJson = BlackBEGet(message);
			}
		}
		else//查找XboxId
		{
			if (!BindID[message].empty())//XboxID已绑定
			{
				string QQid;
				QQid = BindID[message];
				BlackBeJson = BlackBEGet(QQid, message);
			}
			else//XboxID未绑定
			{
				httplib::Client BlackBe("https://api.blackbe.work");
				http = "/openapi/v3/check/?name=" + message;
				string body;
				auto res = BlackBe.Get(http,
					[&](const char* data, size_t data_length) {
						body.append(data, data_length);
				return true;
					});
				BlackBeJson = json::parse(body.begin(), body.end());
			}
		}
		if (!BlackBeJson["data"]["exist"]) {
			sendMsg.groupMsg(GROUPID, "玩家不在黑名单中哦");
		}
		else if (BlackBeJson["data"]["exist"])
		{
			cout << 1 << endl;
			for (json::iterator it = BlackBeJson["data"]["info"].begin(); it != BlackBeJson["data"]["info"].end(); ++it) {
				string body = it.value().dump();
				json secList = json::parse(body.begin(), body.end());
				string name = secList["name"];
				cout << 3 << endl;
				string info = secList["info"];
				cout << 3 << endl;
				string url = secList["uuid"];
				url = "https://blackbe.work/detail/" + url;
				cout << 3 << endl;
				string qq = to_string(secList["qq"]);
				cout << 1 << endl;
				string level = to_string(secList["level"]);
				cout << 2 << endl;
				string msg = "玩家 " + name + " (QQ：" + qq + ")被列于云黑公开库\n危险等级: " + level + "\n封禁原因：" + info + "\n详细信息: " + url;
				cout << 3 << endl;
				sendMsg.groupMsg(GROUPID, msg);
			}
		}
	}
	catch (...)
	{
		sendMsg.groupMsg(GROUPID, "云黑查询失败");
	}
}

inline void ConsoleEvent(string BlackMsg) {
	msgAPI msgSend;
	if (BlackMsg.find(cmdMsg) == BlackMsg.npos)
	{
		string outPut = serverName + ": " + cmdMsg;
		msgSend.groupMsg(GROUPID, outPut);
	}
}//防止命令监听导致掉TPS而建立的新线程

inline int websocketsrv()
{
	reload:SOCKET ClientSocket;
	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
#define DEFAULT_PORT port.c_str()

	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	// Create a SOCKET for the server to listen for client connections

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	// Setup the TCP listening socket
	iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}


	cout << "Websocket Loaded" << endl;;

	while (1)
	{
		ClientSocket = INVALID_SOCKET;
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
#define DEFAULT_BUFLEN 8192

		char recvbuf[DEFAULT_BUFLEN];
		int iResult, iSendResult;
		int recvbuflen = DEFAULT_BUFLEN;

		// Receive until the peer shuts down the connection
		do {

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				try {
					string jsonmsg = recvbuf;
					jsonmsg = jsonmsg.substr(0, iResult);
					jsonmsg = jsonmsg.substr(jsonmsg.find("{"), jsonmsg.find_last_of("}") - 4);
					if (jsonmsg.find("\"meta_event_type\":\"heartbeat\"") == jsonmsg.npos)
					{
						struct _stat info;
						_stat(".\\plugins\\X-Robot\\LastestLog.txt", &info);
						int size = info.st_size;
						if (size / 1024 / 1024 > 10) {
							ofstream a;
							a.open(".\\plugins\\X-Robot\\LastestLog.txt");
							a << "";
							a.close();
						}
						FileLog.info("[源数据]" + jsonmsg);
					}
					//cout <<"/////////////////////////////" << jsonmsg << "////////////////////////////////" << endl;
						// parse explicitly
					json jm;
					try
					{
						jm = json::parse(jsonmsg.begin(), jsonmsg.end());
					}
					catch (...) { cout << "反序列化失败" << endl << endl << endl << endl; FileLog.error("反序列化失败!"); }
					string userid;
					string username;
					INT64 groupid = 0;
					string role = "member";
					string notice_type;
					try { message = jm["message"]; }//message为消息内容，为string
					catch (...) {}
					try { role = jm["sender"]["role"]; }//role为发送者的群聊身份，可选值："owner"群主   "admin"管理员   "member"成员,变量类型为string
					catch (...) {}
					try { userid = to_string(jm["user_id"]); }//userid为发送者QQ号，为string
					catch (...) {}
					try
					{
						username = jm["sender"]["card"];
						if (username == "")
						{
							username = jm["sender"]["nickname"];
						}
					}//username，为发送者的的群昵称（优先）或者用户名，为string
					catch (...) {}
					try { groupid = jm["group_id"]; }//groupid为消息来源的QQ群，为int
					catch (...) {}
					try { notice_type = jm["notice_type"]; }
					catch (...) { notice_type = ""; }
					//消息处理
					if (groupid == GROUPIDINT && notice_type == "")
					{
						//常规指令集
						if (message.find("/") == 0 && message.length() >= 3)
						{
							//辨权
							if (op["OP"] == 0)
							{
								if (role != "member")//QQ执行指令
								{
									string cmdMsg = message.substr(1, message.length());
									Level::runcmd(cmdMsg);
								}
								else if (role == "member")
								{
									msgAPI sendmsg;
									sendmsg.groupMsg(GROUPID, "权限不足，拒绝执行");
								}
							}
							else if (op["OP"] == 1)
							{
								try
								{
									int playerOp = op[userid];
									string cmdMsg = message.substr(1, message.length());
									Level::runcmd(cmdMsg);
								}
								catch (...)
								{
									msgAPI sendmsg;
									sendmsg.groupMsg(GROUPID, "权限不足，拒绝执行");
								}
							}
						}


						if (message.find("添加白名单") == 0 && role == "owner" && message.length() >= 17)
						{
							message = message.substr(16, message.length());
							string cmd = "whitelist add \"" + message + "\"";
							Level::runcmd(cmd);
						}
						if ((message == "list" && listCommand == true) && (timeChecker() == true || OpCheck(userid, role) == true))//玩家列表
						{
							listPlayer();
						}
						else if ((message == "查服" && SrvInfoCommand == true) && (timeChecker() == true || OpCheck(userid, role) == true))
						{
							msgAPI sendMsg;
							int current_pid = GetCurrentPid();
							float cpu_usage_ratio = GetCpuUsageRatio(current_pid);
							MEMORYSTATUSEX statex;
							statex.dwLength = sizeof(statex);
							GlobalMemoryStatusEx(&statex);
							DWORD End = unsigned(GetTickCount64());
							cpu_usage_ratio = cpu_usage_ratio * 100;
							int cpu_usage = cpu_usage_ratio;
							string msg = serverName + "服务器信息" + "%0A服务器版本:" + ll::getBdsVersion() + "%0ABDS协议号:" + to_string(ll::getServerProtocolVersion()) + "%0ALL版本号:" + ll::getLoaderVersionString() + " %0A进程PID: " + to_string(current_pid) + " %0ACPU使用率 : " + to_string(cpu_usage) + "%25%0ACPU核数:" + to_string(GetCpuNum()) + " %0A内存占用 : " + to_string(statex.dwMemoryLoad) + " %25%0A总内存 : " + to_string((statex.ullTotalPhys) / 1024 / 1024) + "MB%0A剩余可用 : " + to_string(statex.ullAvailPhys / 1024 / 1024) + "MB%0A服务器启动时间:" + to_string((End - Start) / 1000 / 60 / 60 / 24) + "天" + to_string(((End - Start) / 1000 / 60 / 60) % 24) + "小时" + to_string(((End - Start) / 1000 / 60) % 60) + "分钟";
							sendMsg.groupMsg(GROUPID, msg);
							listPlayer();
						}
						else if (message == "菜单" && (timeChecker() == true || OpCheck(userid, role) == true))
						{
							msgAPI sendMsg;
							string msg = "\# X-Robot%0A一个为BDS定制的LL机器人%0A> 功能列表%0A1. MC聊天->QQ的转发%0A2. list查在线玩家%0A3. QQ中%25发送消息到mc%0A4. QQ中管理员以上级别\"/命令\"控制台执行\"命令\"%0A5. 群员退群取消白名单, \"绑定 ***\"来设置绑定, \"查询绑定\"来查询%0A6. 发送\"查服\"来获取服务器信息7. 发送\"菜单\"获取指令列表8. 自定义指令 ";
							sendMsg.groupMsg(GROUPID, msg);
						}
						else if (timeChecker() == false && (message == "菜单" || message == "查服" || message == "list"))
						{
							//msgAPI sendMsg;
							//sendMsg.groupMsg(GROUPID, "太着急了，待会再试叭");
						}
						if (message.find("%") == 0 && message.length() >= 3 && with_chat == true && QQforward == true)
						{
							message = message.substr(1, message.length());
							thread msgCutTh(msgCut, message, username);//多线程处理
							msgCutTh.detach();
						}
						else if (with_chat == false && QQforward == true)
						{
							thread msgCutTh(msgCut, message, username);//多线程处理
							msgCutTh.detach();
						}
						if (message == "关服" && role == "owner")
						{
							Level::runcmd("stop");
						}
						if (message == "开服" && OpCheck(userid, role) == true)
						{
							Level::runcmd("stop");
						}
						if (message == "recovery" && OpCheck(userid, role) == true)
						{
							Level::runcmd("stop");
						}

						//ID与白名单相关

						if (message.find("绑定") == 0 && message.length() > 6)
						{
							msgAPI sendMsg;
							if (message.find("[CQ:") == message.npos && message.find("[mirai:") == message.npos)
							{
								if (message.substr(6, 1) == " " && message.length() > 7) { message = message.substr(7, message.length()); }
								else if (message.substr(6, 1) != " ") { message = message.substr(6, message.length()); }
								try {
									json BlackBe = BlackBEGet(userid, message);
									if (!BlackBe["data"]["exist"])
									{
										string msg = "你的XboxID为：" + message + " %0A正在为你绑定...";
										sendMsg.groupMsg(GROUPID, msg);
										if (!BindID[userid].empty()) {
											string XboxName = "";
											XboxName = BindID[userid];
											BindID.erase(BindID.find(userid));
											BindID.erase(BindID.find(XboxName));
											ofstream a(".\\plugins\\X-Robot\\BindID.json");//储存绑定数据
											a << std::setw(4) << BindID << std::endl;
											a.close();
											msg = "whitelist remove \"" + XboxName + "\"";
											Level::runcmd(msg);
										}
										BindID[userid] = message;
										BindID[message] = userid;
										ofstream a(".\\plugins\\X-Robot\\BindID.json");//储存绑定数据
										a << std::setw(4) << BindID << std::endl;
										a.close();
										msg = "whitelist add \"" + message + "\"";
										Level::runcmd(msg);
									}
									else if (BlackBe["data"]["exist"]) {
										sendMsg.groupMsg(GROUPID, "玩家[CQ:at,qq=" + userid + "]账号被列于云黑，请用查云黑检查！");
									}
								}
								catch (...)
								{
									sendMsg.groupMsg(GROUPID, "云黑检查失败，尝试不检查云黑进行绑定");
									string msg = "你的XboxID为：" + message + " %0A正在为你绑定...";
									sendMsg.groupMsg(GROUPID, msg);
									if (!BindID[userid].empty()) {
										string XboxName = "";
										XboxName = BindID[userid];
										BindID.erase(BindID.find(userid));
										BindID.erase(BindID.find(XboxName));
										ofstream a(".\\plugins\\X-Robot\\BindID.json");//储存绑定数据
										a << std::setw(4) << BindID << std::endl;
										a.close();
										msg = "whitelist remove \"" + XboxName + "\"";
										Level::runcmd(msg);
									}
									BindID[userid] = message;
									BindID[message] = userid;
									ofstream a(".\\plugins\\X-Robot\\BindID.json");//储存绑定数据
									a << std::setw(4) << BindID << std::endl;
									a.close();
									msg = "whitelist add \"" + message + "\"";
									Level::runcmd(msg);
								}
							}
							else
							{
								sendMsg.groupMsg(GROUPID, "不要往绑定名单中塞奇怪的东西啊啊啊");
							}

						}
						if ((notice_type == "group_increase") && whitelistAdd == true)
						{
							msgAPI sendMsg;
							string msg;
							msg = "欢迎新成员[CQ:at,qq=" + userid + "],发送\"绑定 ****\"进行绑定并获取白名单";
							sendMsg.groupMsg(GROUPID, msg);
						}
						if (notice_type == "group_decrease")
						{
							msgAPI sendmsg;
							string XboxName;
							try {
								XboxName = BindID[userid];
								BindID.erase(BindID.find(userid));
								BindID.erase(BindID.find(XboxName));
								ofstream a(".\\plugins\\X-Robot\\BindID.json");//储存绑定数据
								a << std::setw(4) << BindID << std::endl;
								a.close();
							}
							catch (...) { XboxName = "未绑定" + userid; }
							string msg = XboxName + "(QQ" + userid + ")离开了我们 %0A已自动删除白名单";
							sendmsg.groupMsg(GROUPID, msg);
							msg = "whitelist remove " + XboxName;
							Level::runcmd(msg);
						}
						if (message == "查询绑定")
						{
							msgAPI sendMsg;
							string XboxName;
							if (BindID[userid].empty())
							{
								XboxName = "未绑定";
							}
							else
							{
								XboxName = BindID[userid];
							}
							string msg = "玩家[CQ:at,qq=" + userid + "],你的绑定是: " + XboxName;
							sendMsg.groupMsg(GROUPID, msg);

						}
						if (message.find("删除绑定") == 0 && message.length() > 12 && OpCheck(userid, role) == true)
						{
							if (message.substr(12, 1) == " " && message.length() > 13) { message = message.substr(13, message.length()); }
							else if (message.substr(12, 1) != " ") { message = message.substr(12, message.length()); }
							msgAPI sendmsg;
							string XboxName;
							string msg;
							try {
								XboxName = BindID[message];
								BindID.erase(BindID.find(message));
								BindID.erase(BindID.find(XboxName));
								msg = "whitelist remove " + XboxName;
								Level::runcmd(msg);
								ofstream a(".\\plugins\\X-Robot\\BindID.json");//储存绑定数据
								a << std::setw(4) << BindID << std::endl;
								a.close();
								msg = "已删除玩家 " + XboxName + " 的绑定和白名单";
							}
							catch (...) { XboxName = "未绑定" + userid; msg = "玩家 [CQ:at,qq=" + message + "] 未绑定"; }
							sendmsg.groupMsg(GROUPID, msg);

						}
						if (message.find("查询绑定") == 0 && message.length() > 12 && OpCheck(userid, role) == true)
						{
							if (message.substr(12, 1) == " " && message.length() > 13) { message = message.substr(13, message.length()); }
							else if (message.substr(12, 1) != " ") { message = message.substr(12, message.length()); }
							if (message.find("[CQ:at,qq=") != message.npos)
							{
								message = message.substr(10, message.length() - 11);
							}
							string XboxName;
							if (BindID[message].empty())
							{
								XboxName = "未绑定";
							}
							else
							{
								XboxName = BindID[message];
							}
							string msg = "玩家 " + message + " 的绑定是: " + XboxName;
							msgAPI sendMsg;
							sendMsg.groupMsg(GROUPID, msg);

						}//查询他人绑定
						if (message == "未绑定名单" && OpCheck(userid, role) == true)
						{
							msgAPI sendMsg;
							json nameList = sendMsg.groupList(GROUPID, true);
							string Num = "摸鱼人员名单:\n";
							for (json::iterator it = nameList["data"].begin(); it != nameList["data"].end(); ++it) {
								json secList = it.value();
								string user_id = to_string(secList["user_id"]);
								string noneNickname = secList["nickname"];
								if (BindID[user_id].empty() == true)
								{
									Num = Num + "QQ:" + user_id + "," + "群名称:" + noneNickname + "\n";
								}
							}
							ofstream Text(".\\plugins\\X-Robot\\NoBindList.txt");
							Text << Num << endl;
							Text.close();

							////////////////////////删除旧文件
							httplib::Client cli(back_ip);
							auto res = cli.Get("/get_group_root_files?group_id=" + GROUPID + "&access_token=" + accessToken);
							json FileList = json::parse(res->body.begin(), res->body.end());
							for (json::iterator it = FileList["data"]["files"].begin(); it != FileList["data"]["files"].end(); ++it) {
								json secList = it.value();
								if (secList["file_name"] == "摸鱼人员名单.txt")
								{
									string file_id = secList["file_id"];
									int busid = secList["busid"];
									http = "/delete_group_file?group_id=" + GROUPID + "&file_id=" + file_id + "&busid=" + to_string(busid);
									if (accessToken != "")
									{
										http = http + "&access_token=" + accessToken;
									}
									cli.Get(http);
									break;
								}
							}
							//////////////////上传新文件
							cli.Get("/upload_group_file?group_id=" + GROUPID + "&file=..\\NoBindList.txt&name=摸鱼人员名单.txt&access_token=" + accessToken);
							std::system("powershell rm plugins\\X-Robot\\NoBindList.txt");
						}
						if (message.find("查云黑") == 0 && message.length() > 9 && OpCheck(userid, role) == true)
						{
							thread BlackBeCheckTh(BlackBeCheck, message);
							BlackBeCheckTh.detach();
						}


						//调试用指令

						if (message == "强制崩溃" && role == "owner")
						{
							string crashString = "123456";
							crashString = crashString.substr(100, 105);
						}
						//自定义指令集
						//用新线程属于是性能换时间了
						thread newthread(customMsg, message, username, cmdMsg, userid);
						newthread.detach();


					}
					string sedbuf = "HTTP/1.1 200 OK\r\n";
					// Echo the buffer back to the sender
					iSendResult = send(ClientSocket, sedbuf.c_str(), (int)sedbuf.length(), 0);
					sedbuf = "Cache-Control:public\r\nContent-Type:text/plain;charset=ASCII\r\nServer:Tengine/1.4.6\r\n\r\n";
					iSendResult = send(ClientSocket, sedbuf.c_str(), (int)sedbuf.length(), 0);
					if (iSendResult == SOCKET_ERROR) {
						printf("send failed: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						closesocket(ListenSocket);
						WSACleanup();
						cout << "reloading" << endl;
						goto reload;
						//return 1;
					}

				}
				catch (...) {
					cout << "机器人发生崩溃qwq，正在生成错误日志" << endl;
					SYSTEMTIME sys;
					GetLocalTime(&sys);
					string cmd = "echo f | xcopy .\\plugins\\X-Robot\\LastestLog.txt .\\plugins\\X-Robot\\CrashLog\\CrashLog-" + to_string(sys.wYear) + "-" + to_string(sys.wMonth) + "-" + to_string(sys.wDay) + "-" + to_string(sys.wHour) + ".txt /y";
					cout << cmd;
					FileLog.error("机器人崩溃!");
					system(cmd.c_str());
				}//崩溃日志
			}

			else if (iResult == 0)
				printf("Connection ended...\n");
			else {
				printf("recv failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}

			// shutdown the send half of the connection since no more data will be sent
			iResult = shutdown(ClientSocket, SD_SEND);
			if (iResult == SOCKET_ERROR) {
				printf("shutdown failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}

		} while (iResult > 0);
	}

	return 1;
	
}

extern Logger loggerPlu;

void PluginInit()
{
	CheckProtocolVersion();
	Logger logger(PLUGIN_NAME);
	logger.info("若见Websocket Loaded则机器人启动成功");
	//信息文件的读取

	json info;
	fstream infoFile;
	infoFile.open(".\\plugins\\X-Robot\\RobotInfo.json");
	infoFile >> info;
	GROUPIDINT = info["QQ_group_id"];
	GROUPID = std::to_string(GROUPIDINT);
	serverName = info["serverName"];
	port = info["port"];
	accessToken = info["accessToken"];

	with_chat = info["settings"]["with_chat"];
	join_escape = info["settings"]["join/escape"];
	QQforward = info["settings"]["QQForward"];
	MCforward = info["settings"]["MCForward"];
	whitelistAdd = info["settings"]["WhitelistAdd"];
	listCommand = info["settings"]["list"];
	SrvInfoCommand = info["settings"]["SrvInfo"];
	messageTime = info["settings"]["messageTime"];
	CommandForward = info["settings"]["CommandForward"];
	back_ip = info["back_ip"];
	infoFile.close();

	std::cout << "转发QQ群：" << GROUPID << endl << "服务器名称：" << serverName << endl << "转发端口：" << port << endl;


	//绑定名单的读取和写入
	fstream BindIDFile;
	BindIDFile.open(".\\plugins\\X-Robot\\BindID.json");
	BindIDFile >> BindID;
	BindIDFile.close();


	//op权限名单的读取和写入
	fstream OPFile;
	OPFile.open(".\\plugins\\X-Robot\\op.json");
	OPFile >> op;

	//log系统初始化
	{
		ofstream a;
		a.open(".\\plugins\\X-Robot\\LastestLog.txt");
		a << "";
		a.close();
	}
	FileLog.setFile(".\\plugins\\X-Robot\\LastestLog.txt");
	FileLog.info("Log开始写入");
	FileLog.consoleLevel = 0;

	//消息转发黑名单读取
	string BlackMsg;
	fstream BlackCmd;
	BlackCmd.open(".\\plugins\\X-Robot\\BlackMsg.txt");
	while (!BlackCmd.eof())
	{
		char a[256];
		BlackCmd.getline(a, 256);
		BlackMsg = BlackMsg + "\n" + a;
	}
	//ll::registerPlugin("Robot", "Introduction", LL::Version(1, 0, 2),"github.com/XingShuyu/X-Robot.git","GPL-3.0","github.com");//注册插件
		//为不影响LiteLoader启动而创建新线程运行websocket
	thread tl(websocketsrv);
	tl.detach();
	Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent& ev)
		{
			msgAPI msgSend;
			msgSend.groupMsg(GROUPID, "服务器已启动");
			Start = unsigned(GetTickCount64());
			timeStart = unsigned(GetTickCount64());
			return 0;
		});
	if(MCforward)
	{
		Event::PlayerChatEvent::subscribe([](const Event::PlayerChatEvent& ev)
			{
				msgAPI msgSend;
		Player* Player = ev.mPlayer;//获取触发监听的玩家
		string mMessage = ev.mMessage;
		string msg = serverName + ":<" + ev.mPlayer->getRealName() + ">" + mMessage;
		msgSend.groupMsg(GROUPID, msg);
		return true;
			});
	}
	if (CommandForward)
	{
		Event::ConsoleOutputEvent::subscribe([BlackMsg](const Event::ConsoleOutputEvent& ev)
			{

				cmdMsg = ev.mOutput.substr(0,ev.mOutput.length()-1);
		thread SendCommand(ConsoleEvent, BlackMsg);
		SendCommand.detach();
				/*if (BlackMsg.find(cmdMsg) == BlackMsg.npos)
				{
					string outPut = serverName + ": " + ev.mOutput;
					msgSend.groupMsg(GROUPID, outPut);
				}*/
				return true;
			});
	}
	if(join_escape)
	{
	
		Event::PlayerLeftEvent::subscribe([](const Event::PlayerLeftEvent& ev)
			{//
				msgAPI msgSend;
				Player* Player = ev.mPlayer;//获取触发监听的玩家
				string msg = ev.mPlayer->getRealName() + "离开了游戏";
				msgSend.groupMsg(GROUPID, msg);
				return true;
			});
	

		Event::PlayerJoinEvent::subscribe([](const Event::PlayerJoinEvent& ev)
			{//
				msgAPI msgSend;
				Player* Player = ev.mPlayer;//获取触发监听的玩家
				string msg = ev.mPlayer->getRealName() + "加入了游戏";
				msgSend.groupMsg(GROUPID, msg);
				return true;
			});
	}
}
