
#include "SuperK.h"

extern const char* g_ExtremeName;

SuperKExtreme::SuperKExtreme(): name_(g_ExtremeName) {
	InitializeDefaultErrorMessages();
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");


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
	CreateProperty("Inlet Temperature", "0", MM::Float, true, pAct, false); 

	// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "NKT Photonics SuperK Extreme Laser", MM::String, true);
}

//********Device API*********//
int SuperKExtreme::Initialize() {
	hub_ = dynamic_cast<SuperKHub*>(GetParentHub());
	initialized_ = true;
	return DEVICE_OK;
}

int SuperKExtreme::Shutdown() {
	initialized_ = false;
	return DEVICE_OK;
}

void SuperKExtreme::GetName(char* pName) const {
	assert(name_.length() < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(pName, name_.c_str());
}


//******Properties*******//
int SuperKExtreme::onEmission(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		uint8_t val;
		int ret = NKTPDLL::registerReadU8(hub_->getPort().c_str(), address_, 0x30, &val, -1); //TODO where does this get read to.
		if (val==3){
			pProp->Set("True");
		} else { //Val should be 0
			pProp->Set("False");
		}
	}
	else if (eAct == MM::AfterSet) {
		std::string enabled;
		pProp->Get(enabled);
		if (enabled.compare("True") == 0) {
			int ret = NKTPDLL::registerWriteU8(hub_->getPort().c_str(), address_, 0x30, 3, -1);
			if (ret!=0) { return ret;}
		} else if (enabled.compare("False") == 0) {
			int ret = NKTPDLL::registerWriteU8(hub_->getPort().c_str(), address_, 0x30, 0, -1);
			if (ret!=0) { return ret;}
		} else {
			return 666;
		}
	}
	return DEVICE_OK;
}

int SuperKExtreme::onPower(MM::PropertyBase* pProp, MM::ActionType eAct) {
	return DEVICE_OK;
}

int SuperKExtreme::onInletTemperature(MM::PropertyBase* pProp, MM::ActionType eAct) {
	return DEVICE_OK;
}