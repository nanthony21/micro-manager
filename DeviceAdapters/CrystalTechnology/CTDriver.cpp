#include "CrystalTechnology.h"

CTDriver::CTDriver(uint8_t numChannels, std::function<int(std::string)> serialSend, std::function<std::string(void)> serialReceive) {
	tx_ = serialSend;
	rx_ = serialReceive;
	numChan_ = numChannels;
}

int CTDriver::reset() {
	std::string cmd = "dds reset";
	return this->tx_(cmd);
}

int CTDriver::setFrequencyMhz(uint8_t chan, double freq) {
	std::string freqStr = std::to_string((long double) freq); //formatted without any weird characters
	return this->setFreq(chan, freqStr);
}

int CTDriver::setWavelengthNm(uint8_t chan, unsigned int wv) {
	std::string freqStr = std::to_string((unsigned long long) wv); 
	freqStr = "#" + freqStr; //add the wavelength command modifier
	return this->setFreq(chan, freqStr);
}

int CTDriver::setFreq(uint8_t chan, std::string freqStr) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	if (ret!=CT_OK) { return ret; }
	std::string str = "dds frequency " + chanStr + " " + freqStr;
	return this->tx_(str);
}

int CTDriver::getChannelStr(uint8_t chan, std::string& chanStr, bool allowWildcard) {
	if ((chan==255) && (allowWildcard)) {
		chanStr = "*";
		return CT_OK;
	} else if (chan >= numChan_) {
		return CT_INVALID_CHANNEL;
	}
	chanStr = std::to_string((unsigned long long) chan); //This weird datatype is required to satisfy VC2010 implementation of to_string
	return CT_OK;
}

int CTDriver::setAmplitude(uint8_t chan,  unsigned int asf) {
	if (asf > 16383) { return CT_INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	if (ret!=CT_OK) { return ret; }
	std::string cmd = "dds amplitude " + chanStr + " " + std::to_string((unsigned long long) asf);
	return this->tx_(cmd);
}

int CTDriver::setGain(uint8_t chan,  unsigned int gain) {
	if (gain > 31) { return CT_INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	if (ret!=CT_OK) { return ret; }
	std::string cmd = "dds gain " + chanStr + " " + std::to_string((unsigned long long) gain);
	return this->tx_(cmd);
}

int CTDriver::setPhase(uint8_t chan, double phaseDegrees) {
	if ((phaseDegrees > 360.0) || (phaseDegrees < 0.0)) { return CT_INVALID_VALUE; }
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, true);
	if (ret != CT_OK) { return ret; }
	unsigned int val = (unsigned int) ((phaseDegrees / 360.0) * 16383);
	std::string cmd = "dds phase " + chanStr + " " + std::to_string((unsigned long long) val);
	return this->tx_(cmd);
}

int CTDriver::getPhase(uint8_t chan, double& phaseDegrees) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	if (ret!=CT_OK) { return ret; }
	std::string cmd = "dds phase " + chanStr;
	ret = this->tx_(cmd);
	if (ret!=CT_OK) { return ret; }
	std::string response;
	ret = this->rx_(response);
	if (ret!=CT_OK) { return ret; }
	//TODO parse the response and set phaseDegrees
	return CT_OK;
}

int CTDriver::getAmplitude(uint8_t chan, unsigned int& asf) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	if (ret!=CT_OK) { return ret; }
	std::string cmd = "dds amplitude " + chanStr;
	ret = this->tx_(cmd);
	if (ret!=CT_OK) { return ret; }
	std::string response;
	ret = this->rx_(response);
	if (ret!=CT_OK) { return ret; }
	//TODO parse the response and set asf
	return CT_OK;
}

/*int CTDriver::getFrequencyMhz(uint8_t chan, double& freq) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	if (ret!=CT_OK) { return ret; }
	std::string cmd = "dds frequency " + chanStr;
	ret = this->tx_(cmd);
	if (ret!=CT_OK) { return ret; }
	std::string response;
	ret = this->rx_(response);
	if (ret!=CT_OK) { return ret; }
	//TODO parse the response and set asf
	return CT_OK;
}

int CTDriver::getWavelengthNm(uint8_t chan, unsigned int& wv) {
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	if (ret!=CT_OK) { return ret; }
	std::string cmd = "dds wavelength " + chanStr;
	ret = this->tx_(cmd);
	if (ret!=CT_OK) { return ret; }
	std::string response;
	ret = this->rx_(response);
	if (ret!=CT_OK) { return ret; }
	//TODO parse the response and set asf
	return CT_OK;
}*/

int CTDriver::getAllTemperatures(std::string& temps) {
	std::string cmd = "temperature read *";
	int ret = this->tx_(cmd);
	if (ret!=CT_OK) { return ret; }
	ret = this->rx_(temps);
	if (ret!=CT_OK) { return ret; }
	return CT_OK;
}

int CTDriver::getBoardInfo(std::string& info) {
	std::string cmd = "boardid partnumber";
	int ret = this->tx_(cmd);
	if (ret!=CT_OK) { return ret; }
	std::string partNo;
	ret = this->rx_(partNo);
	if (ret!=CT_OK) { return ret; }

	std::string cmd = "boardid ctiserialnumber";
	int ret = this->tx_(cmd);
	if (ret!=CT_OK) { return ret; }
	std::string serial;
	ret = this->rx_(serial);
	if (ret!=CT_OK) { return ret; }
	info = partNo + serial;
	return CT_OK;
}