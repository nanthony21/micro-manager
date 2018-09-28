#include "SutterLambda.h"

///////////////////////////////////////////////////////////////////////////////
// DG4Shutter implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~

DG4Shutter::DG4Shutter(const char* name) :
	initialized_(false),
	name_(name),
	answerTimeoutMs_(500)
{
	InitializeDefaultErrorMessages();

	// create pre-initialization properties
	// ------------------------------------

	// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "Sutter Lambda shutter adapter", MM::String, true);

	// Port
	CPropertyAction* pAct = new CPropertyAction(this, &DG4Shutter::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	UpdateStatus();
}

DG4Shutter::~DG4Shutter()
{
	Shutdown();
}

void DG4Shutter::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

int DG4Shutter::Initialize()
{
	// set property list
	// -----------------

	// State
	// -----

	newGlobals(port_);

	CPropertyAction* pAct = new CPropertyAction(this, &DG4Shutter::OnState);
	int ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	AddAllowedValue(MM::g_Keyword_State, "0");
	AddAllowedValue(MM::g_Keyword_State, "1");

	// Delay
	// -----
	pAct = new CPropertyAction(this, &DG4Shutter::OnDelay);
	ret = CreateProperty(MM::g_Keyword_Delay, "0.0", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	ret = UpdateStatus();
	if (ret != DEVICE_OK)
		return ret;

	SetProperty(MM::g_Keyword_State, "0");

	initialized_ = true;

	return DEVICE_OK;
}

int DG4Shutter::Shutdown()
{
	if (initialized_)
	{
		initialized_ = false;
	}
	return DEVICE_OK;
}

int DG4Shutter::SetOpen(bool open)
{
	long pos;
	if (open)
		pos = 1;
	else
		pos = 0;
	return SetProperty(MM::g_Keyword_State, CDeviceUtils::ConvertToString(pos));
}

int DG4Shutter::GetOpen(bool& open)
{
	char buf[MM::MaxStrLength];
	int ret = GetProperty(MM::g_Keyword_State, buf);
	if (ret != DEVICE_OK)
		return ret;
	long pos = atol(buf);
	pos == 1 ? open = true : open = false;

	return DEVICE_OK;
}
int DG4Shutter::Fire(double /*deltaT*/)
{
	return DEVICE_UNSUPPORTED_COMMAND;
}

/**
* Sends a command to Lambda through the serial port.
*/
bool DG4Shutter::SetShutterPosition(bool state)
{
	unsigned char command;

	if (state == false)
	{
		command = 0; // close
	}
	else
	{
		command = (unsigned char)g_DG4Position; // open
	}
	if (DEVICE_OK != WriteToComPort(port_.c_str(), &command, 1))
		return false;

	unsigned char answer = 0;
	unsigned long read;
	MM::MMTime startTime = GetCurrentMMTime();
	MM::MMTime now = startTime;
	MM::MMTime timeout(answerTimeoutMs_ * 1000);
	bool eol = false;
	bool ret = false;
	do {
		if (DEVICE_OK != ReadFromComPort(port_.c_str(), &answer, 1, read))
			return false;
		if (answer == command) {
			ret = true;
			g_DG4State = state;
		}
		if (answer == 13)
			eol = true;
		now = GetCurrentMMTime();
	} while (!(eol && ret) && ((now - startTime) < timeout));

	if (!ret)
		LogMessage("Shutter read timed out", true);

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int DG4Shutter::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
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

int DG4Shutter::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
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

int DG4Shutter::OnDelay(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(this->GetDelayMs());
	}
	else if (eAct == MM::AfterSet)
	{
		double delay;
		pProp->Get(delay);
		this->SetDelayMs(delay);
	}

	return DEVICE_OK;
}