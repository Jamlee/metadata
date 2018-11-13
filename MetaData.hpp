#pragma once

#include <iostream>
#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include "Easylogging.h"

using namespace std;

const wchar_t *GetWC(const char *c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	size_t converted = 0;
	mbstowcs_s(&converted, wc, cSize, c, _TRUNCATE);
	return wc;
}

typedef struct {
	char* Ip;
	short int Port;
	bool IsMetadataServer;
} MetaDataServer, *PMetaDataServer;

// 妫€鏌?TCP 绔彛杩為€氭€?
bool WINAPI CheckPortTCP(short int dwPort, char *ipAddressStr)
{
	el::Logger* logger = el::Loggers::getLogger("function");
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		logger->error("WSAStartup function failed with error:%v", iResult);
		return false;
	}

	struct sockaddr_in client;
	int sock;
	client.sin_family = AF_INET;
	client.sin_port = htons(dwPort);
	InetPton(AF_INET, GetWC(ipAddressStr), &client.sin_addr.s_addr);

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

// 妫€鏌P鏄惁涓烘湁鏁圡etaServer
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

// 浠庨€傞厤鍣ㄤ腑璇诲彇鏈夋晥鐨?DHCP 鏈嶅姟杩涜鍒ゆ柇
bool MetaData::GetServerIP(char* &validDhcpServerIp, short int port) {

	IP_ADAPTER_INFO  *pAdapterInfo;
	ULONG            ulOutBufLen;
	DWORD            dwRetVal;

	HANDLE handleArr[20];
	DWORD  idArr[20];
	MetaDataServer* data[20];

	pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	while (true) {
		// 閫氳繃overflow鎶ラ敊鑾峰彇鐪熸闇€瑕佺殑瀛樺偍绌洪棿
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
			free(pAdapterInfo); // 閲嶆柊鍒嗛厤瓒冲鐨勭┖闂?
			pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
		}
		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) != ERROR_SUCCESS) {
			_logger->error("GetAdaptersInfo call failed with %v", dwRetVal);
			_logger->info("Wait %v second and retry", dwRetVal);
			Sleep(500);
		}
		else {
			break;
		}
	}

	PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
	int ret = false;
	int threadIndex = -1;
	while (pAdapter) {
		if (pAdapter->DhcpEnabled) {
			std::string addrStr(pAdapter->DhcpServer.IpAddress.String);
			if (!addrStr.empty()) {
				++threadIndex;
				// 閲囩敤 thread 鐨勬柟寮忚繘琛屽涓?DHCP 鏈嶅姟鍣ㄧ殑閫夋嫨
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
					ret = true; // 鎴愬姛鎵惧埌 Server
					_logger->info("Detect %v:%v is open", data[i]->Ip, data[i]->Port);
					_logger->info("Ensure Metadata server ip: %v", data[i]->Ip);
					validDhcpServerIp = (char*)LocalAlloc(LPTR, strlen(data[i]->Ip));
					CopyMemory(validDhcpServerIp, data[i]->Ip, strlen(data[i]->Ip) * sizeof(char));
					HeapFree(GetProcessHeap(), 0, data[i]);
					data[i] = NULL;
				}
				else {
					_logger->info("Detect %v:%v is not open", data[i]->Ip, data[i]->Port);
				}
			}
		}
	}

	if (pAdapterInfo)
		free(pAdapterInfo);

	return ret;
}