#include "SutterHub.h"



SutterHub::SutterHub()
{
}


SutterHub::~SutterHub()
{
}

int SutterHub::DetectInstalledDevices() {

}

int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);

bool ControllerBusy();
int GoOnLine(unsigned long answerTimeoutMs);
int GetControllerType(unsigned long answerTimeoutMs, std::string& type, std::string& id);
int GetStatus(unsigned long answerTimeoutMs, unsigned char* status);

int SutterHub::SetCommand(const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho, const unsigned long answerTimeoutMs, const bool responseRequired, const bool CRExpected) {
	std::vector<unsigned char> _;
	return  SetCommand(command, alternateEcho, answerTimeoutMs, _, responseRequired, CRExpected);
}

