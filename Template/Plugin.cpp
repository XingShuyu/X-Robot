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
int GROUPIDINT = 452675761,messageTime = 60;
string GROUPID = std::to_string(GROUPIDINT);//QQ群号
string serverName = "服务器";//服务器名称
json BindID;//绑定
json op;//op鉴定权限
string port;//服务器端口
bool with_chat,join_escape,QQforward,MCforward,whitelistAdd,listCommand,SrvInfoCommand;//配置选择
string cmdMsg;//控制台消息
string BindCheckId="";
DWORD Start;
DWORD timeStart;

using namespace std;

Logger logger("robot");



//原来在dllmain中的版本检查

void CheckProtocolVersion()
{

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
	void sendBack(string msgType, string id, string groupId, string msg);
};
void msgAPI::privateMsg(string QQnum, string msg)
{
	string http = "/send_private_msg?user_id=" + QQnum + "&message=" + msg;
	const char* path = http.c_str();
	httplib::Client cli("127.0.0.1:5700");
	auto res = cli.Get(path);

}
void msgAPI::groupMsg(string group_id, string msg)
{
	string http = "/send_group_msg?group_id=" + group_id + "&message=" + msg;
	const char* path = http.c_str();
	httplib::Client cli("127.0.0.1:5700");
	auto res = cli.Get(path);
}
void msgAPI::sendBack(string msgType, string id, string groupId, string msg)
{
	if (msgType == "private")
	{
		string http = "/send_private_msg?user_id=" + id + "&message=" + msg;
		const char* path = http.c_str();
		httplib::Client cli("127.0.0.1:5700");
		auto res = cli.Get(path);
	}
	else if (msgType == "group")
	{
		string http = "/send_group_msg?group_id=" + groupId + "&message=" + msg;
		const char* path = http.c_str();
		httplib::Client cli("127.0.0.1:5700");
		auto res = cli.Get(path);
	}
	return;
}

