#include "CrystalTechnology.h"
#define BREAK_ERR if (ret!=CTDriver::OK) { return ret; }

CTDriver::CTDriver(std::function<int(std::string)> serialSend, std::function<int(std::string&)> serialReceive):
	numChan_(1) //Assume one channel so we atleast have a value here if something goes weird while getting the number of channels in the constructor.
{
	this->tx_ = serialSend;
	this->rx_ = serialReceive;

	this->tx_("dds freq *");
	std::string response;
	int i = 0;
	while (true) {
		this->rx_(response);
		try {
			response = response.substr(0, 7);
		} catch (std::out_of_range& oor) {
			break; //This is likely what will happen when we reach the end of the response.
		}
		if (response.compare("Channel") == 0) {
			i++;
		} else {
			break; //This is not expected to ever happen.
		}
	}
	this->numChan_ = i;
}

int CTDriver::reset() {
	std::string cmd = "dds reset";
	return this->tx_(cmd);
}

int CTDriver::setFrequencyMhz(uint8_t chan, double freq) {
	std::string freqStr = std::to_string((long double) freq); //formatted without any weird characters
	return this->setFreq(chan, freqStr);
}

int CTDriver::setWavelengthNm(uint8_t chan, double wv) {
	std::string freqStr = std::to_string((long double) wv); 
	freqStr = "#" + freqStr; //add the wavelength command modifier
	return this->setFreq(chan, freqStr);
}

int CTDriver::setFreq(uint8_t chan, std::string freqStr) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	BREAK_ERR
	std::string str = "dds frequency " + chanStr + " " + freqStr;
	return this->tx_(str);
}

int CTDriver::getChannelStr(uint8_t chan, std::string& chanStr, bool allowWildcard) {
	//Used by other methods to get the string used to refer to a channel in a command string. sending 255 chan will result in referring to all channels simultaneously. This is only allowed if `allowWildcard` is true.
	if ((chan==255) && (allowWildcard)) {
		chanStr = "*";
		return CTDriver::OK;
	} else if (chan >= numChan_) {
		return CTDriver::INVALID_CHANNEL;
	}
	chanStr = std::to_string((unsigned long long) chan); //This weird datatype is required to satisfy VC2010 implementation of to_string
	return CTDriver::OK;
}

int CTDriver::setAmplitude(uint8_t chan,  unsigned int asf) {
	if (asf > 16383) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	BREAK_ERR
	std::string cmd = "dds amplitude " + chanStr + " " + std::to_string((unsigned long long) asf);
	return this->tx_(cmd);
}

int CTDriver::setGain(uint8_t chan,  unsigned int gain) {
	if (gain > 31) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	BREAK_ERR
	std::string cmd = "dds gain " + chanStr + " " + std::to_string((unsigned long long) gain);
	return this->tx_(cmd);
}

int CTDriver::setPhase(uint8_t chan, double phaseDegrees) {
	if ((phaseDegrees > 360.0) || (phaseDegrees < 0.0)) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	if (ret != CTDriver::OK) { return ret; }
	unsigned int val = (unsigned int) ((phaseDegrees / 360.0) * 16383);
	std::string cmd = "dds phase " + chanStr + " " + std::to_string((unsigned long long) val);
	return this->tx_(cmd);
}

int CTDriver::getPhase(uint8_t chan, double& phaseDegrees) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	BREAK_ERR
	std::string cmd = "dds phase " + chanStr;
	ret = this->tx_(cmd);
	BREAK_ERR
	std::string response;
	ret = this->rx_(response);
	BREAK_ERR
	//response is in form "Channel {x} @ {phaseDegrees}
	response = response.substr(12); //get rid of the prefix
	phaseDegrees = strtod(response.c_str(), NULL); //0 could indicate an actual 0 or an error, oh well
	return CTDriver::OK;
}

