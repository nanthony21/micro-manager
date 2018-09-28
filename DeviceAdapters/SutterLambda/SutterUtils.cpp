#include "SutterLambda.h"

///////////////////////////////////////////////////////////////////////////////////
//  SutterUtils: Static utility functions that can be used from all devices
//////////////////////////////////////////////////////////////////////////////////
bool SutterUtils::ControllerBusy(MM::Device& /*device*/, MM::Core& /*core*/, std::string port,
	unsigned long /*answerTimeoutMs*/)
{

	MMThreadGuard g(*(::gplocks_[port]));
	return g_Busy[port];
}
int SutterUtils::GoOnLine(MM::Device& device, MM::Core& core,
	std::string port, unsigned long answerTimeoutMs)
{
	// Transfer to On Line
	unsigned char setSerial = (unsigned char)238;
	int ret = core.WriteToSerial(&device, port.c_str(), &setSerial, 1);
	if (DEVICE_OK != ret)
		return ret;

	unsigned char answer = 0;
	bool responseReceived = false;
	int unsigned long read;
	MM::MMTime startTime = core.GetCurrentMMTime();
	do {
		if (DEVICE_OK != core.ReadFromSerial(&device, port.c_str(), &answer, 1, read))
			return false;
		if (answer == 238)
			responseReceived = true;
	} while (!responseReceived && (core.GetCurrentMMTime() - startTime) < (answerTimeoutMs * 1000.0));
	if (!responseReceived)
		return ERR_NO_ANSWER;

	return DEVICE_OK;
}

int SutterUtils::GetControllerType(MM::Device& device, MM::Core& core, std::string port,
	unsigned long answerTimeoutMs, std::string& type, std::string& id)
{
	core.PurgeSerial(&device, port.c_str());
	int ret = DEVICE_OK;

	std::vector<unsigned char> ans;
	std::vector<unsigned char> emptyv;
	std::vector<unsigned char> command;
	command.push_back((unsigned char)253);

	ret = SutterUtils::SetCommandNoCR(device, core, port, command, emptyv, answerTimeoutMs, ans, true);
	if (ret != DEVICE_OK)
		return ret;

	char ans2[128];
	ret = core.GetSerialAnswer(&device, port.c_str(), 128, ans2, "\r");

	std::string answer = ans2;
	if (ret != DEVICE_OK) {
		std::ostringstream errOss;
		errOss << "Could not get answer from 253 command (GetSerialAnswer returned " << ret << "). Assuming a 10-2";
		core.LogMessage(&device, errOss.str().c_str(), true);
		type = "10-2";
		id = "10-2";
	}
	else if (answer.length() == 0) {
		core.LogMessage(&device, "Answer from 253 command was empty. Assuming a 10-2", true);
		type = "10-2";
		id = "10-2";
	}
	else {
		if (answer.substr(0, 2) == "SC") {
			type = "SC";
		}
		else if (answer.substr(0, 4) == "10-3") {
			type = "10-3";
		}
		id = answer.substr(0, answer.length() - 2);
		core.LogMessage(&device, ("Controller type is " + std::string(type)).c_str(), true);
	}

	return DEVICE_OK;
}