inline void msgCut(string message,string username)
{
	string sendmsg;
	msgcut:if (message.find("[CQ:image") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:image"), message.length());
		int CQlocate = message.find("[CQ:image");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = CQlocate+cqat.length()+1;
		cqat = "[图片]";
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	else if (message.find("[CQ:at,qq=") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:at,qq="), message.length());
		int CQlocate = message.find("[CQ:at,qq=");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = cqat.length() + CQlocate + 1;
		int CQLEN = CQlen - 1;
		cqat = cqat.substr(10, CQLEN);
		cqat = "@" + cqat;
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	else if (message.find("[CQ:face,id=") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:face,id="), message.length());
		int CQlocate = message.find("[CQ:face,id=");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = cqat.length() + CQlocate + 1;
		cqat = "[QQ表情]";
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	if (message.find("CQ:") == message.npos)
	{
		sendmsg = serverName+":<" + username + ">" + message;
		cout << "转发消息：" << sendmsg << endl;
		Level::broadcastText(sendmsg, (TextType)0);

	}

}//QQ消息处理
inline void listPlayer()
{
	vector<Player*> allPlayer = Level::getAllPlayers();
	int playerNum = allPlayer.size();
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
}
inline int customMsg(string message,string username,string cmdMsg,string userid)
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

inline bool timeChecker()
{
	DWORD nowTime = GetTickCount64();
	if ((nowTime - timeStart) > (1000 * messageTime))
	{
		timeStart = GetTickCount64();
		return true;
	}
	else
	{
		return false;
	}
}

inline int websocketsrv()
{
	reload:WSADATA wsaData;
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


	cout << "Websocket Loaded" <<endl;;
	SOCKET ClientSocket;
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
				string jsonmsg = recvbuf;
				jsonmsg = jsonmsg.substr(0, iResult);
				jsonmsg = jsonmsg.substr(jsonmsg.find("{"), jsonmsg.find("}"));
					// parse explicitly
				json jm;
				try
				{
					jm = json::parse(jsonmsg.begin(), jsonmsg.end());
				}
				catch (...) {}
				string msgtype;
				string message;
				string userid;
				string username;
				int groupid=0;
				string role="member";
				string notice_type;
				try{ msgtype = jm["message_type"];}//msgtype为消息类型，具体见CQ-http：事件
				catch (...){}
				try{ message = jm["message"]; }//message为消息内容，为string
				catch (...){}
				try { role = jm["sender"]["role"]; }//role为发送者的群聊身份，可选值："owner"群主   "admin"管理员   "member"成员,变量类型为string
				catch (...) {}
				try { userid = to_string(jm["user_id"]); }//userid为发送者QQ号，为int
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
				if (groupid == GROUPIDINT)
				{
					//常规指令集
					if (message.find("/") == 0 && message.length() >= 3)
					{
						//辨权
						if (op["OP"] == 0)
						{
							if (role != "member")//QQ执行指令
							{
								message = message.substr(1, message.length());
								Level::runcmd(message);
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
								message = message.substr(5, message.length());
								Level::runcmd(message);
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
						string cmd = "whitelist add \"" + message+"\"";
						Level::runcmd(cmd);
					}
					if ((message == "list"&& listCommand == true) && (timeChecker() == true || role != "member"))//玩家列表
					{
						listPlayer();
					}
					else if ((message == "查服"&&SrvInfoCommand==true)&&(timeChecker()==true||role!="member"))
					{
						msgAPI sendMsg;
						int current_pid = GetCurrentPid();
						float cpu_usage_ratio = GetCpuUsageRatio(current_pid);
						MEMORYSTATUSEX statex;
						statex.dwLength = sizeof(statex);
						GlobalMemoryStatusEx(&statex);
						DWORD End = GetTickCount64();
						cpu_usage_ratio = cpu_usage_ratio * 100;
						int cpu_usage = cpu_usage_ratio;
						string msg = serverName+"服务器信息"+"%0A服务器版本:"+ ll::getBdsVersion()+"%0ABDS协议号:"+to_string(ll::getServerProtocolVersion())+"%0ALL版本号:"+ll::getLoaderVersionString() + " %0A进程PID: " + to_string(current_pid) + " %0ACPU使用率 : " + to_string(cpu_usage) +"%25%0ACPU核数:"+to_string(GetCpuNum()) + " %0A内存占用 : " + to_string(statex.dwMemoryLoad) + " %25%0A总内存 : " + to_string((statex.ullTotalPhys) / 1024 / 1024) + "MB%0A剩余可用 : " + to_string(statex.ullAvailPhys / 1024 / 1024) + "MB%0A服务器启动时间:"+to_string((End-Start)/1000/60/60/24)+"天"+to_string(((End - Start) / 1000 / 60 /60)%24) + "小时" + to_string(((End - Start) / 1000 / 60 )%60) + "分钟";
						sendMsg.groupMsg(GROUPID, msg);
						listPlayer();
					}
					else if (message == "菜单"&&(timeChecker() == true || role != "member"))
					{
						msgAPI sendMsg;
						string msg = "\# Robot_LiteLoader%0A一个为BDS定制的LL机器人%0A> 功能列表%0A1. MC聊天->QQ的转发%0A2. list查在线玩家%0A3. QQ中#发送消息到mc%0A4. QQ中管理员以上级别\"/命令\"控制台执行\"命令\"%0A5. QQ新群员自动增加白名单，群员退群取消白名单, \"重置个人绑定\"来重置, \"查询绑定\"来查询%0A6. 发送\"查服\"来获取服务器信息7. 发送\"菜单\"获取指令列表8. 自定义指令 ";
						sendMsg.groupMsg(GROUPID, msg);
					}
					else if (timeChecker() == false&&(message=="菜单"||message=="查服"||message=="list"))
					{
						//msgAPI sendMsg;
						//sendMsg.groupMsg(GROUPID, "太着急了，待会再试叭");
					}
					if (message.find("%") == 0 && message.length() >= 3 && with_chat == true && QQforward == true)
					{
						message = message.substr(1, message.length());
						msgCut(message, username);
					}
					else if (with_chat == false && QQforward == true)
					{
						msgCut(message, username);
					}
					if (message == "关服" && role == "owner")
					{
						Level::runcmd("stop");
					}
					if (message == "开服" && role != "member")
					{
						Level::runcmd("stop");
					}
					if (message == "recovery" && role != "member")
					{
						Level::runcmd("stop");
					}

					//ID与白名单相关

					if (userid == BindCheckId)
					{
						msgAPI sendMsg;
						string messageOLD = message;
						cout << messageOLD << endl;
						BindCheckId = "";
						cout << BindCheckId;
						if (messageOLD.find("CQ:") == messageOLD.npos)
						{
							string msg = "你的XboxID为：" + messageOLD + " %0A正在为你绑定...";
							sendMsg.groupMsg(GROUPID, msg);

							msg = "whitelist add \"" + messageOLD + "\"";
							Level::runcmd(msg);
							BindID[userid] = messageOLD;
							ofstream a(".\\plugins\\LL_Robot\\BindID.json");//储存绑定数据
							a << std::setw(4) << BindID << std::endl;
							break;
						}
						else
						{
							sendMsg.groupMsg(GROUPID, "不要往绑定名单中塞奇怪的东西啊啊啊");
							break;
						}
						 
					}
					cout << BindCheckId;

					if ((notice_type == "group_increase" || message == "重置个人绑定") && whitelistAdd == true)
					{
						msgAPI sendMsg;
						string adderId = userid;
						BindCheckId = userid;
						string msg;
						string oldId = "";
						try
						{
							oldId = BindID[adderId];
							msg = "你好,[CQ:at,qq=" + adderId + "],你以前的id是: " + oldId + "更改将会删除旧的白名单，请告诉我你的XboxID";
							sendMsg.groupMsg(GROUPID, msg);
							msg = "whitelist remove " + oldId;
							Level::runcmd(msg);
						}
						catch (...)
						{
							msg = "欢迎新成员[CQ:at,qq=" + userid + "],让我们开始一些基础设置吧";
							sendMsg.groupMsg(GROUPID, msg);
							sendMsg.groupMsg(GROUPID, "现在，请发送你的XboxID，即你的游戏名，我们将为你绑定白名单。");
						}
					}
					if (notice_type == "group_decrease")
					{
						msgAPI sendmsg;
						string XboxName;
						try { XboxName = BindID[userid]; }
						catch (...) { XboxName = "未绑定"; }
						string msg = XboxName + "离开了我们%0A已自动删除白名单";
						sendmsg.groupMsg(GROUPID, msg);
						msg = "whitelist remove " + XboxName;
						Level::runcmd(msg);
					}
					if (message == "查询绑定")
					{
						msgAPI sendMsg;
						string XboxName;
						try
						{
							XboxName = BindID[userid];
						}
						catch (...)
						{
							XboxName = "未绑定";
						}
						string msg = "玩家[CQ:at,qq=" + userid + "],你的绑定是: " + XboxName;
						sendMsg.groupMsg(GROUPID, msg);

					}


					//自定义指令集
					//用新线程属于是性能换时间了
					thread newthread(customMsg, message, username, cmdMsg, userid);
					newthread.detach();

					
				}
				string sedbuf= "HTTP/1.1 200 OK\r\n";
				// Echo the buffer back to the sender
				iSendResult = send(ClientSocket, sedbuf.c_str(), sedbuf.length(), 0);
				sedbuf = "Cache-Control:public\r\nContent-Type:text/plain;charset=ASCII\r\nServer:Tengine/1.4.6\r\n\r\n";
				iSendResult = send(ClientSocket, sedbuf.c_str(), sedbuf.length(), 0);
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

	with_chat = info["settings"]["with_chat"];
	join_escape = info["settings"]["join/escape"];
	QQforward = info["settings"]["QQForward"];
	MCforward = info["settings"]["MCForward"];
	whitelistAdd = info["settings"]["WhitelistAdd"];
	listCommand = info["settings"]["list"];
	SrvInfoCommand = info["settings"]["SrvInfo"];
	messageTime = info["settings"]["messageTime"];
	infoFile.close();

	std::cout << "转发QQ群：" << GROUPIDINT << endl << "服务器名称：" << serverName << endl << "转发端口：" << port << endl;


	//绑定名单的读取和写入
	fstream BindIDFile;
	BindIDFile.open(".\\plugins\\X-Robot\\BindID.json");
	BindIDFile >> BindID;
	BindIDFile.close();


	//op权限名单的读取和写入
	fstream OPFile;
	OPFile.open(".\\plugins\\X-Robot\\op.json");
	OPFile >> op;


	//ll::registerPlugin("Robot", "Introduction", LL::Version(1, 0, 2),"github.com/XingShuyu/X-Robot.git","GPL-3.0","github.com");//注册插件
		//为不影响LiteLoader启动而创建新线程运行websocket
	thread tl(websocketsrv);
reBoot:try
	{
		tl.detach();
	}
	catch(...)
	{
		goto reBoot;
	}
	Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent& ev)
		{
			msgAPI msgSend;
			msgSend.groupMsg(GROUPID, "服务器已启动");
			Start = GetTickCount64();
			timeStart = GetTickCount64();
			return 0;
		});
	if(MCforward)
	{
		Event::PlayerChatEvent::subscribe([](const Event::PlayerChatEvent& ev)
			{
				msgAPI msgSend;
				Player* Player = ev.mPlayer;//获取触发监听的玩家
				string mMessage= ev.mMessage;
				string msg = serverName + ":<" + ev.mPlayer->getRealName() + ">" + mMessage;
				msgSend.groupMsg(GROUPID, msg);
				return true;
			});
	}
	Event::ConsoleOutputEvent::subscribe([](const Event::ConsoleOutputEvent& ev)
		{
			msgAPI msgSend;
			cmdMsg = ev.mOutput;
			string outPut = serverName + ": " + ev.mOutput;
			msgSend.groupMsg(GROUPID, outPut);
			return true;
		});
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
