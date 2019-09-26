
#include "SuperK.h"

extern const char* g_ExtremeName;

SuperKExtreme::SuperKExtreme(): name_(g_ExtremeName), SuperKDevice(0x60) {
	InitializeDefaultErrorMessages();
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");

	// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "NKT Photonics SuperK Extreme Laser", MM::String, true);
}

SuperKExtreme::~SuperKExtreme() {
	Shutdown();
}

//********Device API*********//
int SuperKExtreme::Initialize() {
	hub_ = dynamic_cast<SuperKHub*>(GetParentHub());
	try {
		setNKTAddress(hub_ -> getDeviceAddress(this)); 
	} catch (const std::out_of_range& oor) {
		SetErrorText(99, "SuperKExtreme Laser was not found in the SuperKHub list of connected devices.");
		return 99;
	}

	//For some reason Micromanager tries accessing properties before initialization. For this reason we don't create properties until after initialization.
	//Emission On
	CPropertyAction* pAct = new CPropertyAction(this, &SuperKExtreme::onEmission);
	CreateProperty("Emission Enabled", "False", MM::String, false, pAct, false);
	AddAllowedValue("Emission Enabled", "True");
	AddAllowedValue("Emission Enabled", "False");

	//Set Power
	pAct = new CPropertyAction(this, &SuperKExtreme::onPower);
	CreateProperty("Power %", "0", MM::Float, false, pAct, false);
	SetPropertyLimits("Power %", 0, 100);

	//Get InletTemperature
	pAct = new CPropertyAction(this, &SuperKExtreme::onInletTemperature);
	CreateProperty("Inlet Temperature (C)", "0", MM::Float, true, pAct, false); 
	
	return DEVICE_OK;
}

int SuperKExtreme::Shutdown() {
	SetProperty("Emission Enabled", "False");
	return DEVICE_OK;
}

void SuperKExtreme::GetName(char* pName) const {
	assert(name_.length() < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(pName, name_.c_str());
}


//******Properties*******//

int SuperKExtreme::onEmission(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		unsigned long statusBits;
		NKTPDLL::deviceGetStatusBits(hub_->getPort().c_str(), getNKTAddress(), &statusBits);
		if (statusBits & 0x0001){ //Bit 0 of statusBits indicates if emission is on.
			pProp->Set("True");
		} else {
			pProp->Set("False");
		}
	}
	else if (eAct == MM::AfterSet) {
		std::string enabled;
		pProp->Get(enabled);
		if (enabled.compare("True") == 0) {
			int ret = NKTPDLL::registerWriteU8(hub_->getPort().c_str(), getNKTAddress(), 0x30, 3, -1);
			if (ret!=0) { return ret;}
		} else if (enabled.compare("False") == 0) {
			int ret = NKTPDLL::registerWriteU8(hub_->getPort().c_str(), getNKTAddress(), 0x30, 0, -1);
			if (ret!=0) { return ret;}
		} else {
			return 666;
		}
	}
	return DEVICE_OK;
}

int SuperKExtreme::onPower(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		uint16_t val;
		NKTPDLL::registerReadU16(hub_->getPort().c_str(), getNKTAddress(), 0x37, &val, -1);
		pProp -> Set(((float) val) / 10); //Convert for units of 0.1C to 1C
	} else if (eAct == MM::AfterSet) {
		double power;
		pProp->Get(power);
		uint16_t val = (uint16_t)(power * 10); //Convert for units of C to 0.1C
		NKTPDLL::registerWriteU16(hub_->getPort().c_str(), getNKTAddress(), 0x37, val, -1);
	}
	return DEVICE_OK;
}

int SuperKExtreme::onInletTemperature(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int16_t val;
		NKTPDLL::registerReadS16(hub_->getPort().c_str(), getNKTAddress(), 0x11, &val, -1);
		pProp -> Set(((float) val) / 10); //Convert for units of 0.1C to 1C
	}
	return DEVICE_OK;
}
