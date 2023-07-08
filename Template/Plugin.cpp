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

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>


#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>



typedef websocketpp::client<websocketpp::config::asio_client> client;


using namespace nlohmann;
INT64 GROUPIDINT = 452675761;
int messageTime = 60;
string GROUPID = std::to_string(GROUPIDINT);//QQ群号
string serverName = "服务器";//服务器名称
json BindID;//绑定
json op;//op鉴定权限
bool with_chat,join_escape,QQforward,MCforward,whitelistAdd,listCommand,SrvInfoCommand,CommandForward;//配置选择
string cmdMsg;//控制台消息
DWORD Start;//获取服务器启动时间
DWORD timeStart;//防刷屏
string message;//收到的消息
string http;//http头(别问我为什么要弄成全局)
string accessToken;//通信密钥
string cq_ip;
int get_list_status = 0;//查未绑定名单步骤
string file_id;
int busid;
json BlackJson;

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
};


inline void msgCut(string message,string username)
{
	string sendmsg;
	msgcut:if (message.find("[CQ:image") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:image"), message.length());
		int CQlocate = (int)message.find("[CQ:image");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = (int)cqat.length()+1;
		cqat = "[图片]";
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	else if (message.find("[CQ:at,qq=") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:at,qq="), message.length());
		int CQlocate = (int)message.find("[CQ:at,qq=");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = (int)cqat.length() + 1;
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
		int CQlen = (int)cqat.length() + 1;
		cqat = "[QQ表情]";
		message = message.replace(CQlocate, CQlen, cqat);
		goto msgcut;
	}
	else if (message.find("[CQ:reply") != message.npos)
	{
		string cqat = message.substr(message.find("[CQ:reply"), message.length());
		int CQlocate = (int)message.find("[CQ:reply");
		cqat = cqat.substr(0, cqat.find_first_of("] "));
		int CQlen = (int)cqat.length();
		cqat = "[回复]";
		message = message.replace(CQlocate, CQlen+1, cqat);
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
	string msg = serverName + "服务器在线玩家:\n" + to_string(playerNum) + "位在线玩家\n";
	if (playerNum != 0)
	{
		while (i < playerNum)
		{
			Player* player = allPlayer[i];
			i++;
			msg = msg + player->getRealName() + "\n";
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
	messageFile.close();
	int num = 0;
	while (1)
	{
		string fileMsg;
		if (customMessage[to_string(num)]["QQ"].empty() == true)break;
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
			for (json::iterator it = BlackBeJson["data"]["info"].begin(); it != BlackBeJson["data"]["info"].end(); ++it) {
				string body = it.value().dump();
				json secList = json::parse(body.begin(), body.end());
				string name = secList["name"];
				string info = secList["info"];
				string url = secList["uuid"];
				url = "https://blackbe.work/detail/" + url;
				string qq = to_string(secList["qq"]);
				string level = to_string(secList["level"]);
				string msg = "玩家 " + name + " (QQ：" + qq + ")被列于云黑公开库\n危险等级: " + level + "\n封禁原因：" + info + "\n详细信息: " + url;
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
	string aa = "";
	aa = BlackJson["1"];
	if (aa != "")
	{
		int i = 1;
		bool res = false;
		while (BlackJson[to_string(i)].empty()==FALSE)
		{
			string Black;
			Black = BlackJson[to_string(i)];
			if (Black.find("*")==Black.npos && cmdMsg==Black)
			{
				res = true;
			}
			else if (Black.find("*") != Black.npos)
			{
				string Black1 = Black.substr(0, Black.find_first_of("*"));
				string Black2 = Black.substr(Black.find_first_of("*")+1, Black.length());
				if (cmdMsg.find(Black1) == 0 && cmdMsg.find(Black2) == cmdMsg.length() - Black2.length())
				{
					res = true;
				}
			}
			i = i + 1;
		}
		if (res == false)
		{
			string outPut = serverName + ": " + cmdMsg;
			msgSend.groupMsg(GROUPID, outPut);
		}
	}
}//防止命令监听导致掉TPS而建立的新线程

void groupList(string group_id, int status);

class connection_metadata {
public:
	typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

	connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
		: m_id(id)
		, m_hdl(hdl)
		, m_status("Connecting")
		, m_uri(uri)
		, m_server("N/A")
	{}

	void on_open(client* c, websocketpp::connection_hdl hdl) {
		m_status = "Open";

		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server = con->get_response_header("Server");
	}

	void on_fail(client* c, websocketpp::connection_hdl hdl) {
		m_status = "Failed";

		client::connection_ptr con = c->get_con_from_hdl(hdl);
		m_server = con->get_response_header("Server");
		m_error_reason = con->get_ec().message();
	}

	void on_close(client* c, websocketpp::connection_hdl hdl) {
		m_status = "Closed";
		client::connection_ptr con = c->get_con_from_hdl(hdl);
		std::stringstream s;
		s << "close code: " << con->get_remote_close_code() << " ("
			<< websocketpp::close::status::get_string(con->get_remote_close_code())
			<< "), close reason: " << con->get_remote_close_reason();
		m_error_reason = s.str();
	}

	void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
		if (msg->get_opcode() == websocketpp::frame::opcode::text) {
			string jsonmsg = msg->get_payload();
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
			string post_type="";
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
			try { post_type = jm["post_type"]; }
			catch (...) { post_type = ""; }
			//消息处理
			if (get_list_status == 3 && notice_type == "" && post_type == "")
			{
				////////////////////////删除旧文件
				json FileList = jm;
				for (json::iterator it = FileList["data"]["files"].begin(); it != FileList["data"]["files"].end(); ++it) {
					json secList = it.value();
					if (secList["file_name"] == "摸鱼人员名单.txt")
					{
						file_id = secList["file_id"];
						busid = secList["busid"];
						get_list_status = 4;
						groupList(GROUPID, 4);
						break;
					}
				}
				//////////////////上传新文件
				get_list_status = 0;
				groupList(GROUPID, 5);
			}
			if (get_list_status == 1 && notice_type == "" && post_type == "")
			{
				get_list_status = 2;
				msgAPI sendMsg;
				json nameList = jm;
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
				get_list_status = 3;
				groupList(GROUPID, 3);
			}
			if (groupid == GROUPIDINT && notice_type == ""&& (post_type=="message" || post_type=="message_sent")&&get_list_status==0 )
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
					string msg = serverName + "服务器信息" + "\n服务器版本:" + ll::getBdsVersion() + "\nBDS协议号:" + to_string(ll::getServerProtocolVersion()) + "\nLL版本号:" + ll::getLoaderVersionString() + " \n进程PID: " + to_string(current_pid) + " \nCPU使用率 : " + to_string(cpu_usage) + "%\nCPU核数:" + to_string(GetCpuNum()) + " \n内存占用 : " + to_string(statex.dwMemoryLoad) + " %\n总内存 : " + to_string((statex.ullTotalPhys) / 1024 / 1024) + "MB\n剩余可用 : " + to_string(statex.ullAvailPhys / 1024 / 1024) + "MB\n服务器启动时间:" + to_string((End - Start) / 1000 / 60 / 60 / 24) + "天" + to_string(((End - Start) / 1000 / 60 / 60) % 24) + "小时" + to_string(((End - Start) / 1000 / 60) % 60) + "分钟";
					sendMsg.groupMsg(GROUPID, msg);
					listPlayer();
				}
				else if (message == "菜单" && (timeChecker() == true || OpCheck(userid, role) == true))
				{
					msgAPI sendMsg;
					string msg = "\# X-Robot\n一个为BDS定制的LL机器人\n> 功能列表\n1. MC聊天->QQ的转发\n2. list查在线玩家\n3. QQ中%发送消息到mc\n4. QQ中管理员以上级别\"/命令\"控制台执行\"命令\"\n5. 群员退群取消白名单, \"绑定 ***\"来设置绑定, \"查询绑定\"来查询\n6. 发送\"查服\"来获取服务器信息7. 发送\"菜单\"获取指令列表8. 自定义指令 ";
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
						regex r("[\u4E00-\u9FA5]+");
						if (regex_match(message, r))goto outBind;;
						try {
							json BlackBe = BlackBEGet(userid, message);
							if (!BlackBe["data"]["exist"])
							{
								if ((BindID[message].empty() == false) && userid != BindID[message])
								{
									string bindqq = BindID[message];
									string msg = "该账号已经被绑定了哦\n请联系号主qq" + bindqq + "更改绑定后再试吧";
									sendMsg.groupMsg(GROUPID, msg);
									goto outBind;
								}
								string msg = "你的XboxID为：" + message + " \n正在为你绑定...";
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
							string msg = "你的XboxID为：" + message + " \n正在为你绑定...";
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
			outBind:;
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
					if (message.find("[CQ:at,qq=") != message.npos)
					{
						message = message.substr(10, message.length() - 12);
					}
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
						message = message.substr(10, message.length() - 12);
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
					get_list_status = 1;
					groupList(GROUPID,1);

				}
				if (message.find("查云黑") == 0 && message.length() > 9 && OpCheck(userid, role) == true)
				{
					thread BlackBeCheckTh(BlackBeCheck, message);
					BlackBeCheckTh.detach();
				}


				//调试用指令

				if (message == "强制崩溃" && OpCheck(userid, role) == true)
				{
					string crashString = "123456";
					crashString = crashString.substr(100, 105);
				}
				//自定义指令集
				//用新线程属于是性能换时间了
				thread newthread(customMsg, message, username, cmdMsg, userid);
				newthread.detach();


			}
			if (groupid == GROUPIDINT && notice_type == "group_increase" && whitelistAdd == true)
			{
				msgAPI sendMsg;
				string msg;
				msg = "欢迎新成员[CQ:at,qq=" + userid + "],发送\"绑定 ****\"进行绑定并获取白名单";
				sendMsg.groupMsg(GROUPID, msg);
			}
			if (groupid == GROUPIDINT && notice_type == "group_decrease")
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
				string msg = XboxName + "(QQ" + userid + ")离开了我们 \n已自动删除白名单";
				sendmsg.groupMsg(GROUPID, msg);
				msg = "whitelist remove " + XboxName;
				Level::runcmd(msg);
			}
		}
		else {
			m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload()));
		}
	}

	websocketpp::connection_hdl get_hdl() const {
		return m_hdl;
	}

	int get_id() const {
		return m_id;
	}

	std::string get_status() const {
		return m_status;
	}

	void record_sent_message(std::string message) {
	}

	friend std::ostream& operator<< (std::ostream& out, connection_metadata const& data);
private:
	int m_id;
	websocketpp::connection_hdl m_hdl;
	std::string m_status;
	std::string m_uri;
	std::string m_server;
	std::string m_error_reason;
	std::vector<std::string> m_messages;
};

std::ostream& operator<< (std::ostream& out, connection_metadata const& data) {
	out << "> URI: " << data.m_uri << "\n"
		<< "> Status: " << data.m_status << "\n"
		<< "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
		<< "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";
	out << "> Messages Processed: (" << data.m_messages.size() << ") \n";

	std::vector<std::string>::const_iterator it;
	for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
		out << *it << "\n";
	}

	return out;
}

class websocket_endpoint {
public:
	websocket_endpoint() : m_next_id(0) {
		m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
		m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

		m_endpoint.init_asio();
		m_endpoint.start_perpetual();

		m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
	}

	~websocket_endpoint() {
		m_endpoint.stop_perpetual();

		for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
			if (it->second->get_status() != "Open") {
				// Only close open connections
				continue;
			}

			std::cout << "> Closing connection " << it->second->get_id() << std::endl;

			websocketpp::lib::error_code ec;
			m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
			if (ec) {
				std::cout << "> Error closing connection " << it->second->get_id() << ": "
					<< ec.message() << std::endl;
			}
		}
		cout << "closed" << endl;
		m_thread->join();
	}

	int connect(std::string const& uri) {
		websocketpp::lib::error_code ec;

		client::connection_ptr con = m_endpoint.get_connection(uri, ec);

		if (ec) {
			std::cout << "> Connect initialization error: " << ec.message() << std::endl;
			return -1;
		}

		int new_id = m_next_id++;
		connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(new_id, con->get_handle(), uri);
		m_connection_list[new_id] = metadata_ptr;

		con->set_open_handler(websocketpp::lib::bind(
			&connection_metadata::on_open,
			metadata_ptr,
			&m_endpoint,
			websocketpp::lib::placeholders::_1
		));
		con->set_fail_handler(websocketpp::lib::bind(
			&connection_metadata::on_fail,
			metadata_ptr,
			&m_endpoint,
			websocketpp::lib::placeholders::_1
		));
		con->set_close_handler(websocketpp::lib::bind(
			&connection_metadata::on_close,
			metadata_ptr,
			&m_endpoint,
			websocketpp::lib::placeholders::_1
		));
		con->set_message_handler(websocketpp::lib::bind(
			&connection_metadata::on_message,
			metadata_ptr,
			websocketpp::lib::placeholders::_1,
			websocketpp::lib::placeholders::_2
		));

		m_endpoint.connect(con);

		return new_id;
	}

	void close(int id, websocketpp::close::status::value code, std::string reason) {
		websocketpp::lib::error_code ec;

		con_list::iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			std::cout << "> No connection found with id " << id << std::endl;
			return;
		}

		m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
		if (ec) {
			std::cout << "> Error initiating close: " << ec.message() << std::endl;
		}
	}

	void send(int id, std::string message) {
		websocketpp::lib::error_code ec;

		con_list::iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			std::cout << "> No connection found with id " << id << std::endl;
			return;
		}

		m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
		if (ec) {
			std::cout << "> Error sending message: " << ec.message() << std::endl;
			return;
		}

		metadata_it->second->record_sent_message(message);
	}

	connection_metadata::ptr get_metadata(int id) const {
		con_list::const_iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			return connection_metadata::ptr();
		}
		else {
			return metadata_it->second;
		}
	}
