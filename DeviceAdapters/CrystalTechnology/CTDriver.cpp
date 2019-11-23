#include "CrystalTechnology.h"
#define BREAK_ERR if (ret!=CTDriver::OK) { return ret; }
#define CHECKINIT if (!this->initialized) { return CTDriver::NOT_INITIALIZED; }

CTDriver::CTDriver(std::function<int(std::string)> serialSend, std::function<int(std::string&)> serialReceive):
	numChan_(1), //Assume one channel so we atleast have a value here if something goes weird while getting the number of channels in the constructor.
	initialized(false)
{
	this->tx_ = serialSend;
	this->rx_ = serialReceive;
}

int CTDriver::initialize() {
	this->clearPort();
	int ret = this->tx_("dds freq *");
	BREAK_ERR
	std::string response;
	uint8_t i = 0;
	while (true) {
		ret = this->rx_(response);
		BREAK_ERR
		if (response.compare("\n")==0) { continue; } //Sometimes it sends a new line for no reason.
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
	this->initialized = true;
	return CTDriver::OK;
}

int CTDriver::reset() {
	CHECKINIT
	std::string cmd = "dds reset";
	return this->tx_(cmd);
}

int CTDriver::setFrequencyMhz(uint8_t chan, double freq) {
	CHECKINIT
	std::string freqStr = std::to_string((long double) freq); //formatted without any weird characters
	return this->setFreq(chan, freqStr);
}

int CTDriver::setWavelengthNm(uint8_t chan, double wv) {
	CHECKINIT
	std::string freqStr = std::to_string((long double) wv); 
	freqStr = "#" + freqStr; //add the wavelength command modifier
	return this->setFreq(chan, freqStr);
}

int CTDriver::setFreq(uint8_t chan, std::string freqStr) {
	CHECKINIT
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
	CHECKINIT
	if (asf > 16383) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	BREAK_ERR
	std::string cmd = "dds amplitude " + chanStr + " " + std::to_string((unsigned long long) asf);
	return this->tx_(cmd);
}

int CTDriver::setGain(uint8_t chan,  unsigned int gain) {
	CHECKINIT
	if (gain > 31) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	BREAK_ERR
	std::string cmd = "dds gain " + chanStr + " " + std::to_string((unsigned long long) gain);
	return this->tx_(cmd);
}

int CTDriver::setPhase(uint8_t chan, double phaseDegrees) {
	CHECKINIT
	if ((phaseDegrees > 360.0) || (phaseDegrees < 0.0)) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	if (ret != CTDriver::OK) { return ret; }
	unsigned int val = (unsigned int) ((phaseDegrees / 360.0) * 16383);
	std::string cmd = "dds phase " + chanStr + " " + std::to_string((unsigned long long) val);
	return this->tx_(cmd);
}

int CTDriver::getPhase(uint8_t chan, double& phaseDegrees) {
	CHECKINIT
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
	CHECKINIT
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
	CHECKINIT
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
	CHECKINIT
	double freq;
	int ret = this->getFrequencyMhz(chan, freq);
	BREAK_ERR
	
	ret = this->freqToWavelength(freq, wv);
	BREAK_ERR

	return CTDriver::OK;
}

int CTDriver::getTemperature(std::string sensorType, double& temp) {
	CHECKINIT
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
	CHECKINIT
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
	CHECKINIT
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

AOTFLibCTDriver::AOTFLibCTDriver(uint8_t instance):
	CTDriver(std::bind(&AOTFLibCTDriver::tx, this, std::placeholders::_1), std::bind(&AOTFLibCTDriver::rx, this, std::placeholders::_1)),
	aotfHandle(NULL)
{
	HANDLE tfHandle = AotfOpen(instance);
	if (tfHandle == NULL) {
		throw "AOTFLibrary device was not found.";
	} else {
		this->aotfHandle = tfHandle;
	}
}

AOTFLibCTDriver::~AOTFLibCTDriver() {
	int ret = AotfClose(this->aotfHandle);
	int a = 1; //just for a debug point
}

int AOTFLibCTDriver::tx(std::string cmd) {
	cmd = cmd + "\r";
	int l = cmd.length();
	void* buf = (void*) cmd.c_str();
	bool ret = AotfWrite(this->aotfHandle, l, buf);
	if (ret) { return CTDriver::OK; }
	else {return CTDriver::ERR; }
}

int AOTFLibCTDriver::rx(std::string& out) {
	/*char buf[1024];
	void* pbuf = buf;
	unsigned int bytesRead;
	bool ret = AotfRead(this->aotfHandle, 1024, pbuf, &bytesRead);
	if (!ret) { return CTDriver::ERR; }
	else {
		std::string str(buf, bytesRead);
		out = str;
		return CTDriver::OK;
	}*/
	int i = 0;
	char bigbuf[1024];
	MM::MMTime startTime = GetMMTimeNow();
	while ((GetMMTimeNow() - startTime).getMsec()<1000) { //1 second timeout
		
		char buf[2];
		void* pbuf = buf;
		unsigned int bytesRead;
		bool ret = AotfRead(this->aotfHandle, 1, pbuf, &bytesRead);
		if (!ret) {
			return CTDriver::ERR; 
		} else if ( bytesRead == 1) {
			if (buf[0] == 13) {//carriage return
				std::string str(bigbuf, i);
				out = str;
				return CTDriver::OK;
			} else {
				bigbuf[i] = buf[0];
				i++;
			}	
		} 
	}
	std::string str(bigbuf, i);
	out = str;
	return CTDriver::ERR; //we timed out
}

void AOTFLibCTDriver::clearPort() {
	while (AotfIsReadDataAvailable(this->aotfHandle)) {
		char buf[1024];
		void* pbuf = buf;
		unsigned int bytesRead;
		AotfRead(this->aotfHandle, 1000, pbuf, &bytesRead);
	}
}