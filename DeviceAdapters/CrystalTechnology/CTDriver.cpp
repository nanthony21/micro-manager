#include "CrystalTechnology.h"

#define BREAK_ERR if (ret!=CTDriver::OK) { return ret; }
#define CHECKINIT if (!this->initialized) { return CTDriver::NOT_INITIALIZED; }

CTDriver::CTDriver():
	numChan_(1), //Assume one channel so we atleast have a value here if something goes weird while getting the number of channels in the constructor.
	initialized(false)
{}

int CTDriver::initialize() {
	this->clearPort();
	int ret = this->tx("");//The first time the device starts it will respond to any command with a bunch of ID info. prompt the info and toss it.
	MM::MMTime sTime = GetMMTimeNow();
	while ((GetMMTimeNow() - sTime).getMsec() < 100) {} //wait for the response;
	this->clearPort();
	std::string response;
	ret = this->sendCmd("dds freq *", response);
	BREAK_ERR
	uint8_t num=0;
	while (true) {
		size_t pos = response.find("\r\n");
		if (pos==std::string::npos) {//The last time we come through there should be nothing else. this means we're done.
			break;
		}
		std::string sub = response.substr(0, pos);
		response.erase(0, pos+2);
		if (sub.substr(0,7).compare("Channel")==0) {
			num++;
		} else {
			break; //This is not expected to ever happen.
		}
	}
	this->numChan_ = num;
	this->initialized = true;
	return CTDriver::OK;
}

int CTDriver::reset() {
	CHECKINIT
	std::string cmd = "dds reset";
	return this->sendCmd(cmd);
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
	return this->sendCmd(str);
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
	return this->sendCmd(cmd);
}

int CTDriver::setGain(uint8_t chan,  unsigned int gain) {
	CHECKINIT
	if (gain > 31) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	BREAK_ERR
	std::string cmd = "dds gain " + chanStr + " " + std::to_string((unsigned long long) gain);
	return this->tx(cmd);
}