int CTDriver::getAmplitude(uint8_t chan, unsigned int& asf) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	BREAK_ERR
	std::string cmd = "dds amplitude " + chanStr;
	ret = this->tx_(cmd);
	BREAK_ERR
	std::string response;
	ret = this->rx_(response);
	BREAK_ERR
	//response is in form "Channel {x} @ {asf}
	response = response.substr(12); //get rid of the prefix
	asf = atoi(response.c_str());
	return CTDriver::OK;
}

int CTDriver::getFrequencyMhz(uint8_t chan, double& freq) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	BREAK_ERR
	std::string cmd = "dds frequency " + chanStr;
	ret = this->tx_(cmd);
	BREAK_ERR
	std::string response;
	ret = this->rx_(response);
	BREAK_ERR
	//response is in form "Channel {x} profile {x} frequency {freq scientific notation}Hz (Ftw {xxxxxx variable length})"
	response = response.substr(30); //remove prefix
	size_t pos = response.find("Hz");
	response = response.substr(0, pos); //remove suffix
	freq = strtod(response.c_str(), NULL); //0 could be an error or actual value.
	return CTDriver::OK;
}

int CTDriver::getWavelengthNm(uint8_t chan, double& wv) {
	double freq;
	int ret = this->getFrequencyMhz(chan, freq);
	BREAK_ERR
	
	ret = this->freqToWavelength(freq, wv);
	BREAK_ERR

	return CTDriver::OK;
}

int CTDriver::getTemperature(std::string sensorType, double& temp) {
	if ((sensorType.compare("o")!=0) || (sensorType.compare("a")!=0) || (sensorType.compare("c")!=0)) {
		return CTDriver::INVALID_VALUE;//sensor type must be "o"scillator, "a"mplifier, "c"rystal
	}
	//oscillator temp
	std::string cmd = "temperature read " + sensorType;
	int ret = this->tx_(cmd);
	BREAK_ERR
	std::string response;
	ret = this->rx_(response);
	BREAK_ERR
	size_t pos = response.find(" "); //find the first space
	response.erase(0, pos);
	pos = response.find(" "); //find the second space
	response.erase(0, pos);
	pos = response.find(" "); //find the third space
	double t = strtod(response.substr(pos).c_str(), NULL);
	if (t==0.0) {
		return CTDriver::ERR;
	} else {
		temp = t;
	}
	return CTDriver::OK;
}

int CTDriver::getBoardInfo(std::string& info) {
	std::string cmd = "boardid partnumber";
	int ret = this->tx_(cmd);
	BREAK_ERR
	std::string partNo;
	ret = this->rx_(partNo);
	BREAK_ERR

	cmd = "boardid ctiserialnumber";
	ret = this->tx_(cmd);
	BREAK_ERR
	std::string serial;
	ret = this->rx_(serial);
	BREAK_ERR
	info = partNo + serial;
	return CTDriver::OK;
}

int CTDriver::getTuningCoeff(std::string& coeffs) {
	int ret = this->tx_("calibration tuning *");
	BREAK_ERR
	ret = this->rx_(coeffs);
	BREAK_ERR
	return CTDriver::OK;
}

int CTDriver::freqToWavelength(double freq, double& wavelength) {
	std::string cmd = "calibration wave " + std::to_string((long double) freq);
	int ret = this->tx_(cmd);
	BREAK_ERR
	std::string response;
	ret = this->rx_(response);
	BREAK_ERR
	//Response is in the form "Wavelength {scientific notation number} nm" TODO parse the response and set Wavelength
	response = response.substr(11); //Get rid of "Wavelength " prefix
	response = response.substr(0, response.length()-3); //Get rid of " nm" suffix
	double wv = strtod(response.c_str(), NULL);
	if (wv == 0.0) { return CTDriver::ERR; } //Conversion failed
	wavelength = wv;
	return CTDriver::OK;
}

