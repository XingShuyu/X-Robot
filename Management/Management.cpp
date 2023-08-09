// Management.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <TlHelp32.h>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>


#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <tcpmib.h>


typedef websocketpp::client<websocketpp::config::asio_client> client;

using namespace std;
using namespace YAML;
using namespace nlohmann;

int interval = 0;
json op;
bool start_mode;
string accessToken;
string backupList[5];
string cq_ip;

//go-cqhttp的API封装

/*
class msgAPI
{
public:
	void privateMsg(string QQnum, string msg);
	void groupMsg(string group_id, string msg);
	/// void sendBack(string msgType, string id, string groupId, string msg);
};
*/


INT64 GROUPIDINT;
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

BOOL GetTcpPortState(ULONG nPort)
{
	MIB_TCPTABLE TcpTable[512];
	DWORD nSize = sizeof(TcpTable);
	if (NO_ERROR == GetTcpTable(&TcpTable[0], &nSize, TRUE))
	{
		DWORD nCount = TcpTable[0].dwNumEntries;
		if (nCount > 0)
		{
			for (DWORD i = 0; i < nCount; i++)
			{
				MIB_TCPROW TcpRow = TcpTable[0].table[i];
				DWORD temp1 = TcpRow.dwLocalPort;
				int temp2 = temp1 / 256 + (temp1 % 256) * 256;
				if (temp2 == nPort)
				{
					return TRUE;
				}
			}
		}
		return FALSE;
	}
	return FALSE;
}

void lunch()
{
	Sleep(5000);
	if (start_mode)
	{
		system("start .\\bedrock_server_mod.exe");
	}
	else
	{
		system(".\\bedrock_server_mod.exe");
	}
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
	return false;
}


string QSign;
json info;

int configCQ()
{
	std::cout << "配置 go-cqhttp\n";
	string QQ;
	string password;
	cout << "QQ:\n";
	cin >> QQ;
	cout << "password:\n";
	cin >> password;
	cout << "使用的QSign\n(输入-禁用QSign服务器)\n(推荐输入0使用本地QSign服务器):\n";
	cin>>QSign;
	cout << "access-token:\n(强烈建议配置在公网的服务器设置access-token,如果使用本地go-cqhttp,会自动同步access-token)\n";
	cin >> accessToken;
	if (QSign == "0") { QSign = "http://127.0.0.1:9000"; }
	if (QSign == "-") { system("powershell rm .\\plugins\\X-Robot\\qsign\\ -Recurse"); }
	info["manager"]["qsign"] = QSign;
	Node config = LoadFile(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
	cout << "CQ已被自动配置完成";
	config["account"]["uin"] = QQ;
	config["account"]["password"] = password;
	config["account"]["sign-server"] = QSign;
	config["default-middlewares"]["access-token"] = accessToken;
	info["accessToken"] = accessToken;

	ofstream fout;
	fout.open(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
	fout << config;
	fout.close();
	return 0;
}
int startCq()
{
	system(".\\start_go-cqhttp.bat");
	return 0;
}

int startqsign()
{
	system("start plugins\\X-Robot\\qsign\\bin\\unidbg-fetch-qsign.bat --basePath=plugins\\X-Robot\\qsign\\txlib\\8.9.63");
	return 0;
}


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
			}
			//cout <<"/////////////////////////////" << jsonmsg << "////////////////////////////////" << endl;
				// parse explicitly
			json jm;
			try
			{
				jm = json::parse(jsonmsg.begin(), jsonmsg.end());
			}
			catch (...) { }
			string userid;
			string username;
			INT64 groupid = 0;
			string role = "member";
			string notice_type;
			string post_type;
			string message;
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
			if (groupid == GROUPIDINT)
			{
				if (message == GBK_2_UTF8("开服") && OpCheck(userid, role) == true)
				{
					cout << "正在开服" << endl;
					thread lunchSrv(lunch);
					lunchSrv.detach();
				}
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
		cout << "Closed" << endl;
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

		con->append_header("Authorization", " "+ accessToken);
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

/*
void msgAPI::privateMsg(string QQnum, string msg)
{

}*/
void groupMsgSend(string group_id, string msg)
{
	msg = "{\"action\": \"send_group_msg\",\"params\": {\"group_id\": \"" + group_id + "\",\"message\": \"" + msg + "\",\"auto_escape\": \"false\",\"access_token=\": \"" + accessToken + "\" }}";
	endpoint.send(0, msg);
}
/*
void msgAPI::groupMsg(string group_id, string msg)
{
	thread groupMsgTh(groupMsgSend, group_id, msg);
	groupMsgTh.detach();
}*/


int main()
{
	system("chcp 936");

	fstream infoFile;
	Node config = LoadFile(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
	interval = config["heartbeat"]["interval"].as<int>();
	infoFile.open(".\\plugins\\X-Robot\\RobotInfo.json");
	infoFile >> info;
	GROUPIDINT = info["QQ_group_id"];
	serverName = info["serverName"];
	cq_ip = info["cq_ip"];
	start_mode = info["manager"]["start_mode"];
	bool aleadyConfig = info["manager"]["cqhttp_config"];
	accessToken = info["accessToken"];
	backupTime = info["manager"]["backup_interval"];
	QSign = info["manager"]["qsign"];
	config["account"]["sign-server"] = QSign;
	ofstream fout;
	fout.open(".\\plugins\\X-Robot\\go-cqhttp\\config.yml");
	fout << config;
	fout.close();
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
		if ((QSign.find("127.0.0.1") != QSign.npos) || (QSign.find("0.0.0.0") != QSign.npos))
		{
			thread tl(startqsign);
			tl.detach();

			string po = QSign.substr(QSign.find_last_of(":") + 1, QSign.length());
			ULONG poInt = strtoll(po.c_str(), NULL, 10);
			while (GetTcpPortState(poInt) == FALSE)
			{
				Sleep(1000);
			}
			cout << "检测到QSign启动，正在启动go-cqhttp" << endl;
		}
		Sleep(3000);
		thread tl(startCq);
		tl.detach();
		if ((cq_ip.find("127.0.0.1") != cq_ip.npos) || (cq_ip.find("0.0.0.0") != cq_ip.npos))
		{
			string po = cq_ip.substr(cq_ip.find_last_of(":") + 1, cq_ip.length());
			ULONG poInt = strtoll(po.c_str(), NULL, 10);
			while (GetTcpPortState(poInt) == FALSE)
			{
				Sleep(1000);
			}
			cout << "检测到go-cqhttp启动,启动逻辑单元" << endl;
		}
		Sleep(1000);
	}
	int id = endpoint.connect(cq_ip);
	if (id != -1) {
		std::cout << "> Created connection with id " << id << std::endl;
	}

	while (true)
	{
		Sleep(100);
	}
	return 0;
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