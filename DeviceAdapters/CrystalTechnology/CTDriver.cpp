#include "CrystalTechnology.h"
#define BREAK_ERR if (ret!=CTDriver::OK) { return ret; }

CTDriver::CTDriver(std::function<int(std::string)> serialSend, std::function<std::string(void)> serialReceive):
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
			i++
		} else {
			break; //This is not expected to ever  happen.
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
	//TODO parse the response and set phaseDegrees
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
	//TODO parse the response and set asf
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
	//TODO parse the response and set Freq
	return CTDriver::OK;
}

int CTDriver::getWavelengthNm(uint8_t chan, double& wv) {
	double freq;
	int ret = this->getFrequencyMhz(chan, freq);
	BREAK_ERR
	std::string chanStr;
	int ret = this->getChannelStr(chan, chanStr, false);
	BREAK_ERR
	std::string cmd = "calibration wave " + std::to_string((long double) freq);
	ret = this->tx_(cmd);
	BREAK_ERR
	std::string response;
	ret = this->rx_(response);
	BREAK_ERR
	//Response is in the form "Wavelength {scientific notation number} nm" TODO parse the response and set Wavelength
	response = response.substr(11); //Get rid of "Wavelength " prefix
	response = response.substr(0, response.length()-3); //Get rid of " nm" suffix
    //std::istringstream os(response);
    //os >> wv;
	double wavelength = strtod(response.c_str());
	if (wavelength == 0.0) { return DEVICE_ERR; } //Conversion failed
	else { wv = wavelength; }
	return CTDriver::OK;
}

int CTDriver::getAllTemperatures(std::string& temps) {
	std::string cmd = "temperature read *";
	int ret = this->tx_(cmd);
	BREAK_ERR
	ret = this->rx_(temps);
	BREAK_ERR
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