/**
* Queries the controller for its status
* Meaning of status depends on controller type
* status should be allocated by the caller and at least be 21 bytes long
* status will be the answer returned by the controller, stripped from the first byte (which echos the command)
*/
int SutterUtils::GetStatus(MM::Device& device, MM::Core& core,
	std::string port, unsigned long answerTimeoutMs,
	unsigned char* status)
{
	core.PurgeSerial(&device, port.c_str());
	unsigned char msg[1];
	msg[0] = 204;
	// send command
	int ret = core.WriteToSerial(&device, port.c_str(), msg, 1);
	if (ret != DEVICE_OK)
		return ret;

	unsigned char ans = 0;
	bool responseReceived = false;
	unsigned long read;
	MM::MMTime startTime = core.GetCurrentMMTime();
	do {
		if (DEVICE_OK != core.ReadFromSerial(&device, port.c_str(), &ans, 1, read))
			return false;
		/*if (read > 0)
		printf("Read char: %x", ans);*/
		if (ans == 204)
			responseReceived = true;
		CDeviceUtils::SleepMs(2);
	} while (!responseReceived && (core.GetCurrentMMTime() - startTime) < (answerTimeoutMs * 1000.0));
	if (!responseReceived)
		return ERR_NO_ANSWER;

	responseReceived = false;
	int j = 0;
	startTime = core.GetCurrentMMTime();
	do {
		if (DEVICE_OK != core.ReadFromSerial(&device, port.c_str(), &ans, 1, read))
			return false;
		if (read > 0) {
			/* printf("Read char: %x", ans);*/
			status[j] = ans;
			j++;
			if (ans == '\r')
				responseReceived = true;
		}
		CDeviceUtils::SleepMs(2);
	} while (!responseReceived && (core.GetCurrentMMTime() - startTime) < (answerTimeoutMs * 1000.0) && j < 22);
	if (!responseReceived)
		return ERR_NO_ANSWER;

	return DEVICE_OK;
}





int SutterUtils::SetCommand(MM::Device& device, MM::Core& core,
	const std::string port, const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho,
	const unsigned long answerTimeoutMs, const bool responseRequired, const bool CRExpected)
{
	std::vector<unsigned char> tmpr;
	return  SutterUtils::SetCommand(device, core, port, command, alternateEcho, answerTimeoutMs, tmpr, responseRequired, CRExpected);
}



// lock the port for access,
// write 1, 2, or 3 char. command to equipment
// ensure the command completed by waiting for \r
// pass response back in argument
int SutterUtils::SetCommand(MM::Device& device, MM::Core& core,
	const std::string port, const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho,
	const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired, const bool CRExpected)



