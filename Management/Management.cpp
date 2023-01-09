// Management.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include "httplib.h"
#include "nlohmann/json.hpp"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <TlHelp32.h>


#pragma comment(lib, "Ws2_32.lib")



using namespace std;
using namespace YAML;
using namespace nlohmann;

DWORD timeStart = 0;
int interval = 0;
json op;
bool start_mode;
httplib::Client cli("127.0.0.1:5700");
string accessToken;

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
	auto res = cli.Get(path);

}
void msgAPI::groupMsg(string group_id, string msg)
{
	string http = "/send_group_msg?group_id=" + group_id + "&message=" + msg + "&access_Token=" + accessToken;
	const char* path = http.c_str();
	auto res = cli.Get(path);
}
void msgAPI::sendBack(string msgType, string id, string groupId, string msg)
{
	if (msgType == "private")
	{
		string http = "/send_private_msg?user_id=" + id + "&message=" + msg;
		const char* path = http.c_str();
		auto res = cli.Get(path);
	}
	else if (msgType == "group")
	{
		string http = "/send_group_msg?group_id=" + groupId + "&message=" + msg;
		const char* path = http.c_str();
		auto res = cli.Get(path);
	}
	return;
}

INT64 GROUPIDINT;
string port;
string serverName;
int backupTime;

string GBK_2_UTF8(string gbkStr)
{
	string outUtf8 = "";
	int n = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, NULL, 0);
	WCHAR* str1 = new WCHAR[n];
	MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	char* str2 = new char[n];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
	outUtf8 = str2;
	delete[]str1;
	str1 = NULL;
	delete[]str2;
	str2 = NULL;
	return outUtf8;
}

string UTF8_2_GBK(string utf8Str)
{
	string outGBK = "";
	int n = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
	WCHAR* str1 = new WCHAR[n];
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_ACP, 0, str1, -1, NULL, 0, NULL, NULL);
	char* str2 = new char[n];
	WideCharToMultiByte(CP_ACP, 0, str1, -1, str2, n, NULL, NULL);
	outGBK = str2;
	delete[] str1;
	str1 = NULL;
	delete[] str2;
	str2 = NULL;
	return outGBK;
}



void lunch()
{
	Sleep(5000);
	if (start_mode)
	{
		system("start .\\BDS-Deamon.cmd");
	}
	else
	{
		system(".\\BDS-Deamon.cmd");
	}
}

int autoBackup()
{
Backup:Sleep(backupTime * 3600 * 1000);
	system("xcopy \"worlds\\Bedrock level\" plugins\\X-Robot\\customBackup\\ /s /y");
	goto Backup;
}

