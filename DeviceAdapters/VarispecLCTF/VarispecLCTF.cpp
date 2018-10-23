///////////////////////////////////////////////////////////////////////////////
// FILE:          VarispecLCTF.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   VarispecLCTF 
//
// AUTHOR:        Nick Anthony, BPL, 2018 based heavily on the the VariLC adapter by Rudolf Oldenbourg.


#include "VarispecLCTF.h"
#include <cstdio>
#include <cctype>
#include <string>
#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
#include <sstream>
#include <algorithm> 


const char* g_ControllerName    = "VarispecLCTF";

const char* g_TxTerm            = "\r"; //unique termination
const char* g_RxTerm            = "\r"; //unique termination

const char* g_BaudRate_key        = "Baud Rate";
const char* g_Baud9600            = "9600";
const char* g_Baud115200          = "115200";



using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
	RegisterDevice(g_ControllerName, MM::GenericDevice, "VarispecLCTF");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
	if (deviceName == 0) return 0;
	if (strcmp(deviceName, g_ControllerName) == 0) return new VarispecLCTF();
	return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
	delete pDevice;
}


// General utility function:
int ClearPort(MM::Device& device, MM::Core& core, std::string port)
{
	// Clear contents of serial port 
	const int bufSize = 2048;
	unsigned char clear[bufSize];
	unsigned long read = bufSize;
	int ret;
	while ((int)read == bufSize)
	{
		ret = core.ReadFromSerial(&device, port.c_str(), clear, bufSize, read);
		if (ret != DEVICE_OK)
			return ret;
	}
	return DEVICE_OK;
}





///////////////////////////////////////////////////////////////////////////////
// VarispecLCTF Hub
///////////////////////////////////////////////////////////////////////////////

VarispecLCTF::VarispecLCTF() :
	baud_(g_Baud9600),
	initialized_(false),
	initializedDelay_(false),
	answerTimeoutMs_(1000),
  wavelength_(546)
{
	InitializeDefaultErrorMessages();
	// pre-initialization properties
	// Port:
	CPropertyAction* pAct = new CPropertyAction(this, &VarispecLCTF::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
	SetProperty(MM::g_Keyword_Port, port_.c_str());
   
	pAct = new CPropertyAction(this, &VarispecLCTF::OnBaud);
	CreateProperty(g_BaudRate_key, "Undefined", MM::String, false, pAct, true);

	AddAllowedValue(g_BaudRate_key, g_Baud115200, (long)115200);
	AddAllowedValue(g_BaudRate_key, g_Baud9600, (long)9600);

	EnableDelay();
}


VarispecLCTF::~VarispecLCTF()
{
	Shutdown();
}

void VarispecLCTF::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name, g_ControllerName);
}


bool VarispecLCTF::SupportsDeviceDetection(void)
{
	return true;
}

MM::DeviceDetectionStatus VarispecLCTF::DetectDevice(void)
{
	// all conditions must be satisfied...
	MM::DeviceDetectionStatus result = MM::Misconfigured;

	try
	{
		long baud;
		GetProperty(g_BaudRate_key, baud);

		std::string transformed = port_;
		for (std::string::iterator its = transformed.begin(); its != transformed.end(); ++its)
		{
			*its = (char)tolower(*its);
		}

		if (0 < transformed.length() && 0 != transformed.compare("undefined") && 0 != transformed.compare("unknown"))
		{
			int ret = 0;
			MM::Device* pS;


			// the port property seems correct, so give it a try
			result = MM::CanNotCommunicate;
			// device specific default communication parameters
			GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_AnswerTimeout, "2000.0");
			GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, baud_.c_str());
			GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_DelayBetweenCharsMs, "0.0");
			GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Handshaking, "Off");
			GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Parity, "None");
			GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
			GetCoreCallback()->SetDeviceProperty(port_.c_str(), "Verbose", "1");
			pS = GetCoreCallback()->GetDevice(this, port_.c_str());
			pS->Initialize();

			ClearPort(*this, *GetCoreCallback(), port_);
			ret = sendCmd("V?", serialnum_);
			if (ret != DEVICE_OK || serialnum_.length() < 5)
			{
				LogMessageCode(ret, true);
				LogMessage(std::string("VarispecLCTF not found on ") + port_.c_str(), true);
				LogMessage(std::string("VarispecLCTF serial no:") + serialnum_, true);
				ret = 1;
				serialnum_ = "0";
				pS->Shutdown();
			}
			else
			{
				// to succeed must reach here....
				LogMessage(std::string("VarispecLCTF found on ") + port_.c_str(), true);
				LogMessage(std::string("VarispecLCTF serial no:") + serialnum_, true);
				result = MM::CanCommunicate;
				GetCoreCallback()->SetSerialProperties(port_.c_str(),
					"600.0",
					baud_.c_str(),
					"0.0",
					"Off",
					"None",
					"1");
				serialnum_ = "0";
				pS->Initialize();
				ret = sendCmd("R1");
				pS->Shutdown();
			}
		}
	}
	catch (...)
	{
		LogMessage("Exception in DetectDevice!", false);
	}
	return result;
}

