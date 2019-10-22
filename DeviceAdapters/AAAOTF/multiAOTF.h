#pragma once

#include "AAAOTF.h"

class multiAOTF : public CShutterBase<multiAOTF>
{
public:
   multiAOTF();
   ~multiAOTF();
  
   // Device API
   // ----------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy();

   // Shutter API
   // ---------
   int SetOpen(bool open);
   int GetOpen(bool& open);
   int Fire(double /*interval*/) {return DEVICE_UNSUPPORTED_COMMAND; }


   // action interface
   // ----------------
   int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct);
   //int OnIntensity(MM::PropertyBase* pProp, MM::ActionType eAct);
   //int OnVersion(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnDelayBetweenChannels(MM::PropertyBase* pProp, MM::ActionType eAct);

private:

   //int SetIntensity(int intensity);
	
   int SetShutterPosition(bool state);
   //int GetVersion();

   // MMCore name of serial port
   std::string port_;
   // Command exchange with MMCore                                           
   std::string command_;           
   // close (0) or open (1)
   int state_;
   bool initialized_;
   // channels that we are currently working on 
   int activeMultiChannels_;
   // milliseconds to wait between the per-channel on/off commands
   double delayBetweenChannels_;
};