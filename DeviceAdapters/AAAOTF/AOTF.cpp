///////////////////////////////////////////////////////////////////////////////
// FILE:          AOTF.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   AA AOTF
//                
// AUTHOR:        Erwin Peterman 02/02/2010
//

#ifdef WIN32
   #include <windows.h>
   #define snprintf _snprintf 
#endif

#include "AOTF.h"
#include <string>
#include <math.h>

#include <sstream>

//#include <iostream>
//#include <fstream>



AOTF::AOTF() :
   port_("Undefined"),
   state_(0),
   initialized_(false),
   activeChannel_(g_Channel_1),
   intensity_(100),
   maxintensity_(1900)
{
   InitializeDefaultErrorMessages();
                                                                             
   // create pre-initialization properties                                   
   // ------------------------------------                                   
                                                                             
   // Name                                                                   
   CreateProperty(MM::g_Keyword_Name, g_AOTF, MM::String, true); 
                                                                             
   // Description                                                            
   CreateProperty(MM::g_Keyword_Description, "AA AOTF Shutter Controller driver adapter", MM::String, true);
                                                                             
   // Port                                                                   
   CPropertyAction* pAct = new CPropertyAction (this, &AOTF::OnPort);      
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);  

}                                                                            
                                                                             
AOTF::~AOTF()                                                            
{                                                                            
	Shutdown();                                                               
} 

void AOTF::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_AOTF);
}  

int AOTF::Initialize()
{
	{
		using namespace std::placeholders;
		//this->mds_ = MDS(8, std::bind(&AOTF::sendSerial, this, _1), std::bind(&AOTF::retrieveSerial, this, _1));
		this->mds_ = MDS(8, &AOTF::sendSerial, &AOTF::retrieveSerial);
	}
   CPropertyAction* pAct = new CPropertyAction(this, &AOTF::OnState);
   int ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
   if (ret!=DEVICE_OK) {return ret;}
   AddAllowedValue(MM::g_Keyword_State, "0");
   AddAllowedValue(MM::g_Keyword_State, "1");

   // Intensity
   //--------------------
   pAct = new CPropertyAction(this, &AOTF::OnIntensity);
   ret = CreateProperty(g_Int, "100", MM::Float, false, pAct);
   if (ret!=DEVICE_OK) {return ret;}
   SetPropertyLimits(g_Int, 0, 100);

   // Maximumintensity (in dB)
   //-------------------
   /*pAct = new CPropertyAction(this, &AOTF::OnMaxintensity);
   ret = CreateProperty(g_Maxint, "1900", MM::Integer, false, pAct);
   if (ret!=DEVICE_OK) {return ret;}
   SetPropertyLimits(g_Maxint, 0, 2200);*/

   // The Channel we will act on
   // -------

   pAct = new CPropertyAction (this, &AOTF::OnChannel);
   ret = CreateProperty(MM::g_Keyword_Channel, g_Channel_1, MM::String, false, pAct);  

   std::vector<std::string> commands;                                                  
   commands.push_back(g_Channel_1);                                           
   commands.push_back(g_Channel_2);                                            
   commands.push_back(g_Channel_3);   
   commands.push_back(g_Channel_4);  
   commands.push_back(g_Channel_5);  
   commands.push_back(g_Channel_6);                                           
   commands.push_back(g_Channel_7);                                            
   commands.push_back(g_Channel_8);   

   ret = SetAllowedValues(MM::g_Keyword_Channel, commands);
   if (ret != DEVICE_OK) {return ret;}

   ret = UpdateStatus();                                                 
   if (ret != DEVICE_OK)                                                     
      return ret;

   initialized_ = true;
   return DEVICE_OK;                                                         
} 


int AOTF::Shutdown() {                                                        
   if (initialized_) {                                                                         
      initialized_ = false;                                                  
   }                                                                         
   return DEVICE_OK;                                                         
}                                                                            


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
/*
 * Sets the Serial Port to be used.
 * Should be called before initialization
 */
int AOTF::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(port_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      if (initialized_)
      {
         // revert
         pProp->Set(port_.c_str());
         return ERR_PORT_CHANGE_FORBIDDEN;
      } else {                                                                
			pProp->Get(port_);
	  }
   }                                                                         
   return DEVICE_OK;     
}

int AOTF::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {                                                                         
      // instead of relying on stored state we could actually query the device
      pProp->Set(mds_.channels.at(activeChannelNum_).getEnabled());                                                          
   }                                                                         
   else if (eAct == MM::AfterSet)
   {
      bool enabled;
      pProp->Get(enabled);
      return mds_.channels.at(this->activeChannelNum_).setEnabled(enabled);
   }
   return DEVICE_OK;
}


int AOTF::OnIntensity(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{                                                                         
      // instead of relying on stored state we could actually query the device
      pProp->Set(mds_.channels.at(activeChannelNum_).getPower();                                                          
   }                                                                         
   else if (eAct == MM::AfterSet)
   {
      double power;
      pProp->Get(power);
      return mds_.channels.at(activeChannelNum_).setPower(power);
   }
   return DEVICE_OK;
}

/*
int AOTF::OnMaxintensity(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set((float)maxintensity_);
   }
   else if (eAct == MM::AfterSet)
   {
      long tmpMaxint;
      pProp->Get(tmpMaxint);
      if (tmpMaxint != maxintensity_) {
         maxintensity_ = tmpMaxint;
      }
   }
   return DEVICE_OK;;
}*/

int AOTF::OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(activeChannel_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(activeChannel_);
	  activeChannelNum_ = //Set this
   }
   return DEVICE_OK;
}


int AOTF::sendSerial(std::string cmd) {
	this->SendSerialCommand(port_.c_str(), cmd.c_str(), "\r");
}

std::string AOTF::retrieveSerial() {
	std::string ans;
	this->GetSerialAnswer(port_.c_str(), "?", ans); //TODO this won't work need to find a proper delimiter.
	return ans;
}