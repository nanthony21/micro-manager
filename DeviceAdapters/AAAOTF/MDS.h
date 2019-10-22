//This class does not implement any micromanager api but provides hardware functionality that can be used by other classes.
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>

class MDS {
public:
	MDS(int channels, std::string port);
	std::string getId();
	int updateChannel(uint8_t channel);
	int updateAllChannels();
	std::vector<MDSChannel> channels;
protected:
	const char* port_;
	int numChannels_;
private:
	int updateChannelsFromDevice();
	virtual int sendSerial(std::string) = 0;
	virtual std::string getSerial() = 0;
}

class MDSChannel {
	//The methods in this class do not execute any action. they simply set variables. then you can use getCmdString to get the string that should be sent to the device.
public:
	MDSChannel(int channelNum, double maxPower, double minFrequency, double maxFrequency);
	int setIntensity(double intensity);
	void setEnabled(bool state);
	int setFrequency(double frequency);
	bool getEnabled();
	double getIntensity();
	double getFrequency();
	std::string getCmdString();
protected:
	int channelNum_;
	//On/off
	int enabled_;
	//Intensity
	double power_;
	double maxPower_; //These are in units of dBm
	//Frequency
	double freq_;
	double maxFreq_;
	double minFreq_;
}