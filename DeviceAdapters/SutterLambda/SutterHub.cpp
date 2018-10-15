#include "SutterLambda.h"

//Based heavily on the Arduino Hub.

SutterHub::SutterHub(const char* name): busy_(false), initialized_(false), name_(name)
{
	CPropertyAction* pAct = new CPropertyAction(this, &SutterHub::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	// Answertimeout
	pAct = new CPropertyAction(this, &SutterHub::OnAnswerTimeout);
	CreateProperty("Timeout(ms)", "500", MM::Integer, false, pAct, true);

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

	MMThreadGuard myLock(GetLock());	//We are creating an object name MyLock. the constructor locks access to lock_. when myLock is destroyed it is released.
	PurgeComPort(port_.c_str());
	ret = CheckDeviceisconnected();
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
				GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, "9600");
				GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
				GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Handshaking, "Off");

				// we can speed up detection with shorter answer timeout here
				GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "50.0");
				GetCoreCallback()->SetDeviceProperty(port_.c_str(), "DelayBetweenCharsMs", "0.0");
				MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str()); //No idea what this does. Taken from shutter class

				if (DEVICE_OK == pS->Initialize())
				{
					int status = GoOnline();
					if (DEVICE_OK == status)
						result = MM::CanCommunicate;
					pS->Shutdown();
				}
				// but for operation, we'll need a longer timeout
				GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "2000.0");
			}
		}
		catch (...)
		{
			LogMessage("Exception in DetectDevice");
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
		for (size_t i = 0; i < peripherals.size(); i++)
		{
			MM::Device* pDev = ::CreateDevice(peripherals[i].c_str());
			if (pDev)
			{
				AddInstalledDevice(pDev);
			}
		}
	}
	return DEVICE_OK;
}

bool SutterHub::Busy() {
	MMThreadGuard myLock(GetLock());
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
	unsigned char setSerial = (unsigned char)238;
	int ret = WriteToComPort(port_.c_str(), &setSerial, 1);
	if (DEVICE_OK != ret)
		return ret;

	unsigned char answer = 0;
	bool responseReceived = false;
	int unsigned long read;
	MM::MMTime startTime = GetCurrentMMTime();
	do {
		if (DEVICE_OK != ReadFromComPort(port_.c_str(), &answer, 1, read))
			return false;
		if (answer == 238)
			responseReceived = true;
	} while (!responseReceived && (GetCurrentMMTime() - startTime) < (timeout_));
	if (!responseReceived)
		return ERR_NO_ANSWER;
	return DEVICE_OK;
}

