#pragma once

#include <iostream>
#include <windows.h>
#include <lm.h>
#include "Easylogging.h"

using namespace std;

class ConfigApply {

public:
	ConfigApply(const wstring& computerName, const  wstring& existUsername, const wstring& password);
	bool ChangeComputerName();
	bool ChangeUserPassword();
	bool detectIsNeedReboot();
	bool detectIsNeedChangePassword();

private:
	wstring _computerName;
	wstring _existUsername;
	wstring _password;
	el::Logger* _logger;
};

ConfigApply::ConfigApply(const wstring& computerName, const  wstring& existUsername, const wstring& password):
	_computerName(computerName),
	_existUsername(existUsername),
	_password(password) {
	_logger = el::Loggers::getLogger("configapoly");
}

bool ConfigApply::ChangeComputerName() {
	int ret = SetComputerName(this->_computerName.c_str());
	int retex = SetComputerNameEx(ComputerNamePhysicalDnsHostname, this->_computerName.c_str());
	if (ret != 0 && retex != 0) {
		_logger->info("Change computer name success: %v", _computerName);
		TCHAR  infoBuf[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD  bufCharCount = MAX_COMPUTERNAME_LENGTH + 1;
		GetComputerName(infoBuf, &bufCharCount);
		std::wstring ws = std::wstring(infoBuf);
		_logger->info("Old computer name is: %v", ws);
		return true;
	}
	else {
		_logger->fatal("Change computer name fail: %v", _computerName);
		return false;
	}
}

bool ConfigApply::ChangeUserPassword() {
	USER_INFO_1003 sPassword;
	sPassword.usri1003_password = (LPWSTR)_password.c_str();
	int nStatus = NetUserSetInfo(L".", _existUsername.c_str(), 1003, (LPBYTE)&sPassword, NULL);
	if (nStatus == NERR_Success) {
		_logger->info("Change %v password success: %v", _existUsername, _password);
		return true;
	}
	else if (nStatus == NERR_UserNotFound) {
		_logger->fatal("%v not found", _existUsername);
		return false;
	}
	else {
		_logger->error("Change administrator password error: %v", nStatus);
		return false;
	}
}

bool ConfigApply::detectIsNeedReboot() {
	TCHAR  infoBuf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD  bufCharCount = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerName(infoBuf, &bufCharCount);
	std::wstring ws = std::wstring(infoBuf);
	return ws.compare(_computerName) != 0;
}

bool ConfigApply::detectIsNeedChangePassword() {
	wstring ws(L"saved_password");
	return _password.compare(ws) != 0;
}