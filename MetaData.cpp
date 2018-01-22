#include <iostream>
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
	// 元数据服务地址获取
	char* address = NULL;
	MetaData* md = new MetaData();
	bool ret = md->GetServerIP(address, 80);

	// 获取 metadata 信息
	// HttpRequest Request(L"JamLee UserAgent/1.0", L"", L"");
	// if (Request.SendRequest(L"baidu.com", L"GET", NULL, 0))
	// {
	// 	printf("%ls", Request.resHeader.c_str());
	//	if (!Request.resBody.empty())
	//		printf("%*s", Request.resBody.size(), (char*)&Request.resBody[0]);
	// }

	// 应用服务器配置
	// ConfigApply* ca = new ConfigApply(L"JAMLEE-PC",  L"Administrator", L"jam1994");
	// ca->ChangeComputerName();
	// ca->ChangeUserPassword();

	return 0;
}