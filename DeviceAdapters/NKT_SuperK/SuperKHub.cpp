#include "SuperK.h"

extern std::map<uint8_t, const char*> g_devices;

//******Device API******//
MM::DeviceDetectionStatus SuperKExtreme::DetectDevice() { //Micromanager sets the port_ variable and then tests by running this function.
	if (initialized_) {
		return MM::CanCommunicate;
	}
	else {
		MM::DeviceDetectionStatus result = MM::Misconfigured;
		try {
			result = MM:CanNotCommunicate
			int ret = openPorts(getAllPorts(), 1, 1); //Scan all available ports and open the ones that are recognized as NKT devices
			if (ret!=0){return ret;}
			std::string detectedPorts = getOpenPorts(); //string of comma separated port names
			if (strcmp(port_, detectedPorts) == 0) {//We make the dangerous assumption here that there is only one port found and therefore no commas
				result = MM::CanCommunicate
			}
		}
		catch (...) {
			LogMessage("Exception in SuperKHub::DetectDevice");
		}
		return result;
	}
}


//*******Hub API*********//
int SuperKHub::DetectInstalledDevices() {
	uint8 maxTypes = 255;
	unsigned char types[maxTypes]; 
	int ret = deviceGetAllTypes(port_, types*, maxTypes);
	if (ret!=0) { return ret;}
	for (uint8 i=0; i<maxTypes; i++) {
		if (types[i] == 0) { continue; } //No device detected at address `i`
		else {
			MM::Device* pDev = CreateDevice(g_devices[types[i]].c_str(), i);
			if (pDev) {
				AddInstalledDevice(pDev);
			}
		}
	}
}

//******Properties*******//
int SuperKHub::onPort(MM::PropertyBase* pProp, MM::ActionType pAct) {
	if (pAct == MM::BeforeGet) {
		pProp->Set(port_.c_str());
	}
	else if (pAct == MM::AfterSet) {
		pProp->Get(port_);
	}
	return DEVICE_OK;
}