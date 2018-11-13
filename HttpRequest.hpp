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
	el::Logger* _logger;

public:
	HttpRequest(const std::wstring&, const std::wstring&, const std::wstring&);
	bool SendRequest(const std::wstring &host, short int port, const std::wstring& path,
		const std::wstring &method,
		vector<const wchar_t *> header,
		void *body, DWORD bodySize);

	std::wstring resHeader;
	std::vector<BYTE> resBody;
};

HttpRequest::HttpRequest(const std::wstring &userAgent, const std::wstring &proxyIp, const std::wstring &proxyPort):
	_userAgent(userAgent) {
	_logger = el::Loggers::getLogger("metadata");
}

bool HttpRequest::SendRequest(const std::wstring &host, short int port, const std::wstring& path, 
		const std::wstring &method, 
		vector<const wchar_t *> headers,
		void *body, DWORD bodySize) {

	DWORD dwSize;
	DWORD dwDownloaded;
	DWORD headerSize = 0;
	BOOL  bResults = FALSE;
	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;

	resHeader.resize(0);
	resBody.resize(0);

	hSession = WinHttpOpen(_userAgent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (hSession)
		hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
	else
		_logger->fatal("session handle failed %v", GetLastError());

	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(), NULL,
			WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, NULL);
	else
		_logger->fatal("connect handle failed %v", GetLastError());

	if (hRequest)
		for (auto header : headers) {
			WinHttpAddRequestHeaders(hRequest,
				header,
				(ULONG)-1L,
				WINHTTP_ADDREQ_FLAG_ADD);
		}

	if (hRequest)
		bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, body, bodySize, 0, 0);
	else
		_logger->fatal("request handle failed %v", GetLastError());

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
				_logger->fatal("Error %v in WinHttpQueryDataAvailable", GetLastError());
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
					_logger->fatal("Error %v in WinHttpReadData", GetLastError());
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
		_logger->fatal("Error %v has occurred", GetLastError());

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return bResults;
}