int SutterHub::GetControllerType(std::string& type, std::string& id) {
	PurgeComPort(port_.c_str());
	int ret = DEVICE_OK;
	std::vector<unsigned char> ans;
	std::vector<unsigned char> emptyv;
	std::vector<unsigned char> command;
	command.push_back((unsigned char)253);

	ret = SetCommand(command, emptyv, ans, true, false);
	if (ret != DEVICE_OK)
		return ret;

	std::string ans2;
	ret = GetSerialAnswer(port_.c_str(), "\r", ans2);

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
int SutterHub::GetStatus(unsigned char* status) {
	PurgeComPort(port_.c_str());
	unsigned char msg[1];
	msg[0] = 204;	//0xCC
	// send command
	int ret = WriteToComPort(port_.c_str(), msg, 1);
	if (ret != DEVICE_OK)
		return ret;

	unsigned char ans = 0;
	bool responseReceived = false;
	unsigned long read;
	MM::MMTime startTime = GetCurrentMMTime();
	do {
		if (DEVICE_OK != ReadFromComPort(port_.c_str(), &ans, 1, read))
			return false;
		/*if (read > 0)
		printf("Read char: %x", ans);*/
		if (ans == 204)
			responseReceived = true;
		CDeviceUtils::SleepMs(2);
	} while (!responseReceived && (GetCurrentMMTime() - startTime) < (timeout_));
	if (!responseReceived)
		return ERR_NO_ANSWER;

	responseReceived = false;
	int j = 0;
	startTime = GetCurrentMMTime();
	do {
		if (DEVICE_OK != ReadFromComPort(port_.c_str(), &ans, 1, read))
			return false;
		if (read > 0) {
			/* printf("Read char: %x", ans);*/
			status[j] = ans;
			j++;
			if (ans == '\r')
				responseReceived = true;
		}
		CDeviceUtils::SleepMs(2);
	} while (!responseReceived && (GetCurrentMMTime() - startTime) < (timeout_) && j < 22);
	if (!responseReceived)
		return ERR_NO_ANSWER;
	return DEVICE_OK;
}


// lock the port for access,
// write 1, 2, or 3 char. command to equipment
// ensure the command completed by waiting for \r
// pass response back in argument
int SutterHub::SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, std::vector<unsigned char>& response, const bool responseRequired, const bool CRExpected) {
	int ret = DEVICE_OK;
	bool responseReceived = false;
	MMThreadGuard myLock(GetLock());

	// start time of entire transaction
	MM::MMTime commandStartTime = GetCurrentMMTime();
	// write command to the port
	ret = WriteToComPort(port_.c_str(), &command[0], (unsigned long)command.size());

	if (DEVICE_OK == ret) {
		if (responseRequired) {
			// now ensure that command is echoed from controller
			bool expectCR = CRExpected;
			std::vector<unsigned char>::const_iterator allowedResponse = alternateEcho.begin();
			for (std::vector<unsigned char>::const_iterator irep = command.begin(); irep != command.end(); ++irep) {

				// block/wait for acknowledge, or until we time out;
				unsigned char answer = 0;
				unsigned long read;
				MM::MMTime responseStartTime = GetCurrentMMTime();
				// read the response
				for (;;) {
					answer = 0;
					int status = ReadFromComPort(port_.c_str(), &answer, 1, read);
					if (DEVICE_OK != status) {
						// give up - somebody pulled the serial hardware out of the computer
						LogMessage("ReadFromSerial failed", false);
						return status;
					}
					if (0 < read)
						response.push_back(answer);
					if (answer == *irep) {
						if (!CRExpected) {
							responseReceived = true;
						}
						break;
					}
					else if (answer == 13) { //CR
						if (CRExpected) {
							expectCR = false;
							LogMessage("error, command was not echoed!", false);
						}
						else {
							// todo: can 13 ever be a command echo?? (probably not - no 14 position filter wheels)
							;
						}
						break;
					}
					else if (answer != 0) {
						if (alternateEcho.end() != allowedResponse) {
							// command was echoed, after a fashion....
							if (answer == *allowedResponse) {
								LogMessage(("command " + CDeviceUtils::HexRep(command) +
									" was echoed as " + CDeviceUtils::HexRep(response)).c_str(), true);
								++allowedResponse;
								break;
							}
						}
						std::ostringstream bufff;
						bufff.flags(std::ios::hex | std::ios::showbase);

						bufff << (unsigned int)answer;
						LogMessage((std::string("unexpected response: ") + bufff.str()).c_str(), false);
						break;
					}
					MM::MMTime delta = GetCurrentMMTime() - responseStartTime;
					if (timeout_ < delta.getUsec()) {
						expectCR = false;
						std::ostringstream bufff;
						bufff << delta.getUsec() << " microsec";

						// in some cases it might be normal for the controller to not respond,
						// for example, whenever the command is the same as the previous command or when
						// go on-line sent to a controller already on-line
						LogMessage((std::string("command echo timeout after ") + bufff.str()).c_str(), !responseRequired);
						break;
					}
					CDeviceUtils::SleepMs(5);
					// if we are not at the end of the 'alternate' echo, iterate forward in that alternate echo string
					if (alternateEcho.end() != allowedResponse)
						++allowedResponse;
				} // loop over timeout
			} // the command was echoed  entirely...
			if (expectCR) {
				MM::MMTime startTime = GetCurrentMMTime();
				// now look for a 13 - this indicates that the command has really completed!
				unsigned char answer = 0;
				unsigned long read;
				for (;;) {
					answer = 0;
					int status = ReadFromComPort(port_.c_str(), &answer, 1, read);
					if (DEVICE_OK != status) {
						LogMessage("ReadFromComPort failed", false);
						return status;
					}
					if (0 < read)
						response.push_back(answer);
					if (answer == 13) { // CR  - sometimes the SC sends a 1 before the 13....
						responseReceived = true;
						break;
					}
					if (0 < read) {
						std::ostringstream bufff;
						bufff.flags(std::ios::hex | std::ios::showbase);
						bufff << (unsigned int)answer;
						LogMessage(("error, extraneous response  " + bufff.str()).c_str(), false);
					}

					MM::MMTime del2 = GetCurrentMMTime() - startTime;
					if (timeout_ < del2.getUsec()) {
						std::ostringstream bufff;
						MM::MMTime del3 = GetCurrentMMTime() - commandStartTime;
						bufff << "command completion timeout after " << del3.getUsec() << " microsec";
						LogMessage(bufff.str().c_str(), false);
						break;
					}
				}
			}
		} // response required
		else {// no response required / expected
			CDeviceUtils::SleepMs(5); // docs say controller echoes any command within 100 microseconds
			// 3 ms is enough time for controller to send 3 bytes @  9600 baud
			readunneededresponsehere();
		}
	}
	else{
		LogMessage("WriteToComPort failed!", false);
		return ret;
	}
	return responseRequired ? (responseReceived ? DEVICE_OK : DEVICE_ERR) : DEVICE_OK;
}