inline bool OpCheck(string userid, string role)
{
	if (op["OP"] == 0)
	{
		if (role != "member")
		{
			return true;
		}
		else if (role == "member")
		{
			return false;
		}
	}
	else if (op["OP"] == 1)
	{
		try
		{
			int playerOp = op[userid];
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
}

void rec()
{
	Sleep(5000);
	system("xcopy plugins\\X-Robot\\customBackup\\ \"worlds\\Bedrock level\" /s /y");
	msgAPI msgSend;
	msgSend.groupMsg(to_string(GROUPIDINT), GBK_2_UTF8("回档完成,正在重启服务器"));
	system("start .\\BDS-Deamon.cmd");
}
void bak()
{
	system("powershell rm plugins\\X-Robot\\customBackup -Recurse");
	system("xcopy \"worlds\\Bedrock level\" plugins\\X-Robot\\customBackup\\ /s /y");
	msgAPI msgSend;
	msgSend.groupMsg(to_string(GROUPIDINT), GBK_2_UTF8("备份完成"));
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
				string userid = "";
				string username;
				int groupid = 0;
				string role = "member";
				string notice_type;
				string post_type;
				try { msgtype = jm["message_type"]; }//msgtype为消息类型，具体见CQ-http：事件
				catch (...) {}
				try { message = jm["message"]; }//message为消息内容，为string
				catch (...) {}
				try { role = jm["sender"]["role"]; }//role为发送者的群聊身份，可选值："owner"群主   "admin"管理员   "member"成员,变量类型为string
				catch (...) {}
				try { userid = to_string(jm["user_id"]); }//userid为发送者QQ号，为int
				catch (...) {}
				try { post_type = to_string(jm["post_type"]); }//post_type为消息类型号，为string
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
				if (post_type == "\"meta_event\"")
				{
					timeStart = GetTickCount64();
				}
				if (groupid == GROUPIDINT)
				{
					if (message == GBK_2_UTF8("开服") && OpCheck(userid, role) == true)
					{
						cout << "正在开服" << endl;
						thread lunchSrv(lunch);
						lunchSrv.detach();
					}
					if (message == "backup" && OpCheck(userid, role) == true)
					{
						thread bakTh(bak);
						bakTh.detach();
					}
					if (message == "recovery" && OpCheck(userid, role) == true)
					{
						thread recTh(rec);
						recTh.detach();
					}
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


int configCQ()
{
	std::cout << "配置 go-cqhttp\n";
	string QQ;
	string password;
	cout << "QQ:\n";
	cin >> QQ;
	cout << "password:\n";
	cin >> password;
	Node config = LoadFile(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
	cout << "CQ已被自动配置完成";
	config["account"]["uin"] = QQ;
	config["account"]["password"] = password;

	ofstream fout;
	fout.open(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
	fout << config;
	return 0;
}
int startCq()
{
	system(".\\start_go-cqhttp.bat");
	return 0;
}



inline bool CQSelfChecker()//cq启动自检
{
a:Sleep(60000);
	while ((GetTickCount64() - timeStart) <= 3000 * interval)
	{
		Sleep(1000);
	}
	cout << "CQ疑似异常，尝试启动CQ...." << endl;
	system("tskill go-cqhttp");
	thread tl(startCq);
	tl.detach();
	goto a;
}



int main()
{
	system("chcp 936");
	cli.set_bearer_token_auth("TendedGalaxy522");
	json info;
	fstream infoFile;
	Node config = LoadFile(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
	interval = config["heartbeat"]["interval"].as<int>();
	infoFile.open(".\\plugins\\X-Robot\\RobotInfo.json");
	infoFile >> info;
	GROUPIDINT = info["QQ_group_id"];
	serverName = info["serverName"];
	port = info["manager_port"];
	start_mode = info["manager"]["start_mode"];
	bool aleadyConfig = info["manager"]["cqhttp_config"];
	accessToken = info["accessToken"];
	backupTime = info["manager"]["backup_interval"];
	infoFile.close();


	fstream OPFile;
	OPFile.open(".\\plugins\\X-Robot\\op.json");
	OPFile >> op;


	if (info["manager"]["multi_server"] == false)
	{
		if (aleadyConfig == false)
		{
			ofstream infoFileOut;
			configCQ();
			infoFileOut.open(".\\plugins\\X-Robot\\RobotInfo.json");
			info["manager"]["cqhttp_config"] = true;
			cout << "配置机器人基础信息:" << endl << "你的服务器群QQ号:" << endl;
			cin >> GROUPIDINT;
			cout << "请输入你的服务器名称" << endl;
			cin >> serverName;

			info["QQ_group_id"] = GROUPIDINT;
			string OldName = info["serverName"];
			string JsonInfo = info.dump(4);
			JsonInfo.replace(JsonInfo.find(OldName), OldName.length(), serverName);
			infoFileOut << GBK_2_UTF8(JsonInfo);
			infoFileOut.close();
		}
		thread tl(startCq);
		tl.detach();
		thread Protect(CQSelfChecker);
		Protect.detach();
	}
	thread backupTl(autoBackup);
	backupTl.detach();
	websocketsrv();
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