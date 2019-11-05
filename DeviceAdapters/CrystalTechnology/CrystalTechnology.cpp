// CrystalTechnology.cpp : Defines the exported functions for the DLL application.
//

#include "CrystalTechnology.h"

CrystalTech::CrystalTech() {
	InitializeDefaultErrorMessages();
	SetErrorText(DEVICE_SERIAL_TIMEOUT, "Serial port timed out without receiving a response.");

	CreateProperty(MM::g_Keyword_Name, g_ControllerName, MM::String, true);
	CreateProperty(MM::g_Keyword_Port, "Unkn", MM::String, false, CrystalTech::onPort, true);

}

CrystalTech::~CrystalTech() {}

int CrystalTech::Initialize() {
//TODO
}

int CrystalTech::Shutdown() {
	return DEVICE_OK;
}

int CrystalTech::sendCmd(const char* cmd) {
	//Commands can be delimited by either \r or \n. You can also have multiple commands in a line with a ; delimiter, that is not used here though.
	int ret = SendSerialCommand(port_, cmd, "\r");
	if (ret!=DEVICE_OK) { return ret; }
	return DEVICE_OK;
}

int CrystalTech::sendCmd(const char* cmd, std::string response) {
	int ret = sendCmd(cmd);
	if (ret!=DEVICE_OK) { return ret; }
	ret = GetSerialAnswer(port_, "\r", response); //I'm just guessing about the delimiter here. I don't see it in the manual.
	if (ret!=DEVICE_OK) { return ret; }
	return DEVICE_OK;
}

int CrystalTech::onPort(MM::PropertyBase* pProp, MM::ActionType eAct) {

}