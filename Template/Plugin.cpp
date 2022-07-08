#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "pch.h"
#include <EventAPI.h>
#include <LoggerAPI.h>
#include <MC/Level.hpp>
#include <MC/BlockInstance.hpp>
#include <MC/Block.hpp>
#include <MC/BlockSource.hpp>
#include <MC/Actor.hpp>
#include <MC/Player.hpp>
#include <MC/ItemStack.hpp>
#include <LLAPI.h>
#include <third-party/Nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <algorithm>
#include <httplib.h>

#pragma comment(lib, "Ws2_32.lib")

int GROUPIDINT = 452675761;
string GROUPID = std::to_string(GROUPIDINT);
string serverName = "服务器";
json BindID;


using namespace std;

Logger logger("robot");


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

void msgCut(string message,string username)
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
void listPlayer()
{
	vector<Player*> allPlayer = Level::getAllPlayers();
	int playerNum = allPlayer.size();
	int i = 0;
	string msg="服务器在线玩家:%0A"+to_string(playerNum) + "位在线玩家%0A";
	if (playerNum != 0)
	{
		while (i < playerNum)
		{
			Player* player = allPlayer[i];
			i++;
			msg = msg + player->getRealName() + "%0A";
			cout << player->getRealName() << endl;
		}
	}
	cout << playerNum << endl;
	msgAPI sendMsg;
	sendMsg.groupMsg(GROUPID, msg);
}

int customMsg(string message,string username)
{
	fstream messageFile;
	messageFile.open(".\\plugins\\LL_Robot\\Message.json");
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
		}
		num++;
	}
}
int addNewPlayer(string &message, int &groupId, int &userid)
{
	msgAPI sendMsg;
	string adderId = to_string(userid);
	string msg = "欢迎新成员[CQ:at,qq=" + to_string(userid) + "],让我们开始一些基础设置吧";
	sendMsg.groupMsg(GROUPID, msg);
	sendMsg.groupMsg(GROUPID, "现在，请发送你的XboxID，即你的游戏名，我们将为你绑定白名单。");
	userid = 0;
	while (1)
	{
		if (to_string(userid) == adderId)
		{
			string messageOLD = message;
			msg = "你的XboxID为：" + messageOLD+" %0A正在为你绑定...";
			sendMsg.groupMsg(GROUPID, msg);
			msg = "whitelist add " + messageOLD;
			Level::runcmd(msg);
			BindID[adderId] = messageOLD;
			ofstream a(".\\plugins\\LL_Robot\\BindID.json");//储存绑定数据
			a << std::setw(4) << BindID << std::endl;
			break;
		}
	}


}


