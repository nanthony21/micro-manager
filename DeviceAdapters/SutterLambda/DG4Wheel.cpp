#include "SutterLambda.h"

///////////////////////////////////////////////////////////////////////////////
// DG4Wheel implementation
// ~~~~~~~~~~~~~~~~~~~~~~~

DG4Wheel::DG4Wheel() :
	initialized_(false),
	numPos_(13),
	name_(g_DG4WheelName),
	curPos_(0),
	answerTimeoutMs_(500)
{
	InitializeDefaultErrorMessages();

	// add custom MTB messages
	SetErrorText(ERR_UNKNOWN_POSITION, "Position out of range");
	SetErrorText(ERR_PORT_CHANGE_FORBIDDEN, "You can't change the port after device has been initialized.");
	SetErrorText(ERR_INVALID_SPEED, "Invalid speed setting. You must enter a number between 0 to 7.");
	SetErrorText(ERR_SET_POSITION_FAILED, "Set position failed.");

	// create pre-initialization properties
	// ------------------------------------

	// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "Sutter Lambda filter wheel adapter", MM::String, true);

	// Port
	CPropertyAction* pAct = new CPropertyAction(this, &DG4Wheel::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	UpdateStatus();
}

DG4Wheel::~DG4Wheel()
{
	Shutdown();
}

void DG4Wheel::GetName(char* name) const
{
	assert(name_.length() < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

bool DG4Wheel::Busy()
{
	// TODO: Implement as for shutter and wheel (ASCII 13 is send as a completion message)
	return false;
}

int DG4Wheel::Initialize()
{

	newGlobals(port_);

	// set property list
	// -----------------

	// State
	// -----
	CPropertyAction* pAct = new CPropertyAction(this, &DG4Wheel::OnState);
	int ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// Label
	// -----
	pAct = new CPropertyAction(this, &CStateBase::OnLabel);
	ret = CreateProperty(MM::g_Keyword_Label, "Undefined", MM::String, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// Delay
	// -----
	pAct = new CPropertyAction(this, &DG4Wheel::OnDelay);
	ret = CreateProperty(MM::g_Keyword_Delay, "0.0", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// Busy
	// -----
	// NOTE: in the lack of better status checking scheme,
	// this is a kludge to allow resetting the busy flag.  
	pAct = new CPropertyAction(this, &DG4Wheel::OnBusy);
	ret = CreateProperty("Busy", "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// create default positions and labels
	const int bufSize = 1024;
	char buf[bufSize];
	for (unsigned i = 0; i < numPos_; i++)
	{
		snprintf(buf, bufSize, "Filter-%d", i);
		SetPositionLabel(i, buf);
	}

	ret = UpdateStatus();
	if (ret != DEVICE_OK)
		return ret;

	unsigned char setSerial = (unsigned char)238;
	ret = WriteToComPort(port_.c_str(), &setSerial, 1);
	if (DEVICE_OK != ret)
		return ret;

	SetProperty(MM::g_Keyword_State, "0");

	initialized_ = true;

	return DEVICE_OK;
}

int DG4Wheel::Shutdown()
{
	if (initialized_)
	{
		initialized_ = false;
	}
	return DEVICE_OK;
}

/**
* Sends a command to Lambda through the serial port.
*/
bool DG4Wheel::SetWheelPosition(unsigned pos)
{
	unsigned char command = (unsigned char)pos;

	if (g_DG4State == false)
		return true; // assume that position has been set (shutter is closed)

	 // send command
	if (DEVICE_OK != WriteToComPort(port_.c_str(), &command, 1))
		return false;

	// block/wait for acknowledge, or until we time out;
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
		if (answer == command)
			ret = true;
		if (answer == 13)
			eol = true;
		now = GetCurrentMMTime();
	} while (!(eol && ret) && ((now - startTime) < timeout));

	if (!ret)
		LogMessage("Wheel read timed out", true);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int DG4Wheel::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set((long)curPos_);
	}
	else if (eAct == MM::AfterSet)
	{
		long pos;
		pProp->Get(pos);

		//check if we are already in that state
		if ((unsigned)pos == curPos_)
			return DEVICE_OK;

		if (pos >= (long)numPos_ || pos < 0)
		{
			pProp->Set((long)curPos_); // revert
			return ERR_UNKNOWN_POSITION;
		}

		// try to apply the value
		if (!SetWheelPosition(pos))
		{
			pProp->Set((long)curPos_); // revert
			return ERR_SET_POSITION_FAILED;
		}
		curPos_ = pos;
		g_DG4Position = curPos_;
	}

	return DEVICE_OK;
}

int DG4Wheel::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
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

int DG4Wheel::OnDelay(MM::PropertyBase* pProp, MM::ActionType eAct)
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

int DG4Wheel::OnBusy(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		MMThreadGuard g(*(::gplocks_[port_]));
		pProp->Set(g_Busy[port_] ? 1L : 0L);
	}
	else if (eAct == MM::AfterSet)
	{
		MMThreadGuard g(*(::gplocks_[port_]));
		long b;
		pProp->Get(b);
		g_Busy[port_] = !(b == 0);
	}

	return DEVICE_OK;
}