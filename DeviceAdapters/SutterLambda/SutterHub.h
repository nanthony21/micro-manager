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
	static bool ControllerBusy(MM::Device& device, MM::Core& core, std::string port, unsigned long answerTimeoutMs);
	static int GoOnLine(MM::Device& device, MM::Core& core, std::string port, unsigned long answerTimeoutMs);
	static int GetControllerType(MM::Device& device, MM::Core& core, std::string port, unsigned long answerTimeoutMs, std::string& type, std::string& id);
	static int GetStatus(MM::Device& device, MM::Core& core, std::string port, unsigned long answerTimeoutMs, unsigned char* status);
	static int SetCommand(MM::Device& device, MM::Core& core, const std::string port, const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, const bool responseRequired = true, const bool CRexpected = true);
	static int SetCommand(MM::Device& device, MM::Core& core, const std::string port, const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired = true, const bool CRExpected = true);
	// some commands don't send a \r!!!
	static int SetCommandNoCR(MM::Device& device, MM::Core& core, const std::string port, const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired = true);

};