{
	int ret = DEVICE_OK;
	bool responseReceived = false;

	// block if port is accessed by another thread
	MMThreadGuard g(*(::gplocks_[port]));
	if (::g_Busy[port])
		core.LogMessage(&device, "busy entering SetCommand", false);

	if (!::g_Busy[port])
	{
		g_Busy[port] = true;
		if (!command.empty())
		{
			core.PurgeSerial(&device, port.c_str());
			// start time of entire transaction
			MM::MMTime commandStartTime = core.GetCurrentMMTime();
			// write command to the port
			ret = core.WriteToSerial(&device, port.c_str(), &command[0], (unsigned long)command.size());

			if (DEVICE_OK == ret)
			{
				if (responseRequired)
				{
					// now ensure that command is echoed from controller
					bool expectA13 = CRExpected;
					std::vector<unsigned char>::const_iterator allowedResponse = alternateEcho.begin();
					for (std::vector<unsigned char>::const_iterator irep = command.begin(); irep != command.end(); ++irep)
					{
						// the echoed command can be different from the command, for example, the SC controller
						// echoes close 172 for open 170 and vice versa....

						// and Wheel set position INTERMITTENTLY echos the position with the command and speed stripped out

						// block/wait for acknowledge, or until we time out;
						unsigned char answer = 0;
						unsigned long read;
						MM::MMTime responseStartTime = core.GetCurrentMMTime();
						// read the response
						for (;;)
						{
							answer = 0;
							int status = core.ReadFromSerial(&device, port.c_str(), &answer, 1, read);
							if (DEVICE_OK != status)
							{
								// give up - somebody pulled the serial hardware out of the computer
								core.LogMessage(&device, "ReadFromSerial failed", false);
								return status;
							}
							if (0 < read)
								response.push_back(answer);
							if (answer == *irep)
							{
								if (!CRExpected)
								{
									responseReceived = true;
								}
								break;
							}
							else if (answer == 13) // CR
							{
								if (CRExpected)
								{
									expectA13 = false;
									core.LogMessage(&device, "error, command was not echoed!", false);
								}
								else
								{
									// todo: can 13 ever be a command echo?? (probably not - no 14 position filter wheels)
									;
								}
								break;
							}
							else if (answer != 0)
							{
								if (alternateEcho.end() != allowedResponse)
								{
									// command was echoed, after a fashion....
									if (answer == *allowedResponse)
									{

										core.LogMessage(&device, ("command " + CDeviceUtils::HexRep(command) +
											" was echoed as " + CDeviceUtils::HexRep(response)).c_str(), true);
										++allowedResponse;
										break;
									}
								}
								std::ostringstream bufff;
								bufff.flags(std::ios::hex | std::ios::showbase);

								bufff << (unsigned int)answer;
								core.LogMessage(&device, (std::string("unexpected response: ") + bufff.str()).c_str(), false);
								break;
							}
							MM::MMTime delta = core.GetCurrentMMTime() - responseStartTime;
							if (1000.0 * answerTimeoutMs < delta.getUsec())
							{
								expectA13 = false;
								std::ostringstream bufff;
								bufff << delta.getUsec() << " microsec";

								// in some cases it might be normal for the controller to not respond,
								// for example, whenever the command is the same as the previous command or when
								// go on-line sent to a controller already on-line
								core.LogMessage(&device, (std::string("command echo timeout after ") + bufff.str()).c_str(), !responseRequired);
								break;
							}
							CDeviceUtils::SleepMs(5);
							// if we are not at the end of the 'alternate' echo, iterate forward in that alternate echo string
							if (alternateEcho.end() != allowedResponse)
								++allowedResponse;
						} // loop over timeout
					} // the command was echoed  entirely...
					if (expectA13)
					{
						MM::MMTime startTime = core.GetCurrentMMTime();
						// now look for a 13 - this indicates that the command has really completed!
						unsigned char answer = 0;
						unsigned long read;
						for (;;)
						{
							answer = 0;
							int status = core.ReadFromSerial(&device, port.c_str(), &answer, 1, read);
							if (DEVICE_OK != status)
							{
								core.LogMessage(&device, "ReadFromComPort failed", false);
								return status;
							}
							if (0 < read)
								response.push_back(answer);
							if (answer == 13) // CR  - sometimes the SC sends a 1 before the 13....
							{
								responseReceived = true;
								break;
							}
							if (0 < read)
							{
								std::ostringstream bufff;
								bufff.flags(std::ios::hex | std::ios::showbase);
								bufff << (unsigned int)answer;
								core.LogMessage(&device, ("error, extraneous response  " + bufff.str()).c_str(), false);
							}

							MM::MMTime del2 = core.GetCurrentMMTime() - startTime;
							if (1000.0 * answerTimeoutMs < del2.getUsec())
							{
								std::ostringstream bufff;
								MM::MMTime del3 = core.GetCurrentMMTime() - commandStartTime;
								bufff << "command completion timeout after " << del3.getUsec() << " microsec";
								core.LogMessage(&device, bufff.str().c_str(), false);
								break;
							}
						}
					}
				} // response required
				else // no response required / expected
				{
					CDeviceUtils::SleepMs(5); // docs say controller echoes any command within 100 microseconds
					// 3 ms is enough time for controller to send 3 bytes @  9600 baud
					core.PurgeSerial(&device, port.c_str());
				}
			}
			else
			{
				core.LogMessage(&device, "WriteToComPort failed!", false);
			}
			g_Busy[port] = false;
		}// a command was specified

		return responseRequired ? (responseReceived ? DEVICE_OK : DEVICE_ERR) : DEVICE_OK;
	}// not busy...
	else
	{
		core.LogMessage(&device, ("Sequence error, port " + port + " was busy!").c_str(), false);
		ret = DEVICE_INTERNAL_INCONSISTENCY;
	}

	return ret;

}

// some commands don't send a \r!!!
int SutterUtils::SetCommandNoCR(MM::Device& device, MM::Core& core,
	const std::string port, const std::vector<unsigned char> command, const std::vector<unsigned char> alternateEcho,
	const unsigned long answerTimeoutMs, std::vector<unsigned char>& response, const bool responseRequired)
{

	return  SutterUtils::SetCommand(device, core, port, command, alternateEcho, answerTimeoutMs, response, responseRequired, false);
}
