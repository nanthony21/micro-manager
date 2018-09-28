#include "SutterLambda.h"

const char* g_ShutterModeProperty = "Mode";
const char* g_FastMode = "Fast";
const char* g_SoftMode = "Soft";
const char* g_NDMode = "ND";
const char* g_ControllerID = "Controller Info";

///////////////////////////////////////////////////////////////////////////////
// Shutter implementation
// ~~~~~~~~~~~~~~~~~~~~~~~

Shutter::Shutter(const char* name, int id) :
	initialized_(false),
	id_(id),
	name_(name),
	nd_(1),
	controllerType_("10-2"),
	controllerId_(""),
	answerTimeoutMs_(500),
	curMode_(g_FastMode)
{
	InitializeDefaultErrorMessages();
	SetErrorText(ERR_NO_ANSWER, "No Sutter Controller found.  Is it switched on and connected to the specified comunication port?");

	// create pre-initialization properties
	// ------------------------------------

	// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "Sutter Lambda shutter adapter", MM::String, true);

	// Port
	CPropertyAction* pAct = new CPropertyAction(this, &Shutter::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	EnableDelay();
}

Shutter::~Shutter()
{
	Shutdown();
}

void Shutter::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

int Shutter::Initialize()
{
	if (initialized_)
		return DEVICE_OK;

	newGlobals(port_);


	// set property list
	// -----------------

	// State
	// -----
	CPropertyAction* pAct = new CPropertyAction(this, &Shutter::OnState);
	int ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	AddAllowedValue(MM::g_Keyword_State, "0");
	AddAllowedValue(MM::g_Keyword_State, "1");


	int j = 0;
	while (GoOnLine() != DEVICE_OK && j < 4)
		j++;
	if (j >= 4)
		return ERR_NO_ANSWER;

	ret = GetControllerType(controllerType_, controllerId_);
	if (ret != DEVICE_OK)
		return ret;

	CreateProperty(g_ControllerID, controllerId_.c_str(), MM::String, true);
	std::string msg = "Controller reported ID " + controllerId_;
	LogMessage(msg.c_str());

	// Shutter mode
	// ------------
	if (controllerType_ == "SC" || controllerType_ == "10-3") {
		pAct = new CPropertyAction(this, &Shutter::OnMode);
		std::vector<std::string> modes;
		modes.push_back(g_FastMode);
		modes.push_back(g_SoftMode);
		modes.push_back(g_NDMode);

		CreateProperty(g_ShutterModeProperty, g_FastMode, MM::String, false, pAct);
		SetAllowedValues(g_ShutterModeProperty, modes);
		// set initial value
		//SetProperty(g_ShutterModeProperty, curMode_.c_str());

		// Neutral Density-mode Shutter (will not work with 10-2 controller)
		pAct = new CPropertyAction(this, &Shutter::OnND);
		CreateProperty("NDSetting", "1", MM::Integer, false, pAct);
		SetPropertyLimits("NDSetting", 1, 144);
	}

	unsigned char status[22];
	ret = SutterUtils::GetStatus(*this, *GetCoreCallback(), port_, 100, status);
	// note: some controllers will not know this command and return an error, 
	// so do not balk if we do not get an answer

	// status meaning is different on different controllers:
	if (controllerType_ == "10-3") {
		if (id_ == 0) {
			if (status[4] == 170)
				SetOpen(true);
			if (status[4] == 172)
				SetOpen(false);
			if (status[6] == 0xDC)
				curMode_ = g_FastMode;
			if (status[6] == 0xDD)
				curMode_ = g_SoftMode;
			if (status[6] == 0xDE)
				curMode_ = g_NDMode;
			if (curMode_ == g_NDMode)
				nd_ = (unsigned int)status[7];
		}
		else if (id_ == 1) {
			if (status[5] == 186)
				SetOpen(true);
			if (status[5] == 188)
				SetOpen(false);
			int offset = 0;
			if (status[6] == 0xDE)
				offset = 1;
			if (status[7 + offset] == 0xDC)
				curMode_ = g_FastMode;
			if (status[7 + offset] == 0xDD)
				curMode_ = g_SoftMode;
			if (status[7 + offset] == 0xDE)
				curMode_ = g_NDMode;
			if (curMode_ == g_NDMode)
				nd_ = (unsigned int)status[8 + offset];
		}
	}
	else if (controllerType_ == "SC") {
		if (status[0] == 170)
			SetOpen(true);
		if (status[0] == 172)
			SetOpen(false);
		if (status[1] == 0xDC)
			curMode_ = g_FastMode;
		if (status[1] == 0xDD)
			curMode_ = g_SoftMode;
		if (status[1] == 0xDE)
			curMode_ = g_NDMode;
		if (curMode_ == g_NDMode)
			nd_ = (unsigned int)status[2];
	}

	// Needed for Busy flag
	changedTime_ = GetCurrentMMTime();

	initialized_ = true;

	return DEVICE_OK;
}

int Shutter::Shutdown()
{
	if (initialized_)
	{
		initialized_ = false;
	}
	return DEVICE_OK;
}


bool Shutter::SupportsDeviceDetection(void)
{
	return true;
}

MM::DeviceDetectionStatus Shutter::DetectDevice(void)
{
	MM::DeviceDetectionStatus result = MM::Misconfigured;

	try
	{
		// convert into lower case to detect invalid port names:
		std::string test = port_;
		for (std::string::iterator its = test.begin(); its != test.end(); ++its)
		{
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
			MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str());

			if (DEVICE_OK == pS->Initialize())
			{
				int status = SutterUtils::GoOnLine(*this, *GetCoreCallback(), port_, nint(answerTimeoutMs_));
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



int Shutter::SetOpen(bool open)
{
	long pos;
	if (open)
		pos = 1;
	else
		pos = 0;
	return SetProperty(MM::g_Keyword_State, CDeviceUtils::ConvertToString(pos));
}

int Shutter::GetOpen(bool& open)
{
	char buf[MM::MaxStrLength];
	int ret = GetProperty(MM::g_Keyword_State, buf);
	if (ret != DEVICE_OK)
		return ret;
	long pos = atol(buf);
	pos == 1 ? open = true : open = false;

	return DEVICE_OK;
}
int Shutter::Fire(double /*deltaT*/)
{
	return DEVICE_UNSUPPORTED_COMMAND;
}

/**
* Sends a command to Lambda through the serial port.
*/
bool Shutter::SetShutterPosition(bool state)
{
	std::vector<unsigned char> command;
	std::vector<unsigned char> alternateEcho;

	// the Lambda intermittently SC echoes inverted commands!

	if (id_ == 0)
	{
		// shutter A
		command.push_back(state ? 170 : 172);
		alternateEcho.push_back(state ? 172 : 170);
	}
	else
	{
		// shutter B
		command.push_back(state ? 186 : 188);
		alternateEcho.push_back(state ? 188 : 186);
	}

	int ret = SutterUtils::SetCommand(*this, *GetCoreCallback(), port_, command, alternateEcho,
		(unsigned long)(0.5 + answerTimeoutMs_));

	// Start timer for Busy flag
	changedTime_ = GetCurrentMMTime();

	return (DEVICE_OK == ret) ? true : false;
}

bool Shutter::Busy()
{
	if (::g_Busy[port_])
		return true;

	if (GetDelayMs() > 0.0) {
		MM::MMTime interval = GetCurrentMMTime() - changedTime_;
		MM::MMTime delay(GetDelayMs()*1000.0);
		if (interval < delay) {
			return true;
		}
	}

	return false;
}

bool Shutter::ControllerBusy()
{
	return SutterUtils::ControllerBusy(*this, *GetCoreCallback(), port_, static_cast<long>(0.5 + answerTimeoutMs_));
}

bool Shutter::SetShutterMode(const char* mode)
{
	if (strcmp(mode, g_NDMode) == 0)
		return SetND(nd_);

	std::vector<unsigned char> command;
	std::vector<unsigned char> alternateEcho;

	if (0 == strcmp(mode, g_FastMode))
		command.push_back((unsigned char)220);
	else if (0 == strcmp(mode, g_SoftMode))
		command.push_back((unsigned char)221);

	if ("SC" != controllerType_)
		command.push_back((unsigned char)(id_ + 1));


	int ret = SutterUtils::SetCommand(*this, *GetCoreCallback(), port_, command, alternateEcho,
		(unsigned long)(0.5 + answerTimeoutMs_));
	return (DEVICE_OK == ret) ? true : false;
}



bool Shutter::SetND(unsigned int nd)
{
	std::vector<unsigned char> command;
	std::vector<unsigned char> alternateEcho;

	command.push_back((unsigned char)222);

	if ("SC" == controllerType_)
		command.push_back((unsigned char)nd);
	else
	{
		command.push_back((unsigned char)(id_ + 1));
		command.push_back((unsigned char)(nd));
	}

	int ret = SutterUtils::SetCommand(*this, *GetCoreCallback(), port_, command, alternateEcho,
		(unsigned long)(0.5 + answerTimeoutMs_));
	return (DEVICE_OK == ret) ? true : false;

}

int Shutter::GoOnLine()
{
	return SutterUtils::GoOnLine(*this, *GetCoreCallback(), port_, nint(answerTimeoutMs_));
}

int Shutter::GetControllerType(std::string& type, std::string& id)
{
	return SutterUtils::GetControllerType(*this, *GetCoreCallback(), port_, nint(answerTimeoutMs_),
		type, id);
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int Shutter::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
	}
	else if (eAct == MM::AfterSet)
	{
		long pos;
		pProp->Get(pos);

		// apply the value
		SetShutterPosition(pos == 0 ? false : true);
	}

	return DEVICE_OK;
}

int Shutter::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(port_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		if (initialized_)
		{
			// revert
			pProp->Set(port_.c_str());
			return ERR_PORT_CHANGE_FORBIDDEN;
		}

		pProp->Get(port_);
	}

	return DEVICE_OK;
}

int Shutter::OnMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(curMode_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		std::string mode;
		pProp->Get(mode);

		if (SetShutterMode(mode.c_str()))
			curMode_ = mode;
		else
			return ERR_UNKNOWN_SHUTTER_MODE;
	}

	return DEVICE_OK;
}

int Shutter::OnND(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set((long)nd_);
	}
	else if (eAct == MM::AfterSet)
	{
		std::string ts;
		pProp->Get(ts);
		std::istringstream os(ts);
		os >> nd_;
		if (curMode_ == g_NDMode) {
			SetND(nd_);
		}
	}

	return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Shutter implementation
// ~~~~~~~~~~~~~~~~~~~~~~~
#ifdef DefineShutterOnTenDashTwo
ShutterOnTenDashTwo::ShutterOnTenDashTwo(const char* name, int id) :
	initialized_(false),
	id_(id),
	name_(name),
	answerTimeoutMs_(500),
	curMode_(g_FastMode)
{
	InitializeDefaultErrorMessages();

	// create pre-initialization properties
	// ------------------------------------

	// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "Sutter Lambda shutter adapter", MM::String, true);

	// Port
	CPropertyAction* pAct = new CPropertyAction(this, &ShutterOnTenDashTwo::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	EnableDelay();
	//UpdateStatus();
}

ShutterOnTenDashTwo::~ShutterOnTenDashTwo()
{
	Shutdown();
}

void ShutterOnTenDashTwo::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

int ShutterOnTenDashTwo::Initialize()
{
	if (initialized_)
		return DEVICE_OK;


	newGlobals(port_);

	// set property list
	// -----------------

	// State
	// -----
	CPropertyAction* pAct = new CPropertyAction(this, &ShutterOnTenDashTwo::OnState);
	int ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	AddAllowedValue(MM::g_Keyword_State, "0");
	AddAllowedValue(MM::g_Keyword_State, "1");

	// Shutter mode
	// ------------
	pAct = new CPropertyAction(this, &ShutterOnTenDashTwo::OnMode);
	std::vector<std::string> modes;
	modes.push_back(g_FastMode);
	modes.push_back(g_SoftMode);

	CreateProperty(g_ShutterModeProperty, g_FastMode, MM::String, false, pAct);
	SetAllowedValues(g_ShutterModeProperty, modes);



	// Transfer to On Line
	unsigned char setSerial = (unsigned char)238;
	ret = WriteToComPort(port_.c_str(), &setSerial, 1);
	if (DEVICE_OK != ret)
		return ret;

	// set initial values
	SetProperty(g_ShutterModeProperty, curMode_.c_str());

	ret = UpdateStatus();
	if (ret != DEVICE_OK)
		return ret;


	// Needed for Busy flag
	changedTime_ = GetCurrentMMTime();

	initialized_ = true;

	return DEVICE_OK;
}

int ShutterOnTenDashTwo::Shutdown()
{
	if (initialized_)
	{
		initialized_ = false;
	}
	return DEVICE_OK;
}

int ShutterOnTenDashTwo::SetOpen(bool open)
{
	long pos;
	if (open)
		pos = 1;
	else
		pos = 0;
	return SetProperty(MM::g_Keyword_State, CDeviceUtils::ConvertToString(pos));
}

int ShutterOnTenDashTwo::GetOpen(bool& open)
{
	char buf[MM::MaxStrLength];
	int ret = GetProperty(MM::g_Keyword_State, buf);
	if (ret != DEVICE_OK)
		return ret;
	long pos = atol(buf);
	pos == 1 ? open = true : open = false;

	return DEVICE_OK;
}
int ShutterOnTenDashTwo::Fire(double /*deltaT*/)
{
	return DEVICE_UNSUPPORTED_COMMAND;
}

/**
* Sends a command to Lambda through the serial port.
*/
bool ShutterOnTenDashTwo::SetShutterPosition(bool state)
{
	bool ret = true;
	unsigned char command;
	if (id_ == 0)
	{
		// shutter A
		command = state ? 170 : 172;
	}
	else
	{
		// shutter B
		command = state ? 186 : 188;
	}

	// wait if the device is busy
	MMThreadGuard g(*(::gplocks_[port_]));

	if (::g_Busy[port_])
		LogMessage("busy entering SetShutterPosition", true);

	MM::MMTime startTime = GetCurrentMMTime();
	while (::g_Busy[port_] && (GetCurrentMMTime() - startTime) < (500 * 1000.0))	//Is the timeout really supposed to be 500 seconds?
	{
		CDeviceUtils::SleepMs(10);
	}

	if (g_Busy[port_])
		LogMessage(std::string("Sequence error, port ") + port_ + std::string("was busy!"), false);

	g_Busy[port_] = true;

	unsigned char msg[2];
	msg[0] = command;
	msg[1] = 13; // CR

	(void)PurgeComPort(port_.c_str());

	// send command
	if (DEVICE_OK != WriteToComPort(port_.c_str(), msg, 1))
		return false;

	// Start timer for Busy flag
	changedTime_ = GetCurrentMMTime();

	// block/wait for acknowledge, or until we time out;
	unsigned char answer = 0;
	unsigned long read;
	startTime = GetCurrentMMTime();

	// first we will see the echo
	startTime = GetCurrentMMTime();
	// read the response
	for (;;)
	{
		if (DEVICE_OK != ReadFromComPort(port_.c_str(), &answer, 1, read))
		{
			LogMessage("ReadFromComPort failed", false);
			return false;
		}
		if (answer == command)
		{
			ret = true;
			break;
		}
		else if (answer == 13) // CR
		{
			LogMessage("error, command was not echoed!", false);
			break;
		}
		else if (answer != 0)
		{
			std::ostringstream bufff;
			bufff << answer;
			LogMessage("unexpected response: " + bufff.str(), false);
		}
		MM::MMTime del2 = GetCurrentMMTime() - startTime;
		if (1000.0 * answerTimeoutMs_ < del2.getUsec())
		{
			std::ostringstream bufff;
			bufff << "answer timeout after " << del2.getUsec() << " microsec";
			LogMessage(bufff.str(), false);
			break;
		}
	}


	startTime = GetCurrentMMTime();
	answer = 0;
	// now look for a 13
	for (;;) {
		if (DEVICE_OK != ReadFromComPort(port_.c_str(), &answer, 1, read))
		{
			LogMessage("ReadFromComPort failed", false);
			return false;
		}
		if (answer == 13) // CR
		{
			ret = true;
			break;
		}
		if (0 < read)
		{
			LogMessage("error, extraneous response " + std::string((char*)&answer), false);
		}
		if (1000.0 * answerTimeoutMs_ < (GetCurrentMMTime() - startTime).getUsec())
		{
			std::ostringstream bufff;
			bufff << answerTimeoutMs_ * 2. << " msec";
			LogMessage("answer timeout after " + bufff.str(), false);
			break;
		}
	}

	g_Busy[port_] = false;

	return ret;
}

bool ShutterOnTenDashTwo::Busy()
{

	{
		std::auto_ptr<MMThreadGuard> pg(new MMThreadGuard(*(::gplocks_[port_])));
		if (::g_Busy[port_])
			return true;
	}

	if (GetDelayMs() > 0.0) {
		MM::MMTime interval = GetCurrentMMTime() - changedTime_;
		MM::MMTime delay(GetDelayMs()*1000.0);
		if (interval < delay) {
			return true;
		}
	}

	return false;
}

bool ShutterOnTenDashTwo::ControllerBusy()
{

	return g_Busy[port_];
}

bool ShutterOnTenDashTwo::SetShutterMode(const char* mode)
{
	unsigned char msg[3];
	//msg[0] = 0;

	if (strcmp(mode, g_FastMode) == 0)
		msg[0] = 220;
	else if (strcmp(mode, g_SoftMode) == 0)
		msg[0] = 221;
	else
		return false;

	msg[1] = (unsigned char)id_ + 1;
	msg[2] = 13; // CR

	// send command
	if (DEVICE_OK != WriteToComPort(port_.c_str(), msg, 3))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int ShutterOnTenDashTwo::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
	}
	else if (eAct == MM::AfterSet)
	{
		long pos;
		pProp->Get(pos);

		// apply the value
		SetShutterPosition(pos == 0 ? false : true);
	}

	return DEVICE_OK;
}

int ShutterOnTenDashTwo::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(port_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		if (initialized_)
		{
			// revert
			pProp->Set(port_.c_str());
			return ERR_PORT_CHANGE_FORBIDDEN;
		}

		pProp->Get(port_);
	}

	return DEVICE_OK;
}


int ShutterOnTenDashTwo::OnMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(curMode_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		std::string mode;
		pProp->Get(mode);

		if (SetShutterMode(mode.c_str()))
			curMode_ = mode;
		else
			return ERR_UNKNOWN_SHUTTER_MODE;
	}

	return DEVICE_OK;
}

#endif