int VarispecLCTF::Initialize()
{

	// empty the Rx serial buffer before sending command
	int ret = ClearPort(*this, *GetCoreCallback(), port_);
	if (ret != DEVICE_OK)
		return ret;

	// Name
	ret = CreateProperty(MM::g_Keyword_Name, g_ControllerName, MM::String, true);
	if (DEVICE_OK != ret)
		return ret;

	// Version number
	CPropertyAction* pAct = new CPropertyAction(this, &VarispecLCTF::OnSerialNumber);
	ret = CreateProperty("Version Number", "Version Number Not Found", MM::String, true, pAct);
	if (ret != DEVICE_OK)
		return ret;

	pAct = new CPropertyAction(this, &VarispecLCTF::OnBriefMode);
	ret = CreateProperty("Mode; 1=Brief; 0=Standard", "", MM::String, true, pAct);
	if (ret != DEVICE_OK)
		return ret;

	//Set VarispecLCTF to Standard mode
	briefModeQ_ = false;
	ret = sendCmd("B0",getFromVarispecLCTF_);
	if (ret != DEVICE_OK)
		return ret;

	// Wavelength
	std::string ans;
	ret = sendCmd("V?", ans);	//The serial number response also contains the tuning range of the device
	std::vector<double> nums = getNumbersFromMessage(ans, briefModeQ_);	//This will be in the format (revision level, shortest wavelength, longest wavelength, serial number).
	if (ret != DEVICE_OK)
		return ret;
	pAct = new CPropertyAction(this, &VarispecLCTF::OnWavelength);
	ret = CreateProperty("Wavelength", DoubleToString(wavelength_).c_str(), MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;
	SetPropertyLimits("Wavelength", nums.at(1), nums.at(2));

	// Delay
	pAct = new CPropertyAction(this, &VarispecLCTF::OnDelay);
	ret = CreateProperty("Device Delay (ms.)", "200.0", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;
	SetPropertyLimits("Device Delay (ms.)", 0.0, 200.0);

	pAct = new CPropertyAction(this, &VarispecLCTF::OnSendToVarispecLCTF);
	ret = CreateProperty("String send to VarispecLCTF", "", MM::String, false, pAct);
	if (ret != DEVICE_OK) {
		return ret;
	}
	pAct = new CPropertyAction(this, &VarispecLCTF::OnGetFromVarispecLCTF);
	ret = CreateProperty("String from VarispecLCTF", "", MM::String, true, pAct);
	if (ret != DEVICE_OK) {
		return ret;
	}
	SetErrorText(99, "Device set busy for ");
	return DEVICE_OK;
}

int VarispecLCTF::Shutdown() {
	initialized_ = false;
	return DEVICE_OK;
}

//////////////// Action Handlers (VarispecLCTF) /////////////////

int VarispecLCTF::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
	if (pAct == MM::BeforeGet)
	{
		pProp->Set(port_.c_str());
	}
	else if (pAct == MM::AfterSet)
	{
		if (initialized_)
		{
			pProp->Set(port_.c_str());
			return DEVICE_INVALID_INPUT_PARAM;
		}
		pProp->Get(port_);
	}

	return DEVICE_OK;
}

int VarispecLCTF::OnBaud(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(baud_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		if (initialized_)
		{
			pProp->Set(baud_.c_str());
			return DEVICE_INVALID_INPUT_PARAM;
		}
		pProp->Get(baud_);
	}

	return DEVICE_OK;
}


int VarispecLCTF::OnBriefMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet) {
		std::string ans;
		int ret = sendCmd("B?", ans);
		if (ret != DEVICE_OK)return DEVICE_SERIAL_COMMAND_FAILED;
		if (ans == "1") {
			briefModeQ_ = true;
		}
		else {
			briefModeQ_ = false;
		}
		if (briefModeQ_) {
			pProp->Set(" 1");
		}
		else {
			pProp->Set(" 0");
		}
	}
	else if (eAct == MM::AfterSet)
	{

	}
	return DEVICE_OK;
}


 int VarispecLCTF::OnSerialNumber(MM::PropertyBase* pProp, MM::ActionType eAct)
 {
	 if (eAct == MM::BeforeGet)
	 {
		 int ret = sendCmd("V?", serialnum_);
		 if (ret != DEVICE_OK)return DEVICE_SERIAL_COMMAND_FAILED;
		 pProp->Set(serialnum_.c_str());
	 }
	 return DEVICE_OK;
 }

 int VarispecLCTF::OnWavelength(MM::PropertyBase* pProp, MM::ActionType eAct)
 {
	 if (eAct == MM::BeforeGet)
	 {
		 std::string ans;
		 int ret = sendCmd("W?", ans);
		 if (ret != DEVICE_OK)return DEVICE_SERIAL_COMMAND_FAILED;
		 vector<double> numbers = getNumbersFromMessage(ans, briefModeQ_);
		 if (numbers.size() == 0) { //The device must have returned "W*" meaning that an invalid wavelength was sent
			 SetErrorText(99, "The Varispec device was commanded to tune to an out of range wavelength.");
			 return 99;
		 }
		 pProp->Set(numbers[0]);
	 }
	 else if (eAct == MM::AfterSet)
	 {
		 double wavelength;
		 // read value from property
		 pProp->Get(wavelength);
		 // write wavelength out to device....
		 ostringstream cmd;
		 cmd.precision(3);
		 cmd << "W " << wavelength;
		 int ret = sendCmd(cmd.str());
		 if (ret != DEVICE_OK)
			 return DEVICE_SERIAL_COMMAND_FAILED;
		 changedTime_ = GetCurrentMMTime();
		 wavelength_ = wavelength;
	 }
	 return DEVICE_OK;
 }

 int VarispecLCTF::OnSendToVarispecLCTF(MM::PropertyBase* pProp, MM::ActionType eAct)
 {
	 if (eAct == MM::AfterSet) {
		 // read value from property
		 pProp->Get(sendToVarispecLCTF_);
		 return sendCmd(sendToVarispecLCTF_, getFromVarispecLCTF_);
	 }
	 return DEVICE_OK;
 }

