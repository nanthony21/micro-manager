#include "SuperK.h"

extern std::map<uint8_t, const char*> g_devices;


SuperKHub::SuperKHub() {
	InitializeDefaultErrorMessages();
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");

	CPropertyAction* pAct = new CPropertyAction(this, &SuperKHub::onPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
}

SuperKHub::~SuperKHub() {
	Shutdown();
}

//******Device API******//
MM::DeviceDetectionStatus SuperKHub::DetectDevice() { //Micromanager sets the port_ variable and then tests by running this function.
	MM::DeviceDetectionStatus result = MM::Misconfigured;
	try {
		result = MM::CanNotCommunicate;
		unsigned short maxLen = 255;
		char ports[255];
		NKTPDLL::getAllPorts(ports, &maxLen);
		int ret =  NKTPDLL::openPorts(ports, 1, 1); //Scan all available ports and open the ones that are recognized as NKT devices
		if (ret!=0){return result;}
		char detectedPorts[255];
		NKTPDLL::getOpenPorts(detectedPorts, &maxLen); //string of comma separated port names
		if (strcmp(port_.c_str(), detectedPorts) == 0) {//We make the dangerous assumption here that there is only one port found and therefore no commas
			result = MM::CanCommunicate;
		}
		NKTPDLL::closePorts("");//Make sure to close the ports or micromanager won't be able ot open it's own port.
	}
	catch (...) {
		LogMessage("Exception in SuperKHub::DetectDevice");
	}
	return result;
}

//********Device API*********//
int SuperKHub::Initialize() {
	//TODO need to actually do a check here.
	return DEVICE_OK;
}

int SuperKHub::Shutdown() {
	return DEVICE_OK;
}

//*******Hub API*********//
int SuperKHub::DetectInstalledDevices() {
	unsigned char maxTypes = 255;
	unsigned char types[255]; 
	int ret = NKTPDLL::deviceGetAllTypes(port_.c_str(), types, &maxTypes);
	if (ret!=0) { return ret;}
	for (uint8_t i=0; i<maxTypes; i++) {
		if (types[i] == 0) { continue; } //No device detected at address `i`
		else {
			const char* devName = g_devices.at(types[i]);
			MM::Device* pDev = CreateDevice(devName);
			if (pDev) {
				AddInstalledDevice(pDev);
			}
		}
	}
	return DEVICE_OK;
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

std::string SuperKHub::getPort() {
	return port_;
}