private:
	typedef std::map<int, connection_metadata::ptr> con_list;

	client m_endpoint;
	websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

	con_list m_connection_list;
	int m_next_id;
};

websocket_endpoint endpoint;

void msgAPI::privateMsg(string QQnum, string msg)
{

}
void groupMsgSend(string group_id, string msg)
{
	msg = "{\"action\": \"send_group_msg\",\"params\": {\"group_id\": \"" + group_id + "\",\"message\": \"" + msg + "\",\"auto_escape\": \"false\"}}";
	endpoint.send(0, msg);
}
void msgAPI::groupMsg(string group_id, string msg)
{
	thread groupMsgTh(groupMsgSend, group_id, msg);
	groupMsgTh.detach();
}
void groupList(string group_id,int status)
{
	if (status == 1)
	{
		string abcd = "{\"action\": \"get_group_member_list\",\"params\": {\"group_id\": \"" + GROUPID + "\",\"no_cache\": \"true\"}}";
		endpoint.send(0, abcd);
	}
	if (status == 3)
	{
		string abcd = "{\"action\": \"get_group_root_files\",\"params\": {\"group_id\": \"" + GROUPID + "\"}}";
		endpoint.send(0, abcd);
	}
	if (status == 4)
	{
		string abcd = "{\"action\": \"delete_group_file\",\"params\": {\"group_id\": \"" + GROUPID + "\",\"file_id\": \"" + file_id + "\",\"busid\": \"" + to_string(busid) + "\"}}";
		endpoint.send(0, abcd);
	}
	if (status == 5)
	{
		string abcd = "{\"action\": \"upload_group_file\",\"params\": {\"group_id\": \"" + GROUPID + "\",\"file\": \"..\\NoBindList.txt\",\"name\": \"NoBindList.txt\"}}";
		endpoint.send(0, abcd);
		std::system("powershell rm plugins\\X-Robot\\NoBindList.txt");
	}
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
	cq_ip = info["cq_ip"];
	infoFile.close();

	std::cout << "转发QQ群：" << GROUPID << endl << "服务器名称：" << serverName << endl << "转发地址：" << cq_ip << endl;


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
	int MsgNum = 0;
	while (!BlackCmd.eof())
	{
		MsgNum = MsgNum + 1;
		char a[512];
		BlackCmd.getline(a, 512);
		BlackJson[to_string(MsgNum)] = a;
		BlackMsg = BlackMsg + "\n" + a;
	}
	//ll::registerPlugin("Robot", "Introduction", LL::Version(1, 0, 2),"github.com/XingShuyu/X-Robot.git","GPL-3.0","github.com");//注册插件
		//为不影响LiteLoader启动而创建新线程运行websocket
	int id = endpoint.connect(cq_ip);
	if (id != -1) {
		std::cout << "> Created connection with id " << id << std::endl;
	}


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