int CTDriver::wavelengthToFreq(double wavelength, double& freq) {
	std::string cmd = "calibration tune " + std::to_string((long double) wavelength);
	int ret = this->tx_(cmd);
	BREAK_ERR
	std::string response;
	ret = this->rx_(response);
	BREAK_ERR
	//Response is in the form "Frequency {scientific notation number}Hz (FTW {ftw})" TODO parse the response and set Wavelength
	response = response.substr(response.find(" ")); //Get rid of "Frequency " prefix
	response = response.substr(0, response.find(" ")); //Get rid of " (Ftw {XX})" suffix
	response = response.substr(0, response.length()-2); //Get rid of "Hz" suffix
	double f = strtod(response.c_str(), NULL);
	if (f == 0.0) { return CTDriver::ERR; } //Conversion failed
	freq = f;
	return CTDriver::OK;
}

CTDriverCyAPI::CTDriverCyAPI(std::string deviceSerial):
	CTDriver(std::bind(&CTDriverCyAPI::tx, this, std::placeholders::_1), std::bind(&CTDriverCyAPI::rx, this, std::placeholders::_1)),
	usbDev(NULL)
{
	std::wstring wideDeviceSerial = std::wstring(deviceSerial.begin(), deviceSerial.end()); //We have to convert back up to wstring for the string comparison to work.
	this->usbDev = new CCyUSBDevice();
	int devices = this->usbDev->DeviceCount();
	for (int i=0; i<devices; i++) {
		if (this->usbDev->Open(i)) {   // Open automatically  calls Close() when a new handle is opened.
			if (this->usbDev->SerialNumber == wideDeviceSerial) { //We found the matching device.
				return;
			}
		}
	}
	throw "Could not find a CyAPI usb device matching the serial number:" + deviceSerial; //If we got to the end of the list without finding our device then raise an error
}

int CTDriverCyAPI::tx(std::string cmd) {
	CCyControlEndPoint* ep = this->usbDev->ControlEndPt;
	cmd.append("\r");
	LONG length = cmd.length();
	if (ep->Write((PUCHAR) cmd.c_str(), length)) {//Blocking synchronous transfer of data
		return CTDriver::OK;
	} else {
		return CTDriver::ERR;
	}
}

int CTDriverCyAPI::rx(std::string& response) {
	CCyControlEndPoint* ep = this->usbDev->ControlEndPt;
	unsigned char buf[1024];
	LONG numRead;
	if (ep->Read(buf, numRead)) {
		response = std::string((char*) buf);
		return CTDriver::OK;
	} else {
		return CTDriver::ERR;
	}
}

std::map<std::string, CTDriver::DriverType> CTDriverCyAPI::getConnectedDevices() {
	//Returns a map of all found devices that match the vID of crystal technologies and the pID of an AODS RF driver. Map pairs are device serial number, device type
	CCyUSBDevice* usbDev = new CCyUSBDevice();
	int devices = usbDev->DeviceCount();
	std::map<std::string, CTDriver::DriverType> m = std::map<std::string, CTDriver::DriverType>();
	for (int i=0; i<devices; i++) {
		if (usbDev->Open(i)) {   // Open automatically  calls Close() if necessary
			if (usbDev->VendorID == 5831) { //Crystal technologies "AOTF Utilities Release Notes" states that this is the VID for their AOTF controllers
				std::wstring wstr = std::wstring(usbDev->SerialNumber);
				std::string serialNum = std::string(wstr.begin(), wstr.end()); // This is a dangerous conversion from wstring to string which could totally mangle things. However we want to use this number in string form later so idk what to do about it.
				int pid = usbDev->ProductID;//PIDs (old, new): OctalChannel (1, 17), QuadChannel (3, 19), SingleChannel (2, 18)
				if ((pid==1)||(pid==17)) {
					m[serialNum] = CTDriver::OctalType;
				} else if ((pid==2)||(pid==18)) {
					m[serialNum] = CTDriver::SingleType;
				} else if ((pid==3)||(pid==19)) {
					m[serialNum] = CTDriver::QuadType;
				}
			}
		}
	}
	usbDev->Close();
	return m;
}