int VarispecLCTF::OnGetFromVarispecLCTF(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		//   GetSerialAnswer (port_.c_str(), "\r", getFromVarispecLCTF_);
		pProp->Set(getFromVarispecLCTF_.c_str());
	}
	return DEVICE_OK;
}

int VarispecLCTF::OnDelay(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		double delay = GetDelayMs();
		initializedDelay_ = true;
		pProp->Set(delay);
	}
	else if (eAct == MM::AfterSet)
	{
		double delayT;
		pProp->Get(delayT);
		if (initializedDelay_) {
			SetDelayMs(delayT);
		}
		delay_ = delayT * 1000;
	}
	return DEVICE_OK;
}

bool VarispecLCTF::Busy()
{
	if (delay_.getMsec() > 0.0) {
		MM::MMTime interval = GetCurrentMMTime() - changedTime_;
		if (interval.getMsec() < delay_.getMsec()) {
			return true;
		}
	}
	return false;
}

std::string VarispecLCTF::DoubleToString(double N)
{
	ostringstream ss("");
	ss << N;
	return ss.str();
}

std::vector<double> VarispecLCTF::getNumbersFromMessage(std::string VarispecLCTFmessage, bool briefMode) {
	std::istringstream variStream(VarispecLCTFmessage);
	std::string prefix;
	double val;
	std::vector<double> values;

	if (!briefMode) {
		variStream >> prefix;
	}
	for (;;) {
		variStream >> val;
		if (!variStream.fail()) {
			values.push_back(val);
		}
		else {
			break;
		}
	}

	return values;
}

