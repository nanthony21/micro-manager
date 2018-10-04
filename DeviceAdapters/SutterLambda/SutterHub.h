#pragma once
#include "../../MMDevice/DeviceBase.h"
class SutterHub : public HubBase<SutterHub>
{
public:
	SutterHub();
	~SutterHub();

	//Device API
	int Initialize();
	int Shutdown() { return DEVICE_OK; };
	void GetName(char* pName) const;
	bool Busy() { return busy_; };

	//Hub API
	int DetectInstalledDevices();

	//Action API
	int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);

	//From old sutterutil class
	bool ControllerBusy();
	int GoOnLine(unsigned long answerTimeoutMs);
	int GetControllerType(unsigned long answerTimeoutMs, std::string& type, std::string& id);
	int GetStatus(unsigned long answerTimeoutMs, unsigned char* status);
	int SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, const bool responseRequired = true, const bool CRexpected = true);
	int SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired = true, const bool CRExpected = true);
	// some commands don't send a \r!!!
	int SetCommandNoCR(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired = true);
private:
	const std::string port;

};

