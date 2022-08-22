// Management.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define CPPHTTPLIB_OPENSSL_SUPPORT


#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include "httplib.h"
#include "Nlohmann/json.hpp"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <algorithm>

using namespace std;
using namespace YAML;

//go-cqhttp的API封装
class msgAPI
{
public:
	void privateMsg(std::string QQnum, std::string msg);
	void groupMsg(std::string group_id, std::string msg);
	/// void sendBack(string msgType, string id, string groupId, string msg);
	void sendBack(std::string msgType, std::string id, std::string groupId, std::string msg);
};
void msgAPI::privateMsg(std::string QQnum, std::string msg)
{
	string http = "/send_private_msg?user_id=" + QQnum + "&message=" + msg;
	const char* path = http.c_str();
	httplib::Client cli("127.0.0.1:5700");
	auto res = cli.Get(path);

}
void msgAPI::groupMsg(std::string group_id, std::string msg)
{
	string http = "/send_group_msg?group_id=" + group_id + "&message=" + msg;
	const char* path = http.c_str();
	httplib::Client cli("127.0.0.1:5700");
	auto res = cli.Get(path);
}
void msgAPI::sendBack(std::string msgType, std::string id, std::string groupId, std::string msg)
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


	cout << "Websocket Loaded" << endl;;
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
				int userid = 0;
				string username;
				int groupid = 0;
				string role = "member";
				string notice_type;
				try { msgtype = jm["message_type"]; }//msgtype为消息类型，具体见CQ-http：事件
				catch (...) {}
				try { message = jm["message"]; }//message为消息内容，为string
				catch (...) {}
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

			reP:if (message.find("\\n") != message.npos) { message.replace(message.find("\\n"), message.find("\\n") + 2, " "); goto reP; }

				//消息处理
				if (groupid == GROUPIDINT)
				{
					//常规指令集
					if (message.find("sudo") == 0 && message.length() >= 7)
					{
						//辨权
						if (op["OP"] == 0)
						{
							if (role != "member")//QQ执行指令
							{
								message = message.substr(5, message.length());
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
								int playerOp = op[to_string(userid)];
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
						string cmd = "whitelist add \"" + message + "\"";
						Level::runcmd(cmd);
					}
					if (message == "list")//玩家列表
					{
						listPlayer();
					}
					if (message.find("chat") == 0 && message.length() >= 6 && with_chat == true && QQforward == true)
					{
						message = message.substr(5, message.length());
						msgCut(message, username);
					}
					else if (with_chat == false && QQforward == true)
					{
						msgCut(message, username);
					}
					if (message == "查服")
					{
						msgAPI sendMsg;
						int current_pid = GetCurrentPid();
						float cpu_usage_ratio = GetCpuUsageRatio(current_pid);
						float memory_usage = GetMemoryUsage(current_pid);
						cpu_usage_ratio = cpu_usage_ratio * 100;
						int cpu_usage = cpu_usage_ratio;
						int memory_usageInt = memory_usage;
						string msg = serverName + "服务器信息%0A进程PID: " + to_string(current_pid) + "%0ACPU使用率: " + to_string(cpu_usage) + "%25%0A内存占用: " + to_string(memory_usageInt) + "MB";
						sendMsg.groupMsg(GROUPID, msg);
						listPlayer();
					}
					if (message == "菜单")
					{
						msgAPI sendMsg;
						string msg = "\# Robot_LiteLoader%0A一个为BDS定制的LL机器人%0A> 功能列表%0A1. MC聊天->QQ的转发%0A2. list查在线玩家%0A3. QQ中chat 发送消息到mc%0A4. QQ中管理员以上级别\"sudo 命令\"控制台执行\"命令\"%0A5. QQ新群员自动增加白名单，群员退群取消白名单, \"重置个人绑定\"来重置, \"查询绑定\"来查询%0A6. 发送\"查服\"来获取服务器信息7. 发送\"菜单\"获取指令列表8. 自定义指令 ";
						sendMsg.groupMsg(GROUPID, msg);
					}


					//ID与白名单相关
					if ((notice_type == "group_increase" || message == "重置个人绑定") && whitelistAdd == true)
					{
						thread groupIncrease(addNewPlayer, ref(message), ref(groupid), ref(userid));
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
					if (message == "查询绑定")
					{
						msgAPI sendMsg;
						string XboxName;
						try
						{
							XboxName = BindID[to_string(userid)];
						}
						catch (...)
						{
							XboxName = "未绑定";
						}
						string msg = "玩家[CQ:at,qq=" + to_string(userid) + "],你的绑定是: " + XboxName;
						sendMsg.groupMsg(GROUPID, msg);

					}

					//自定义指令集
					//用新线程属于是性能换时间了
					thread newthread(customMsg, message, username, cmdMsg, to_string(userid));
					newthread.detach();


				}
				string sedbuf = "HTTP/1.1 200 OK\r\n";
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

int main()
{
    std::cout << "Config go-cqhttp\n";
    string QQ;
    string password;
    cout << "QQ:\n";
    cin >> QQ;
    cout << "password:\n";
    cin >> password;
    password = "\'" + password + "\'";
    Node config = LoadFile(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
    config["account"]["uin"] = QQ;
    config["account"]["password"] = password;
    ofstream fout;
    fout.open(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
    fout << config;
    cout << "CQ已配置完毕，输入任何关闭此程序" << endl;
    cin >> QQ;
}


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