int VarispecLCTF::sendCmd(std::string cmd, std::string& out) {
	int ret = sendCmd(cmd);
	if (ret != DEVICE_OK) {
		return DEVICE_SERIAL_COMMAND_FAILED;
	}
	GetSerialAnswer(port_.c_str(), "\r", out); //Try returning any extra response from the device.
	return DEVICE_OK;
}

int VarispecLCTF::sendCmd(std::string cmd) {
	int ret = SendSerialCommand(port_.c_str(), cmd.c_str(), "\r");
	if (ret != DEVICE_OK) {
		return DEVICE_SERIAL_COMMAND_FAILED;
	}
	std::string response;
	GetSerialAnswer(port_.c_str(), "\r", response);	//Read back the response and make sure it matches what we sent. If not there is an issue with communication.
	if (response != cmd) {
		SetErrorText(99, "The VarispecLCTF did not respond.");
		return 99;
	}
	return DEVICE_OK;
}

int VarispecLCTF::IsPropertySequenceable(const char* name, bool& isSequenceable) const
{
	if (strcmp(name, "Wavelength") == 0)
		isSequenceable = true;
	else
		isSequenceable = false;
	return DEVICE_OK;
}

int VarispecLCTF::GetPropertySequenceMaxLength(const char* name, long& nrEvents) const
{
	if (strcmp(name, "Wavelength") == 0) {
		nrEvents = 128;	//We are using the palette functionality as this is slightly faster than specifying a wavelength jump. the limit of paletted is 128
	}
	else {
		nrEvents = 0;
	}
	return DEVICE_OK;
}

int VarispecLCTF::StartPropertySequence(const char* propertyName) {
	if (strcmp(propertyName, "Wavelength") == 0)
	{
		int ret;
		ret = sendCmd("M0"); //Ensure we are in sequence mode 0.
		if (ret != DEVICE_OK) {return ret;}
		ret = sendCmd("G1"); //Enable the TTL port. wavength will change every pulse
		if (ret != DEVICE_OK) { return ret; }
		ret = sendCmd("P0");	//Go to the first pallete element before sequencing begins
		if (ret != DEVICE_OK) { return ret; }
		return DEVICE_OK;
	}
	else {
		return DEVICE_UNSUPPORTED_COMMAND;
	}
}

int VarispecLCTF::StopPropertySequence(const char* propertyName) {
	if (strcmp(propertyName, "Wavelength") == 0)
	{
		int ret = sendCmd("G0"); //disable the TTL port
		ret |= sendCmd("P0");//Go to the first pallete element after sequencing
		return ret;
	}
	else
		return DEVICE_UNSUPPORTED_COMMAND;
}

int VarispecLCTF::ClearPropertySequence(const char* propertyName) {
	if (strcmp(propertyName, "Wavelength") == 0)
	{
		int ret = sendCmd("C1");
		sequence_.clear();
		return ret;
	}
	else
		return DEVICE_UNSUPPORTED_COMMAND;
}

int VarispecLCTF::AddToPropertySequence(const char* propertyName, const char* value) {
	if (strcmp(propertyName, "Wavelength") == 0) {
		double wv = atof(value);
		sequence_.push_back(wv);
		return DEVICE_OK;
	}
	else {
		return DEVICE_UNSUPPORTED_COMMAND;
	}
}

int VarispecLCTF::SendPropertySequence(const char* propertyName) {
	if (strcmp(propertyName, "Wavelength") == 0)
	{
		ostringstream cmd;
		for (unsigned int i = 0; i < sequence_.size(); i++) {
			cmd.precision(3);
			cmd << "D" << sequence_.at(i) << "," << i;
		}
		return sendCmd(cmd.str());
	}
	else {
		return DEVICE_UNSUPPORTED_COMMAND;
	}
}
