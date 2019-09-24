
#include "NKT_SuperK.h"


SuperKExtreme::SuperKExtreme(const char* name): name_(name)
{
	InitializeDefaultErrorMessages();
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");

	CPropertyAction* pAct = new CPropertyAction(this, &SutterHub::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	//Emission On
	pAct = new CPropertyAction(this, &SuperKExtreme::onEmission);
	CreatePropery("Emission Enabled", "False", MM::String, false, pAct, false);
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
	initialized_ = true;
	return DEVICE_OK;
}

int SuperKExtreme::Shutdown() {
	initialized_ = false;
	return DEVICE_OK;
}

void SuperKExtreme::GetName(char* pName) const {
	assert(name_.length() < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(name, name_.c_str());
}

bool SuperKExtreme::Busy();


//******Properties*******//
int SuperKExtreme::onEmission(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		int ret = registerReadU8(port_, address_, 0x30, -1); //TODO where does this get read to.
	}
	else if (eAct == MM:AfterSet) {
		std::string enabled;
		pProp->Get(enabled);
		if (enabled.compare("True") == 0) {
			int ret = registerWriteU8(port_, address_, 0x30, 3, -1);
			if (ret!=0) { return ret;}
		} else if (enabled.compare("False") == 0) {
			int ret = registerWriteU8(port_, address_, 0x30, 0, -1);
			if (ret!=0) { return ret;}
		} else {
			return 666;
		}
	}
	return DEVICE_OK;
}

int SuperKExtreme::onPower(MM::PropertyBase* pProp, MM::ActionType eAct) {

}

int SuperKExtreme::onInletTemperature(MM::PropertyBase* pProp, MM::ActionType eAct) {

}