#include "MDS.h"


MDSChannel::MDSChannel(int channel, double maxPower, double minFrequency, double maxFrequency) {
	this->channelNum_ = channel;
	this->maxPower_ = maxPower;
	this->minFreq_ = minFrequency;
	this->maxFreq_ = maxFrequency;
}

/**
 * Here we set the intensity
 */
int MDSChannel::setIntensity(double intensity)
{              
	if (intensity > this->maxPower_) {
		return 1;
	}
   this->power_ = intensity;
   return 0;
}

/**
 * Here we set the shutter to open or close
 */
void MDSChannel::setEnabled(bool state)                              
{             
	this->enabled_ = state ? 1 : 0;
}

int MDSChannel::setFrequency(double frequency) {
	if ((frequency > this->maxFreq_) || (frequency < this->minFreq_)) {
		return 1;
	}
	this->freq_ = frequency;
	return 0;
}

bool MDSChannel::getEnabled() {
	return this->enabled_;
}

double MDSChannel::getIntensity() {
	return this->power_;
}

double MDSChannel::getFrequency() {
	return this->freq_;
}


std::string MDSChannel::getCmdString() {
	std::ostringstream command;
	command << "L" << channelNum_ << "F" << freq_ << "D" << power_ << "O" << enabled_;
	return command.str();
}


MDS::MDS(int numChannels, std::string port) {
	this->port_ = port.c_str();
	this->numChannels_ = numChannels;
	for (uint8_t i=0; i < numChannels; i++) {
		this->channels.push_back(MDSChannel(i+1));
	}
}

int MDS::updateChannel(uint8_t channel) {
	if ((channel > this->numChannels_) || (channel < 1)) {
		return DEVICE_ERROR;
	}
	MDSChannel chan = channels.at(channel - 1);
	std::string cmd = chan.getCmdString();
	int ret = this->sendSerial(cmd);
}

int MDS::updateAllChannels() {
	for (int i=1; i<this->numChannels_+1; i++) {
		this->updateChannel(i);
	}
}

std::string MDS::getId() {
	int ret = this->sendSerial("q");
	return this->getSerial();
}

int MDS::updateChannelsFromDevice() {
	int ret = this->sendSerial("s");
	//TODO parse and upde channel objects.
}
