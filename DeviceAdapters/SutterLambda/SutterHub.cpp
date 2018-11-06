#include "SutterLambda.h"


extern const char* g_HubName;
extern const char* g_WheelAName;
extern const char* g_WheelBName;
extern const char* g_WheelCName;
extern const char* g_ShutterAName;
extern const char* g_ShutterBName;
extern const char* g_LambdaVF5Name;

//Based heavily on the Arduino Hub.

SutterHub::SutterHub(const char* name): busy_(false), initialized_(false), name_(name)
{
	CPropertyAction* pAct = new CPropertyAction(this, &SutterHub::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	// Answertimeout
	pAct = new CPropertyAction(this, &SutterHub::OnAnswerTimeout);
	CreateProperty("Timeout(ms)", "500", MM::Integer, false, pAct, true);

	//Motors Enabled
	pAct = new CPropertyAction(this, &SutterHub::OnMotorsEnabled);
	CreateProperty("Motors Enabled", "1", MM::Integer, false, pAct, false);

	// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "Sutter Lambda Controller", MM::String, true);
}

SutterHub::~SutterHub()
{
	Shutdown();
}

int SutterHub::Initialize() {
	int ret = CreateProperty(MM::g_Keyword_Name, g_HubName, MM::String, true);
	if (ret != DEVICE_OK) { return ret; }

	MMThreadGuard myLock(GetLock());	//We are creating an object named MyLock. the constructor locks access to lock_. when myLock is destroyed it is released.
	PurgeComPort(port_.c_str());
	ret = GoOnline(); //Check that we're connected
	if (ret != DEVICE_OK) { return ret; }

	initialized_ = true;
	return DEVICE_OK;
}

int SutterHub::Shutdown() {
	initialized_ = false;
	return DEVICE_OK;
}

void SutterHub::GetName(char* name) const
{
	assert(name_.length() < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

MM::DeviceDetectionStatus SutterHub::DetectDevice() {
	if (initialized_) {
		return MM::CanCommunicate;
	}
	else {
		MM::DeviceDetectionStatus result = MM::Misconfigured;
		try {
			// convert into lower case to detect invalid port names:
			std::string test = port_;
			for (std::string::iterator its = test.begin(); its != test.end(); ++its) {
				*its = (char)tolower(*its);
			}
			// ensure we have been provided with a valid serial port device name
			if (0 < test.length() && 0 != test.compare("undefined") && 0 != test.compare("unknown"))
			{
				// the port property seems correct, so give it a try
				result = MM::CanNotCommunicate;
				// device specific default communication parameters
				GetCoreCallback()->SetSerialProperties(port_.c_str(),
														  "50.0",
														  "9600",
														  "0.0",
														  "Off",
														  "None",
														  "1");

				MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str()); //No idea what this does. Taken from shutter class
				if (DEVICE_OK == pS->Initialize())
				{
					int status = GoOnline();
					if (DEVICE_OK == status) {result = MM::CanCommunicate;}
					pS->Shutdown();
				}
				// but for operation, we'll need a longer timeout
				GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "2000.0");
			}
		}
		catch (...)
		{
			LogMessage("Exception in SutterHub DetectDevice");
		}
		return result;
	}
}

int SutterHub::DetectInstalledDevices() {
	if (DetectDevice() == MM::CanCommunicate) {
		std::vector<std::string> peripherals;
		peripherals.clear();
		peripherals.push_back(g_WheelAName);
		peripherals.push_back(g_WheelBName);
		peripherals.push_back(g_WheelCName);
		peripherals.push_back(g_ShutterAName);
		peripherals.push_back(g_ShutterBName);
		peripherals.push_back(g_LambdaVF5Name);
		for (size_t i=0; i<peripherals.size(); i++) {
			MM::Device* pDev = CreateDevice(peripherals[i].c_str());
			if (pDev) {
				AddInstalledDevice(pDev);
			}
		}
	}
	return DEVICE_OK;
}

bool SutterHub::Busy() {
	return busy_;
}

int SutterHub::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct) {
	if (pAct == MM::BeforeGet) {
		pProp->Set(port_.c_str());
	}
	else if (pAct == MM::AfterSet) {
		pProp->Get(port_);
	}
	return DEVICE_OK;
}

int SutterHub::OnAnswerTimeout(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set((long)timeout_);
	}
	else if (eAct == MM::AfterSet) {
		pProp->Get((long&)timeout_);
	}
	return DEVICE_OK;
}

int SutterHub::GoOnline() {
	// Transfer to On Line
	std::vector<unsigned char> cmd;
	cmd.push_back(238); //0xEE
	return SetCommand(cmd);
}

