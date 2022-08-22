#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define CPPHTTPLIB_OPENSSL_SUPPORT

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
#include "httplib.h"
#include "SysInfo.h"

#pragma comment(lib, "Ws2_32.lib")

int GROUPIDINT = 452675761;
string GROUPID = std::to_string(GROUPIDINT);//QQ群号
string serverName = "服务器";//服务器名称
json BindID;//绑定
json op;//op鉴定权限
string port;//服务器端口
bool with_chat,join_escape,QQforward,MCforward,whitelistAdd;//配置选择
string cmdMsg;//控制台消息


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
inline int addNewPlayer(string &message, int &groupId, int &userid)
{
	msgAPI sendMsg;
	string adderId = to_string(userid);
	string msg;
	string oldId = "";
	try
	{
		oldId = BindID[adderId];
		msg = "你好,[CQ:at,qq=" + adderId + "],你以前的id是: " + oldId + "更改将会删除旧的白名单，若确定更改，请告诉我你的XboxID";
		sendMsg.groupMsg(GROUPID, msg);

	}
	catch(...)
	{
		msg = "欢迎新成员[CQ:at,qq=" + to_string(userid) + "],让我们开始一些基础设置吧";
		sendMsg.groupMsg(GROUPID, msg);
		sendMsg.groupMsg(GROUPID, "现在，请发送你的XboxID，即你的游戏名，我们将为你绑定白名单。");
	}

	userid = 0;
	while (1)
	{
		if (to_string(userid) == adderId)
		{
			string messageOLD = message;
			cout << messageOLD << endl;
			if (messageOLD.find("CQ:") == messageOLD.npos)
			{
				msg = "你的XboxID为：" + messageOLD + " %0A正在为你绑定...";
				sendMsg.groupMsg(GROUPID, msg);
				msg = "whitelist add \"" + messageOLD + "\"";
				Level::runcmd(msg);
				BindID[adderId] = messageOLD;
				ofstream a(".\\plugins\\LL_Robot\\BindID.json");//储存绑定数据
				a << std::setw(4) << BindID << std::endl;
				if (oldId != "")
				{
					msg = "whitelist remove " + oldId;
					Level::runcmd(msg);
				}
				break;
			}
			else
			{
				sendMsg.groupMsg(GROUPID, "不要往绑定名单中塞奇怪的东西啊啊啊");
				break;
			}
		}
	}


}


int PluginInit()
{
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


	LL::registerPlugin("Robot", "Introduction", LL::Version(1, 0, 2));//注册插件
		//为不影响LiteLoader启动而创建新线程运行websocket
	thread tl(websocketsrv);
	tl.detach();

	Event::ServerStartedEvent::subscribe([](const Event::ServerStartedEvent& ev)
		{
			msgAPI msgSend;
			msgSend.groupMsg(GROUPID, "服务器已启动");
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
	
		//这部分为在玩家离开时发送XXX离开了游戏，如要启用，可以删除头，尾的"/*","*/"随后编译
		Event::PlayerLeftEvent::subscribe([](const Event::PlayerLeftEvent& ev)
			{//
				msgAPI msgSend;
				Player* Player = ev.mPlayer;//获取触发监听的玩家
				string msg = ev.mPlayer->getRealName() + "离开了游戏";
				msgSend.groupMsg(GROUPID, msg);
				return true;
			});
	

		//这部分为在玩家加入时发送XXX加入了游戏，如要启用，可以删除头，尾的"/*","*/"随后编译
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
