#include "../../MMDevice/ModuleInterface.h"
#include "AAAOTF.h"
#include "AOTF.h"
#include "multiAOTF.h"

const char* g_AOTF = "AAAOTF";
const char* g_Int = "Power (% of max)";
const char* g_Maxint = "Maximum intensity (dB)";
const char* g_mChannel = "Channels (8 bit word 1..255)";
const char* g_Channel_1 = "1";	
const char* g_Channel_2 = "2";		
const char* g_Channel_3 = "3";		
const char* g_Channel_4 = "4";		
const char* g_Channel_5 = "5";			
const char* g_Channel_6 = "6";			
const char* g_Channel_7 = "7";			
const char* g_Channel_8 = "8";			
const char* g_DelayBetweenChannels = "Delay between channels (ms)";

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_AOTF, MM::ShutterDevice, "AAAOTF");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;
  
   if (strcmp(deviceName, g_AOTF) == 0)
   {
       AOTF* s = new AOTF();
       return s;
   }

   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}
