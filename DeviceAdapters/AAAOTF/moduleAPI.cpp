#include "../../MMDevice/ModuleInterface.h"
#include "AAAOTF.h"
#include "AOTF.h"

const char* g_AOTF = "AAAOTF";
const char* g_Int = "Power (% of max)";
const char* g_Maxint = "Maximum intensity (dB)";
const char* g_mChannel = "Channels (8 bit word 1..255)";		
const char* g_DelayBetweenChannels = "Delay between channels (ms)";

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
	RegisterDevice(g_AOTF, MM::GenericDevice, "AAAOTF");
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
