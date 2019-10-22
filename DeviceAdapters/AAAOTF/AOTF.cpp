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
   /*,*/
   /*version_("Undefined")*/
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
	std::ostringstream command;
	int i;


	if (initialized_)
      return DEVICE_OK;
      
   // set property list
   // -----------------

   // State
   //------------------


   CPropertyAction* pAct = new CPropertyAction(this, &AOTF::OnState);
   int ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);

   if (ret!=DEVICE_OK)
	   return ret;

   AddAllowedValue(MM::g_Keyword_State, "0");
   AddAllowedValue(MM::g_Keyword_State, "1");

   // Intensity
   //--------------------
   pAct = new CPropertyAction(this, &AOTF::OnIntensity);
   ret = CreateProperty(g_Int, "100", MM::Float, false, pAct);
   if (ret!=DEVICE_OK)
	   return ret;
   SetPropertyLimits(g_Int, 0, 100);

   // Maximumintensity (in dB)
   //-------------------
   pAct = new CPropertyAction(this, &AOTF::OnMaxintensity);
   ret = CreateProperty(g_Maxint, "1900", MM::Integer, false, pAct);
   if (ret!=DEVICE_OK)
	   return ret;
   SetPropertyLimits(g_Maxint, 0, 2200);

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
   if (ret != DEVICE_OK)                                                     
      return ret;


   //switch AOTF to internal mode
   SendSerialCommand(port_.c_str(), "I0", "\r");
   if (ret != DEVICE_OK)                                                     
      return ret;

   // switch all channels off on startup instead of querying which one is open
   SetProperty(MM::g_Keyword_State , "0");

   ret = UpdateStatus();                                                 
   if (ret != DEVICE_OK)                                                     
      return ret;


	command.str("");
	for(i=1;i<=8;i++) {
		command<< "L" << i << "O0\r";
	}

	ret = SendSerialCommand(port_.c_str(), command.str().c_str(), "\r");
	if (ret!=DEVICE_OK)
	   return ret;


   initialized_ = true;

   return DEVICE_OK;                                                         
}  

int AOTF::SetOpen(bool open)
{  
   long pos;
   if(open)
	   pos=1;
   else
	   pos=0;

   return SetProperty(MM::g_Keyword_State, CDeviceUtils::ConvertToString(pos));
} 

int AOTF::GetOpen(bool& open)
{     
   char buf[MM::MaxStrLength];
   int ret = GetProperty(MM::g_Keyword_State, buf);

   if (ret != DEVICE_OK)                                                     
      return ret;
   long pos = atol(buf);
	   pos==1 ? open=true : open = false;
   
   return DEVICE_OK;                                                         
} 




int AOTF::Shutdown()                                                
{                                                                            
   if (initialized_)                                                         
   {                                                                         
      initialized_ = false;                                                  
   }                                                                         
   return DEVICE_OK;                                                         
}                                                                            

// Never busy because all commands block
bool AOTF::Busy()
{
   return false;
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
      }
                                                                             
      pProp->Get(port_);                                                     
   }                                                                         
   return DEVICE_OK;     
}

int AOTF::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {                                                                         
      // instead of relying on stored state we could actually query the device
      pProp->Set((long)state_);                                                          
   }                                                                         
   else if (eAct == MM::AfterSet)
   {
      long pos;
      pProp->Get(pos);

      return SetShutterPosition(pos == 0 ? false : true);
   }
   return DEVICE_OK;
}


int AOTF::OnIntensity(MM::PropertyBase* pProp, MM::ActionType eAct)
{

	if (eAct == MM::BeforeGet)
   {                                                                         
      // instead of relying on stored state we could actually query the device
      pProp->Set((double)intensity_);                                                          
   }                                                                         
   else if (eAct == MM::AfterSet)
   {
      double pos;
      pProp->Get(pos);
      return SetIntensity(pos);
   }
   return DEVICE_OK;
}

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
}
int AOTF::OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(activeChannel_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      // if there is a channel change and the shutter was open, re-open in the new position
      std::string tmpChannel;
      pProp->Get(tmpChannel);
      if (tmpChannel != activeChannel_) {
         activeChannel_ = tmpChannel;
		 if (state_ == 1)
            SetShutterPosition(true);
      }
      // It might be a good idea to close the shutter at this point...
   }
   return DEVICE_OK;
}
