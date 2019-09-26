#include "SuperK.h"

extern const char* g_VariaName;

SuperKVaria::SuperKVaria(): name_(g_VariaName), SuperKDevice(0x68) { 
	InitializeDefaultErrorMessages();
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");
		// Name
	CreateProperty(MM::g_Keyword_Name, name_.c_str(), MM::String, true);

	// Description
	CreateProperty(MM::g_Keyword_Description, "NKT Photonics SuperK Varia Filter", MM::String, true);
}

SuperKVaria::~SuperKVaria() {
	Shutdown();
}


//********Device API*********//
int SuperKVaria::Initialize() {
	hub_ = dynamic_cast<SuperKHub*>(GetParentHub());
	try {
		setNKTAddress(hub_ -> getDeviceAddress(this)); 
	} catch (const std::out_of_range& oor) {
		SetErrorText(99, "SuperKExtreme Laser was not found in the SuperKHub list of connected devices.");
		return 99;
	}

	//For some reason Micromanager tries accessing properties before initialization. For this reason we don't create properties until after initialization.
	//MonitorInput
	CPropertyAction* pAct = new CPropertyAction(this, &SuperKVaria::onMonitorInput);
	CreateProperty("MonitorInput", "0", MM::Float, true, pAct, false);

	//Bandwidth
	pAct = new CPropertyAction(this, &SuperKVaria::onBandwidth);
	CreateProperty("Bandwidth (nm)", "10", MM::Float, false, pAct, false);
	SetPropertyLimits("Bandwidth (nm)", 10, 100);

	//Wavelength
	pAct = new CPropertyAction(this, &SuperKVaria::onWavelength);
	CreateProperty("Wavelength", "632", MM::Float, false, pAct, false);
	SetPropertyLimits("Wavelength", 400, 850);

	//Short Wave Pass
	pAct = new CPropertyAction(this, &SuperKVaria::onSWP);
	CreateProperty("Short Wave Pass", "632", MM::Float, false, pAct, false);
	SetPropertyLimits("Short Wave Pass", 400, 850);

	//Long Wave Pass
	pAct = new CPropertyAction(this, &SuperKVaria::onLWP);
	CreateProperty("Long Wave Pass", "632", MM::Float, false, pAct, false);
	SetPropertyLimits("Long Wave Pass", 400, 850);

	return DEVICE_OK;
}

int SuperKVaria::Shutdown() {
	return DEVICE_OK;
}

void SuperKVaria::GetName(char* pName) const {
	assert(name_.length() < CDeviceUtils::GetMaxStringLength());
	CDeviceUtils::CopyLimitedString(pName, name_.c_str());
}

//******Properties*******//
int SuperKVaria::onMonitorInput(MM::PropertyBase* pProp, MM::ActionType eAct){
	if (eAct == MM::BeforeGet) {
		uint16_t val;
		NKTPDLL::registerReadU16(hub_->getPort().c_str(), getNKTAddress(), 0x13, &val, -1);
		float percent = ((float)val) / 10;//convert from units of 0.1% to %
		pProp->Set(percent);
	}
}

int SuperKVaria::onBandwidth(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set(bandwidth_);
	}
	if (eAct == MM::AfterSet) {
		pProp->Get(bandwidth_);
		//Check the four properties and make sure its ok.
		updateFilters();
	}
}

int SuperKVaria::onWavelength(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set(wavelength_);
	}
	if (eAct == MM::AfterSet) {
		pProp->Get(wavelength_);
		//Check the four properties and make sure its ok.
		updateFilters();
	}
}

int SuperKVaria::onLWP(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set(lwp_);
	}
	if (eAct == MM::AfterSet) {
		pProp->Get(lwp_);
		//Check the four properties and make sure its ok.
		updateFilters();
	}
}

int SuperKVaria::onSWP(MM::PropertyBase* pProp, MM::ActionType eAct) {
	if (eAct == MM::BeforeGet) {
		pProp->Set(swp_);
	}
	if (eAct == MM::AfterSet) {
		pProp->Get(swp_);
		//Check the four properties and make sure its ok.
		updateFilters();
	}
}

int SuperKVaria::updateFilters() {

}