int SutterHub::GetControllerType(std::string& type, std::string& id) {
	PurgeComPort(port_.c_str());
	int ret = DEVICE_OK;
	std::vector<unsigned char> ans;
	std::vector<unsigned char> emptyv;
	std::vector<unsigned char> command;
	command.push_back((unsigned char)253); //0xFD

	ret = SetCommand(command, emptyv, ans);
	if (ret != DEVICE_OK) {return ret;}

	std::string ans2(ans.begin(), ans.end());

	if (ret != DEVICE_OK) {
		std::ostringstream errOss;
		errOss << "Could not get answer from 253 command (GetSerialAnswer returned " << ret << "). Assuming a 10-2";
		LogMessage(errOss.str().c_str(), true);
		type = "10-2";
		id = "10-2";
	}
	else if (ans2.length() == 0) {
		LogMessage("Answer from 253 command was empty. Assuming a 10-2", true);
		type = "10-2";
		id = "10-2";
	}
	else {
		if (ans2.substr(0, 2) == "SC") {
			type = "SC";
		}
		else if (ans2.substr(0, 4) == "10-3") {
			type = "10-3";
		}
		id = ans2.substr(0, ans2.length() - 2);
		LogMessage(("Controller type is " + std::string(type)).c_str(), true);
	}
	return DEVICE_OK;
}

/**
* Queries the controller for its status
* Meaning of status depends on controller type
* status should be allocated by the caller and at least be 21 bytes long
* status will be the answer returned by the controller, stripped from the first byte (which echos the command)
*/
int SutterHub::GetStatus(std::vector<unsigned char>& status) {
	std::vector<unsigned char> msg;
	msg.push_back(204);	//0xCC
	// send command
	int ret = SetCommand(msg,msg,status);
	return ret;
}

int SutterHub::OnMotorsEnabled(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::AfterSet) {
		long mEnabled;
		pProp->Get(mEnabled);
		std::vector<unsigned char> cmd;
		std::vector<unsigned char> response;
		cmd.push_back(0xCE | (unsigned char) mEnabled);
		SetCommand(cmd);
		mEnabled_ = (bool) mEnabled;
	}
	if (eAct == MM::BeforeGet){
		pProp->Set((long) mEnabled_);
	}
	return DEVICE_OK;
}

// lock the port for access,
// write 1, 2, or 3 char. command to equipment
// ensure the command completed by waiting for \r
// pass response back in argument
int SutterHub::SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, std::vector<unsigned char>& response) {
	bool responseReceived = false;
	MMThreadGuard myLock(GetLock());

	// start time of entire transaction
	MM::MMTime commandStartTime = GetCurrentMMTime();
	// write command to the port
	int ret = WriteToComPort(port_.c_str(), &command[0], (unsigned long)command.size());
	if (ret != DEVICE_OK) {return ret;}

	// now ensure that command is echoed from controller
	std::vector<unsigned char>::const_iterator allowedResponse = alternateEcho.begin();
	for (std::vector<unsigned char>::const_iterator irep = command.begin(); irep != command.end(); ++irep) {
		// block/wait for acknowledge, or until we time out;
		unsigned char answer = 0;
		unsigned long read;
		MM::MMTime responseStartTime = GetCurrentMMTime();
		// read the response
		int status = ReadFromComPort(port_.c_str(), &answer, 1, read);
		if (DEVICE_OK != status) {return status;} // give up - somebody pulled the serial hardware out of the computer
		if (read > 0){
			if (answer == *irep) {
			}
			else if (alternateEcho.end() != allowedResponse) {
				if (answer == *allowedResponse) {
					LogMessage(("command " + CDeviceUtils::HexRep(command) +
						" was echoed as alternate " + CDeviceUtils::HexRep(response)).c_str(), true);
					++allowedResponse;
				}
			}
			else {
				//We got an unexpected response
				std::ostringstream bufff;
				bufff.flags(std::ios::hex | std::ios::showbase);
				bufff << (unsigned int)answer;
				LogMessage((std::string("unexpected response: ") + bufff.str()).c_str(), false);
			}
			continue;
		}
		else {
			LogMessage("SutterHub Serial timed out", false);
			return DEVICE_ERR;
		}
	} // the command was echoed  entirely...
	MM::MMTime startTime = GetCurrentMMTime();
	// now look for a 13 - this indicates that the command has really completed!
	unsigned char answer = 0;
	unsigned long read;
	while (true) {
		answer = 0;
		int status = ReadFromComPort(port_.c_str(), &answer, 1, read);
		if (DEVICE_OK != status) {return status;}
		if (0 < read) {
			response.push_back(answer);
			if (answer == 13) { // CR  - sometimes the SC sends a 1 before the 13....
				responseReceived = true;
				return DEVICE_OK;
			}
			else {
				std::ostringstream bufff;
				bufff.flags(std::ios::hex | std::ios::showbase);
				bufff << (unsigned int)answer;
				LogMessage(("error, extraneous response  " + bufff.str()).c_str(), false);
			}
		}
		else {
			LogMessage("Sutter Serial Timed Out", false);
			return DEVICE_ERR;
		}

		MM::MMTime del2 = GetCurrentMMTime() - startTime;
		if (timeout_ < del2.getUsec()) {
			std::ostringstream bufff;
			MM::MMTime del3 = GetCurrentMMTime() - commandStartTime;
			bufff << "command completion timeout after " << del3.getUsec() << " microsec";
			LogMessage(bufff.str().c_str(), false);
			return DEVICE_ERR;
		}
	}
}

int SutterHub::SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> altEcho){
	std::vector<unsigned char> response;
	return SetCommand(command, altEcho, response);
}

int SutterHub::SetCommand(const std::vector<unsigned char> command) {
	return SetCommand(command,command);
}