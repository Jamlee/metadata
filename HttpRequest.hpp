#pragma once
#include <iostream>;
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

#include "Easylogging.h"

using namespace std;

class HttpRequest {

private:
	std::wstring _userAgent;

public:
	HttpRequest(const std::wstring&, const std::wstring&, const std::wstring&);
	bool SendRequest(const std::wstring&, const std::wstring&, void*, DWORD);

	std::wstring resHeader;
	std::vector<BYTE> resBody;
};

HttpRequest::HttpRequest(const std::wstring &userAgent, const std::wstring &proxyIp, const std::wstring &proxyPort):
	_userAgent(userAgent) {

}

bool HttpRequest::SendRequest(const std::wstring &url, const std::wstring &method, void *body, DWORD bodySize) {

	DWORD dwSize;
	DWORD dwDownloaded;
	DWORD headerSize = 0;
	BOOL  bResults = FALSE;
	HINTERNET hSession;
	HINTERNET hConnect;
	HINTERNET hRequest;

	resHeader.resize(0);
	resBody.resize(0);

	hSession = WinHttpOpen(_userAgent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (hSession)
		hConnect = WinHttpConnect(hSession, url.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
	else
		printf("session handle failed\n");

	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, method.c_str(), NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	else
		printf("connect handle failed\n");

	if (hRequest)
		bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, body, bodySize, 0, 0);
	else
		printf("request handle failed\n");

	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);
	if (bResults)
	{
		bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, WINHTTP_NO_OUTPUT_BUFFER, &headerSize, WINHTTP_NO_HEADER_INDEX);
		if ((!bResults) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
		{
			resHeader.resize(headerSize / sizeof(wchar_t));
			if (resHeader.empty())
			{
				bResults = TRUE;
			}
			else
			{
				bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, &resHeader[0], &headerSize, WINHTTP_NO_HEADER_INDEX);
				if (!bResults) headerSize = 0;
				resHeader.resize(headerSize / sizeof(wchar_t));
			}
		}
	}
	if (bResults)
	{
		do
		{
			// Check for available data.
			dwSize = 0;
			bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);
			if (!bResults)
			{
				printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
				break;
			}

			if (dwSize == 0)
				break;

			do
			{
				// Allocate space for the buffer.
				DWORD dwOffset = resBody.size();
				resBody.resize(dwOffset + dwSize);

				// Read the data.
				bResults = WinHttpReadData(hRequest, &resBody[dwOffset], dwSize, &dwDownloaded);
				if (!bResults)
				{
					printf("Error %u in WinHttpReadData.\n", GetLastError());
					dwDownloaded = 0;
				}

				resBody.resize(dwOffset + dwDownloaded);

				if (dwDownloaded == 0)
					break;

				dwSize -= dwDownloaded;
			} while (dwSize > 0);
		} while (true);
	}

	// Report any errors.
	if (!bResults)
		printf("Error %d has occurred.\n", GetLastError());

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return bResults;
}