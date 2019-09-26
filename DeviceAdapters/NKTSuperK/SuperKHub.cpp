#include "SuperK.h"

extern std::map<uint8_t, const char*> g_devices;


SuperKHub::SuperKHub() {
	InitializeDefaultErrorMessages();
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");

	CPropertyAction* pAct = new CPropertyAction(this, &SuperKHub::onPort);
	CreateProperty("Com Port", "Undefined", MM::String, false, pAct, true);

	//Find NKT ports and add them a property options.
	unsigned short maxLen = 255;
	char ports[255];
	NKTPDLL::getAllPorts(ports, &maxLen);
	int ret =  NKTPDLL::openPorts(ports, 1, 1); //Scan all available ports and open the ones that are recognized as NKT devices
	//if (ret!=0){return result;}
	char detectedPorts[255];
	NKTPDLL::getOpenPorts(detectedPorts, &maxLen); //string of comma separated port names
	NKTPDLL::closePorts("");//Make sure to close the ports or micromanager won't be able ot open it's own port.
	std::string detectedPortsStr(detectedPorts);
	size_t pos = 0;
	std::string token;
	std::string delimiter = ",";

	while (true) {
		pos = detectedPortsStr.find(delimiter);
		token = detectedPortsStr.substr(0, pos);
		AddAllowedValue("Com Port", token.c_str());
		detectedPortsStr.erase(0, pos + delimiter.length());
		if (pos == std::string::npos) {break;}
	}
}

SuperKHub::~SuperKHub() {
	Shutdown();
}

//********Device API*********//
int SuperKHub::Initialize() {
	int ret = NKTPDLL::openPorts(port_.c_str(), 1, 1); //Usage of the NKT sdk Dll requires we use their port opening mechanics rather than micromanager's.
	if (ret!=0){return ret;}
	return DEVICE_OK;
}

int SuperKHub::Shutdown() {
	int ret = NKTPDLL::closePorts(port_.c_str());
	if (ret!=0){return ret;}
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
			try{
				const char* devName = g_devices.at(types[i]); //Some of the NKT devices do not have a class in this device adapter. in that case pDev will be 0 and will not be added.
				SuperKDevice* pDev = dynamic_cast<SuperKDevice*>(CreateDevice(devName));
				if (pDev) {
					pDev->setNKTAddress(i);
					AddInstalledDevice(dynamic_cast<MM::Device*>(pDev));
				}
			} catch (const std::out_of_range& oor) {} // If a device type is in `types` but not in g_devices then an error will occur. this device isn't supported anyway though.
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