int websocketsrv()
{
	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
#define DEFAULT_PORT "5701"

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
#define DEFAULT_BUFLEN 4096

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
				int userid=0;
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
				try { userid = jm["user_id"]; }//userid为发送者QQ号，为int
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
					if (message.find("sudo") == 0 && role != "member" && message.length() >= 6)//QQ执行指令
					{
						message = message.substr(5, message.length());
						Level::runcmd(message);
					}
					else if (role == "member" && message.find("sudo") == 0)
					{
						msgAPI sendmsg;
						sendmsg.groupMsg(GROUPID, "权限不足，拒绝执行");
					}
					if (message.find("添加白名单") == 0 && role == "owner" && message.length() >= 17)
					{
						message = message.substr(16, message.length());
						string cmd = "whitelist add " + message;
						Level::runcmd(cmd);
					}
					if (message == "list")//玩家列表
					{
						listPlayer();
					}
					if (message.find("chat") == 0 && message.length() >= 6)
					{
						message = message.substr(5, message.length());
						msgCut(message, username);
					}

					//ID与白名单相关
					if (notice_type == "group_increase" || message == "重置个人绑定")
					{
						thread groupIncrease(addNewPlayer,ref(message),ref(groupid),ref(userid));
						groupIncrease.detach();
					}
					if (notice_type == "group_decrease")
					{
						msgAPI sendmsg;
						string XboxName = BindID[to_string(userid)];
						string msg = XboxName + "离开了我们%0A已自动删除白名单";
						sendmsg.groupMsg(GROUPID, msg);
						msg = "whitelist remove " + XboxName;
						Level::runcmd(msg);
					}


					//自定义指令集
					//用新线程属于是性能换时间了
					thread newthread(customMsg, message, username);
					newthread.detach();

					
				}

				
				

				// Echo the buffer back to the sender
				iSendResult = send(ClientSocket, recvbuf, iResult, 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed: %d\n", WSAGetLastError());
					closesocket(ClientSocket);
					WSACleanup();
					return 1;
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
int PluginInit()
{
	//信息文件的读取和默认值写入
	json info = "{\"QQ_group_id\":722047078,\"serverName\":\"Your Server Name\"}"_json;//信息默认值
	json File;
	fstream infoFile;
	infoFile.open(".\\plugins\\LL_Robot\\RobotInfo.json");
	if (infoFile.fail() == 1)
	{
		infoFile.close();
		ofstream o(".\\plugins\\LL_Robot\\RobotInfo.json");
		o << setw(4) << info << endl;
		o.close();
	}
	else
	{
		infoFile >> info;
	}
	GROUPIDINT = info["QQ_group_id"];
	GROUPID = std::to_string(GROUPIDINT);
	serverName = info["serverName"];
	std::cout << "转发QQ群：" << GROUPIDINT << endl << "服务器名称：" << serverName << endl;


	//绑定名单的读取和写入
	fstream BindIDFile;
	BindIDFile.open(".\\plugins\\LL_Robot\\BindID.json");
	BindIDFile >> BindID;
	BindIDFile.close();

	LL::registerPlugin("Robot", "Introduction", LL::Version(1, 0, 2));//注册插件
		//为不影响LiteLoader启动而创建新线程运行winsocket
	thread tl(websocketsrv);
	tl.detach();

	Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent& ev)
		{
			msgAPI msgSend;
			msgSend.groupMsg(GROUPID, "服务器已启动");
			return 0;
		});

	//这部分为在玩家加入时发送XXX加入了游戏，如要启用，可以删除头，尾的"/*","*/"随后编译
	/*Event::PlayerJoinEvent::subscribe([](const Event::PlayerJoinEvent& ev)
		{//
			msgAPI msgSend;
			Player* Player = ev.mPlayer;//获取触发监听的玩家
			string msg = ev.mPlayer->getRealName() + "加入了游戏";
			msgSend.groupMsg(GROUPID, msg);
			return true;
		});*/
	Event::PlayerChatEvent::subscribe([](const Event::PlayerChatEvent& ev)
		{//
			msgAPI msgSend;
			Player* Player = ev.mPlayer;//获取触发监听的玩家
			string mMessage= ev.mMessage;
			string msg = "<" + ev.mPlayer->getRealName() + ">" + mMessage;
			msgSend.groupMsg(GROUPID, msg);
			return true;
		});
	Event::ConsoleOutputEvent::subscribe([](const Event::ConsoleOutputEvent& ev)
		{
			msgAPI msgSend;
			string outPut = ev.mOutput;
			msgSend.groupMsg(GROUPID, outPut);
			return true;
		});
	//这部分为在玩家离开时发送XXX离开了游戏，如要启用，可以删除头，尾的"/*","*/"随后编译
	/*Event::PlayerLeftEvent::subscribe([](const Event::PlayerLeftEvent& ev)
		{//
			msgAPI msgSend;
			Player* Player = ev.mPlayer;//获取触发监听的玩家
			string msg = ev.mPlayer->getRealName() + "离开了游戏";
			msgSend.groupMsg(GROUPID, msg);
			return true;
		});
	*/
}
