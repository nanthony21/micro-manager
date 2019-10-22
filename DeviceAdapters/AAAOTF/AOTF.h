#pragma once

#include "AAAOTF.h"
#include "MDS.h"

class AOTF : public CGenericBase<AOTF>, public MDS
{
public:
   AOTF();
   ~AOTF();
  
   // Device API
   // ----------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy() {return false;};


   // action interface
   // ----------------
   int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnIntensity(MM::PropertyBase* pProp, MM::ActionType eAct);
   //int OnMaxintensity(MM::PropertyBase* pProp, MM::ActionType eAct);
   //int OnVersion(MM::PropertyBase* pProp, MM::ActionType eAct);

private:      
	std::string port_;
	MDS mds_;
   bool initialized_;
   // channel that we are currently working on 
   std::string activeChannel_;
   int sendSerial(std::string);
   std::string retrieveSerial();
   
   
};
