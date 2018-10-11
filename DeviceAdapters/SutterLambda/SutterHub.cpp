#include "SutterHub.h"

//Based heavily on the Arduino Hub.

SutterHub::SutterHub(): busy_(false), initialized_(false)
{
	CPropertyAction* pAct = new CPropertyAction(this, &SutterHub::onPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM:String, false, pAct, true);
}

SutterHub::~SutterHub()
{
	Shutdown();
}

int SutterHub::Initialize() {
	int ret = CreateProperty(MM:g_Keyword_Name, g_HubName, MM : String, true);
	if (ret != DEVICE_OK) { return ret; }

	MMThreadGuard myLock(GetLock());	//We are creating an object name MyLock. the constructor locks access to lock_. when myLock is destroyed it is released.
	PurgeComPort(port_.c_str());
	ret = CheckDevice is connected();
	if (ret != DEVICE_OK) { return ret; }

	initialized_ = true;
	return DEVICE_OK;
}

int SutterHub::Shutdown() {
	initialized_ = false;
	return DEVICE_OK;
}
MM::DeviceDetectionStatus SutterHub::DetectDevice() {
	if (initialized_) {
		return MM::CanCommunicate;
	}
	else ...
}

int SutterHub::DetectInstalledDevices() {
	if (DetectDevice() == MM:CanCommunicate) {
		std::vector<std::string> peripherals;
		peripherals.clear();
		peripherals.push_back(g_WheelAName);
		peripherals.push_back(g_WheelBName);
		peripherals.push_back(g_WheelCName);
		peripherals.push_back(g_ShutterAName);
		peripherals.push_back(g_ShutterBName);
		peripherals.push_back(g_DG4WheelName);
		peripherals.push_back(g_DG4ShutterName);
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
	return DEVICE_OK
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

bool ControllerBusy();
int GoOnLine(unsigned long answerTimeoutMs);
int GetControllerType(unsigned long answerTimeoutMs, std::string& type, std::string& id);
int GetStatus(unsigned long answerTimeoutMs, unsigned char* status);


// lock the port for access,
// write 1, 2, or 3 char. command to equipment
// ensure the command completed by waiting for \r
// pass response back in argument
int SutterHub::SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired, const bool CRExpected) {
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
				MM::MMTime responseStartTime = core.GetCurrentMMTime();
				// read the response
				for (;;) {
					answer = 0;
					int status = core.ReadFromSerial(&device, port.c_str(), &answer, 1, read);
					if (DEVICE_OK != status) {
						// give up - somebody pulled the serial hardware out of the computer
						core.LogMessage(&device, "ReadFromSerial failed", false);
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
							expectA13 = false;
							core.LogMessage(&device, "error, command was not echoed!", false);
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

								core.LogMessage(&device, ("command " + CDeviceUtils::HexRep(command) +
									" was echoed as " + CDeviceUtils::HexRep(response)).c_str(), true);
								++allowedResponse;
								break;
							}
						}
						std::ostringstream bufff;
						bufff.flags(std::ios::hex | std::ios::showbase);

						bufff << (unsigned int)answer;
						core.LogMessage(&device, (std::string("unexpected response: ") + bufff.str()).c_str(), false);
						break;
					}
					MM::MMTime delta = core.GetCurrentMMTime() - responseStartTime;
					if (1000.0 * answerTimeoutMs < delta.getUsec()) {
						expectA13 = false;
						std::ostringstream bufff;
						bufff << delta.getUsec() << " microsec";

						// in some cases it might be normal for the controller to not respond,
						// for example, whenever the command is the same as the previous command or when
						// go on-line sent to a controller already on-line
						core.LogMessage(&device, (std::string("command echo timeout after ") + bufff.str()).c_str(), !responseRequired);
						break;
					}
					CDeviceUtils::SleepMs(5);
					// if we are not at the end of the 'alternate' echo, iterate forward in that alternate echo string
					if (alternateEcho.end() != allowedResponse)
						++allowedResponse;
				} // loop over timeout
			} // the command was echoed  entirely...
			if (expectA13) {
				MM::MMTime startTime = core.GetCurrentMMTime();
				// now look for a 13 - this indicates that the command has really completed!
				unsigned char answer = 0;
				unsigned long read;
				for (;;) {
					answer = 0;
					int status = core.ReadFromSerial(&device, port.c_str(), &answer, 1, read);
					if (DEVICE_OK != status) {
						core.LogMessage(&device, "ReadFromComPort failed", false);
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
						core.LogMessage(&device, ("error, extraneous response  " + bufff.str()).c_str(), false);
					}

					MM::MMTime del2 = core.GetCurrentMMTime() - startTime;
					if (1000.0 * answerTimeoutMs < del2.getUsec()) {
						std::ostringstream bufff;
						MM::MMTime del3 = core.GetCurrentMMTime() - commandStartTime;
						bufff << "command completion timeout after " << del3.getUsec() << " microsec";
						core.LogMessage(&device, bufff.str().c_str(), false);
						break;
					}
				}
			}
		} // response required
		else {// no response required / expected
			CDeviceUtils::SleepMs(5); // docs say controller echoes any command within 100 microseconds
			// 3 ms is enough time for controller to send 3 bytes @  9600 baud
			read unneeded response here.
		}
	}
	else{
		LogMessage("WriteToComPort failed!", false);
		return ret;
	}
	return responseRequired ? (responseReceived ? DEVICE_OK : DEVICE_ERR) : DEVICE_OK;
}