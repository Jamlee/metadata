﻿#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <lm.h>
#include <iphlpapi.h>
#include <dhcpcsdk.h>
#include <winhttp.h>
#include <windows.h>

#include "Easylogging.h"
#include "HttpRequest.hpp"
#include "MetaData.hpp"
#include "ConfigApply.hpp"

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Dhcpcsvc.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "winhttp.lib")

using namespace std;

INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[])
{
	el::Logger* logger = el::Loggers::getLogger("main");

	wstring computerName;
	wstring adminPassword;

	// 元数据服务地址获取
	char* address = NULL;
	MetaData* md = new MetaData();
	while (!md->GetServerIP(address, 8080)) {
		logger->info("Wait a nic with dhcp configured");
		Sleep(1000);
	}

	if (strlen(address) == 0) {
		logger->fatal("Not found the metaserver");
	}
  
	string tmp(address);
	wstring metaDataServer(tmp.begin(), tmp.end());
	
	// 获取 computer name 信息
	HttpRequest comreq(L"Happy UserAgent/1.0", L"", L"");
	vector<const wchar_t *> comreqHeaders = {};
	if (comreq.SendRequest(metaDataServer, 80, L"/latest/local-hostname", L"GET", comreqHeaders, NULL, 0))
	{
		if (!comreq.resBody.empty()) {
			string name(comreq.resBody.begin(), comreq.resBody.end());
			wstring _computerName(name.begin(), name.end());
			computerName = _computerName;
			logger->info("Get computer name is [%v]", name);
		}
		else
			logger->fatal("Can not get computer name");
	}

	// 获取 administrator password 信息
	HttpRequest pwreq(L"Happy UserAgent/1.0", L"", L"");
	vector<const wchar_t *> pwreqHeaders = { L"DomU_Request: send_my_password" };
	if (pwreq.SendRequest(metaDataServer, 8080, L"/", L"GET", pwreqHeaders, NULL, 0))
	{
		if (!pwreq.resBody.empty()) {
			string password(pwreq.resBody.begin(), pwreq.resBody.end());
			wstring _password(password.begin(), password.end());
			adminPassword = _password;
			logger->info("Get administrator password is [%v]", password);
		}
		else
			logger->fatal("Can not get administrator password");
	}
	vector<const wchar_t *> pwsavedreqHeaders = { L"DomU_Request: saved_password" };
	if (pwreq.SendRequest(metaDataServer, 8080, L"/", L"GET", pwsavedreqHeaders, NULL, 0))
	{
		logger->info("Ensure password is saved");
	}

	// 应用服务器配置
	 ConfigApply* ca = new ConfigApply(computerName.c_str(),  L"Administrator", adminPassword.c_str());
	 if (ca->detectIsNeedChangePassword()) {
		 logger->info("i try to change password");
		 ca->ChangeUserPassword();
	 }
	 if (ca->detectIsNeedReboot()) {
		 ca->ChangeComputerName();
		 logger->info("i try to change computer name and reboot windows");
	 }

	return 0;
}