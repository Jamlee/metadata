#pragma once

#include <iostream>
#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>

#include "Easylogging.h"

using namespace std;

typedef struct {
	char* Ip;
	short int Port;
	bool IsMetadataServer;
} MetaDataServer, *PMetaDataServer;

// 检查 TCP 端口连通性
bool WINAPI CheckPortTCP(short int dwPort, char *ipAddressStr)
{
	el::Logger* logger = el::Loggers::getLogger("function");
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup function failed with error: %v\n", iResult);
		return false;
	}

	struct sockaddr_in client;
	int sock;
	client.sin_family = AF_INET;
	client.sin_port = htons(dwPort);
	client.sin_addr.s_addr = inet_addr(ipAddressStr);

	sock = (int)socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		logger->error("Socket function failed with error:%v", WSAGetLastError());
		WSACleanup();
		return false;
	}

	logger->info("Checking Port: %v:%v", ipAddressStr, dwPort);
	int result = connect(sock, (struct sockaddr *) &client, sizeof(client));
	if (result == SOCKET_ERROR) {
		logger->error("Socket function failed with error:%v", WSAGetLastError());
		WSACleanup();
		return false;
	}
	else
	{
		WSACleanup();
		closesocket(sock);
		return true;
	}
}

// 检查IP是否为有效MetaServer
DWORD WINAPI CheckMds(LPVOID lpParam) {
	PMetaDataServer mds = (PMetaDataServer)lpParam;
	if (CheckPortTCP(mds->Port, mds->Ip)) {
		mds->IsMetadataServer = true;
		return 0;
	}
	return -1;
}


class MetaData {

public:
	MetaData();
	bool GetServerIP(char* &validDhcpServerIp, short int port);

	el::Logger* _logger;
};

MetaData::MetaData() {
	_logger = el::Loggers::getLogger("metadata");
}

// 从适配器中读取有效的 DHCP 服务进行判断
bool MetaData::GetServerIP(char* &validDhcpServerIp, short int port) {

	IP_ADAPTER_INFO  *pAdapterInfo;
	ULONG            ulOutBufLen;
	DWORD            dwRetVal;

	HANDLE handleArr[20];
	DWORD  idArr[20];
	MetaDataServer* data[20];

	pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) != ERROR_SUCCESS) {
		_logger->fatal("GetAdaptersInfo call failed with %v", dwRetVal);
	}

	PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
	int ret = 1;
	int threadIndex = -1;
	while (pAdapter) {
		if (pAdapter->DhcpEnabled) {
			std::string addrStr(pAdapter->DhcpServer.IpAddress.String);
			if (!addrStr.empty()) {
				++threadIndex;
				// 采用 thread 的方式进行多个 DHCP 服务器的选择
				_logger->info("Find DHCP Server: %v", pAdapter->DhcpServer.IpAddress.String);
				data[threadIndex] = (MetaDataServer*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetaDataServer));
				data[threadIndex]->Ip = pAdapter->DhcpServer.IpAddress.String;
				data[threadIndex]->Port = port;
				data[threadIndex]->IsMetadataServer = false;

				handleArr[threadIndex] = CreateThread(
					NULL, 0, CheckMds, data[threadIndex], 0, &idArr[threadIndex]);
				if (handleArr[threadIndex] == NULL)
				{
					_logger->fatal("Create thread");
				}
			}
		}
		pAdapter = pAdapter->Next;
	}

	if (threadIndex >= 0) {
		WaitForMultipleObjects(threadIndex + 1, handleArr, TRUE, 2000);
		for (int i = 0; i <= threadIndex; i++)
		{
			CloseHandle(handleArr[i]);
			if (data[i] != NULL) {
				if (data[i]->IsMetadataServer) {
					ret = 0; // 成功找到 Server
					_logger->info("Detect %v:%v is open", data[i]->Ip, data[i]->Port);
					_logger->info("Ensure Metadata server ip: %v", data[i]->Ip);
					validDhcpServerIp = (char*)LocalAlloc(LPTR, strlen(data[i]->Ip));
					CopyMemory(validDhcpServerIp, data[i]->Ip, strlen(data[i]->Ip) * sizeof(char));
					HeapFree(GetProcessHeap(), 0, data[i]);
					data[i] = NULL;
				}
				else {
					_logger->info(" Detect %v:%v is not open", data[i]->Ip, data[i]->Port);
				}
			}
		}
	}

	if (pAdapterInfo)
		free(pAdapterInfo);

	return ret;
}