int CTDriver::setPhase(uint8_t chan, double phaseDegrees) {
	CHECKINIT
	if ((phaseDegrees > 360.0) || (phaseDegrees < 0.0)) { return CTDriver::INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	if (ret != CTDriver::OK) { return ret; }
	unsigned int val = (unsigned int) ((phaseDegrees / 360.0) * 16383);
	std::string cmd = "dds phase " + chanStr + " " + std::to_string((unsigned long long) val);
	return this->sendCmd(cmd);
}

int CTDriver::getPhase(uint8_t chan, double& phaseDegrees) {
	CHECKINIT
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	BREAK_ERR
	std::string cmd = "dds phase " + chanStr;
	std::string response;
	ret = this->sendCmd(cmd, response);
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
	std::string response;
	ret = this->sendCmd(cmd, response);
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
	std::string response;
	this->sendCmd(cmd, response);
	BREAK_ERR
	//response is in form "Channel {x} profile {x} frequency {freq scientific notation}Hz (Ftw {xxxxxx variable length})"
	response = response.substr(30); //remove prefix
	size_t pos = response.find("Hz");
	response = response.substr(0, pos); //remove suffix
	freq = strtod(response.c_str(), NULL) / 1e6; //0 could be an error or actual value. divide to convert from Hz to MHz
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
	std::string response;
	int ret = this->sendCmd(cmd, response);
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
	std::string partNo;
	int ret = this->sendCmd(cmd, partNo);
	BREAK_ERR

	cmd = "boardid ctiserialnumber";
	std::string serial;
	ret = this->sendCmd(cmd, serial);
	BREAK_ERR
	info = partNo + serial;
	return CTDriver::OK;
}

int CTDriver::getTuningCoeffs(std::vector<double> coeffs) {
	CHECKINIT
	std::string response;
	int ret = this->sendCmd("calibration tuning *", response);
	BREAK_ERR
	std::vector<std::string> lines;
	boost::split(lines, response, boost::is_any_of("\r\n"), boost::token_compress_on); //Split each line into a separate string
	if (lines.size() != 6) { //There should always be 5 coeffients and one empty trailing line since the string ends witht eh delimiter. They are returned in this order: 0 order, 1st order (x), 2nd order (x^2), etc.
		return CTDriver::INVALID_VALUE;
	}
	for (int i=0; i<5; i++) {
		std::string line = lines.at(i);
		double coeff = strtod(line.substr(35).c_str(), NULL);//Each line is in the form "Tuning Polynomial Coefficient {chanNum} is {freqScientific)"
		coeffs.at(i) = coeff;
	}
	return CTDriver::OK;
}

int CTDriver::setTuningCoeffs(std::vector<double> coeffs) {
	CHECKINIT
	for (int i=0; i<5; i++) {
		std::ostringstream streamObj;
		streamObj << coeffs.at(i); //This should format as scientific when needed.
		std::string coeffstr = streamObj.str();
		std::string cmd = "calibration tuning " + std::to_string((long long) i) + std::string(" ") + coeffstr;
		int ret = this->sendCmd(cmd);
		BREAK_ERR
	}
	return CTDriver::OK;
}

int CTDriver::freqToWavelength(double freq, double& wavelength) {
	std::string cmd = "calibration wave " + std::to_string((long double) freq);
	std::string response;
	int ret = this->sendCmd(cmd, response);
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
	std::string response;
	int ret = this->sendCmd(cmd, response);
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

int CTDriver::sendCmd(std::string cmd) {
	std::string response;
	int ret = this->sendCmd(cmd, response); //This will read the "* " command terminator, but we expect the response to be totally empty.
	BREAK_ERR
	if (response.length()!=0) {
		return CTDriver::ERR;
	}
	return CTDriver::OK;
}

int CTDriver::sendCmd(std::string cmd, std::string& responseOut) {
	int ret = this->tx(cmd);
	//Read the echo
	std::string response; 
	ret = this->readUntil("\r\n", response);
	BREAK_ERR
	if (!response.compare(cmd)==0) {
		return CTDriver::NOECHO;
	}
	ret = this->readUntil("* ", response);
	BREAK_ERR
	responseOut = response;
	return CTDriver::OK;
}

AOTFLibCTDriver::AOTFLibCTDriver(uint8_t instance):
	CTDriver(),
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
	int ret = AotfClose(this->aotfHandle); //This is boolean, so should be 1 for success.
	int a = 1; //just for a debug point
}

int AOTFLibCTDriver::tx(std::string cmd) {
	std::string termcmd = cmd + "\r";
	int l = termcmd.length();
	void* buf = (void*) termcmd.c_str();
	bool ret = AotfWrite(this->aotfHandle, l, buf);
	if (!ret) { return CTDriver::ERR; }
	return CTDriver::OK;
}

int AOTFLibCTDriver::readUntil(std::string delim, std::string& out) {
	uint8_t lenDelim = delim.length();
	MM::MMTime startTime = GetMMTimeNow();
	char bigBuff[1024];
	char buf[2];
	void* pbuf = buf;
	unsigned int bytesRead=0;
	int i = 0;
	
	while (true) {
		while (!AotfIsReadDataAvailable(this->aotfHandle)) {//Don't try to read unless there's something to read
			if ((GetMMTimeNow() - startTime).getMsec()>1000) { //1 second timeout
				return CTDriver::SERIAL_TIMEOUT;
			}
		}
		bool ret = AotfRead(this->aotfHandle, 1, pbuf, &bytesRead);
		if (!ret) {
			std::string str(bigBuff, i);
			out = str;
			return CTDriver::ERR;
		} else if (bytesRead == 1) {
			bigBuff[i] = buf[0];
			i++;
			if (i>=lenDelim) { //Don't check for delimiter if we haven't gathered enough bytes yet.
				char* delimPtr = bigBuff + (i-lenDelim);
				std::string testStr(delimPtr, lenDelim);
				if (delim.compare(testStr)==0) {
					out = std::string(bigBuff, i-lenDelim);
					return CTDriver::OK;
				}
			}
		}
	}
}

void AOTFLibCTDriver::clearPort() {
	while (AotfIsReadDataAvailable(this->aotfHandle)) {
		char buf[1024];
		void* pbuf = buf;
		unsigned int bytesRead;
		AotfRead(this->aotfHandle, 1000, pbuf, &bytesRead);
	}
}