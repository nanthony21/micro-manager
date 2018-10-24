#include "SutterLambda.h"

///////////////////////////////////////////////////////////////////////////////
// Wheel implementation
// ~~~~~~~~~~~~~~~~~~~~

Wheel::Wheel(const char* name, unsigned id, bool evenPositionsOnly, const char* description) :
	initialized_(false),
	numPos_(10),
	id_(id),
	name_(name),
	curPos_(0),
	speed_(3),
	evenPositionsOnly_(evenPositionsOnly),
	description_(description)
{
	assert(id == 0 || id == 1 || id == 2);
	assert(strlen(name) < (unsigned int)MM::MaxStrLength);

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
	CreateProperty(MM::g_Keyword_Description, description_, MM::String, true);

	UpdateStatus();
}

Wheel::~Wheel()
{
	Shutdown();
}

void Wheel::GetName(char* name) const
{
	assert(name_.length() < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

/**
* Kludgey implementation of the status check
*
*/
bool Wheel::Busy() {
	return hub_->Busy();
}

int Wheel::Initialize() {
	hub_ = dynamic_cast<SutterHub*>(GetParentHub());
	// set property list
	// -----------------

	// Gate Closed Position
	int ret = CreateProperty(MM::g_Keyword_Closed_Position, "", MM::String, false);

	// State
	// -----
	CPropertyAction* pAct = new CPropertyAction(this, &Wheel::OnState);
	ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// Speed
	// -----
	pAct = new CPropertyAction(this, &Wheel::OnSpeed);
	ret = CreateProperty(MM::g_Keyword_Speed, "3", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;
	for (int i = 0; i < 8; i++) {
		std::ostringstream os;
		os << i;
		AddAllowedValue(MM::g_Keyword_Speed, os.str().c_str());
	}

	// Label
	// -----
	pAct = new CPropertyAction(this, &CStateBase::OnLabel);
	ret = CreateProperty(MM::g_Keyword_Label, "Undefined", MM::String, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// Delay
	// -----
	pAct = new CPropertyAction(this, &Wheel::OnDelay);
	ret = CreateProperty(MM::g_Keyword_Delay, "0.0", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;



	// Busy
	// -----
	// NOTE: in the lack of better status checking scheme,
	// this is a kludge to allow resetting the busy flag.  
	pAct = new CPropertyAction(this, &Wheel::OnBusy);
	ret = CreateProperty("Busy", "0", MM::Integer, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// create default positions and labels
	const int bufSize = 1024;
	char buf[bufSize];
	int _ = evenPositionsOnly_ ? 2 : 1;
	for (unsigned i = 0; i < numPos_; i+=_)
	{
		snprintf(buf, bufSize, "Filter-%d", i);
		SetPositionLabel(i, buf);
		// Also give values for Closed-Position state while we are at it
		snprintf(buf, bufSize, "%d", i);
		AddAllowedValue(MM::g_Keyword_Closed_Position, buf);
	}

	GetGateOpen(open_);

	ret = UpdateStatus();
	if (ret != DEVICE_OK)
		return ret;

	initialized_ = true;
	return DEVICE_OK;
}

int Wheel::Shutdown()
{
	initialized_ = false;
	return DEVICE_OK;
}

/**
* Sends a command to Lambda through the serial port.
* The command format is one byte encoded as follows:
* bits 0-3 : position
* bits 4-6 : speed
* bit  7   : wheel id
*/

bool Wheel::SetWheelPosition(unsigned pos)
{
	// build the respective command for the wheel
	unsigned char upos = (unsigned char)pos;

	std::vector<unsigned char> command;

	// the Lambda 10-2, at least, SOMETIMES echos only the pos, not the command and speed - really confusing!!!
	std::vector<unsigned char> alternateEcho;

	switch (id_)
	{
	case 0:
		command.push_back((unsigned char)(speed_ * 16 + pos));
		alternateEcho.push_back(upos);
		break;
	case 1:
		command.push_back((unsigned char)(128 + speed_ * 16 + pos));
		alternateEcho.push_back(upos);
		break;
	case 2:
		alternateEcho.push_back(upos);    // TODO!!!  G.O.K. what they really echo...
		command.push_back((unsigned char)252); // used for filter C
		command.push_back((unsigned char)(speed_ * 16 + pos));
		break;
	default:
		break;
	}

	int ret2 = hub_->SetCommand(command, alternateEcho);
	return (DEVICE_OK == ret2) ? true : false;

}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int Wheel::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set((long)curPos_);
	}
	else if (eAct == MM::AfterSet)
	{
		bool gateOpen;
		GetGateOpen(gateOpen);
		long pos;
		pProp->Get(pos);

		if (pos >= (long)numPos_ || pos < 0)
		{
			pProp->Set((long)curPos_); // revert
			return ERR_UNKNOWN_POSITION;
		}

		// For efficiency, the previous state and gateOpen position is cached
		if (gateOpen) {
			// check if we are already in that state
			if (((unsigned)pos == curPos_) && open_)
				return DEVICE_OK;

			// try to apply the value
			if (!SetWheelPosition(pos))
			{
				pProp->Set((long)curPos_); // revert
				return ERR_SET_POSITION_FAILED;
			}
		}
		else {
			if (!open_) {
				curPos_ = pos;
				return DEVICE_OK;
			}

			char closedPos[MM::MaxStrLength];
			GetProperty(MM::g_Keyword_Closed_Position, closedPos);
			int gateClosedPosition = atoi(closedPos);

			if (!SetWheelPosition(gateClosedPosition))
			{
				pProp->Set((long)curPos_); // revert
				return ERR_SET_POSITION_FAILED;
			}
		}
		curPos_ = pos;
		open_ = gateOpen;
	}

	return DEVICE_OK;
}

int Wheel::OnSpeed(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set((long)speed_);
	}
	else if (eAct == MM::AfterSet)
	{
		long speed;
		pProp->Get(speed);
		if (speed < 0 || speed > 7)
		{
			pProp->Set((long)speed_); // revert
			return ERR_INVALID_SPEED;
		}
		speed_ = speed;
	}

	return DEVICE_OK;
}

int Wheel::OnDelay(MM::PropertyBase* pProp, MM::ActionType eAct)
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

int Wheel::OnBusy(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
			pProp->Set((long)hub_->Busy());
	}
	return DEVICE_OK;
}

