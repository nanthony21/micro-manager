#ifdef WIN32
#include <windows.h>
#define snprintf _snprintf 
#endif

#include "SuperK.h"

// Device Names
const char* g_HubName = "SuperK Hub";
const char* g_ExtremeName = "Extreme_S4x2_Fianium";
const char* g_VariaName = "Varia";
const char* g_BoosterName = "Booster_2011";
const char* g_FrontPanelName = "Front_Panel_2011";

std::map<uint8_t, const char*> g_devices; //A map of device names keyed by the integer used by NKTDLL to identify the device type. Used to identify which devices are installed.



///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
	g_devices[0x60] = g_ExtremeName;
	g_devices[0x61] = g_FrontPanelName;
	g_devices[0x65] = g_BoosterName;
	g_devices[0x68] = g_VariaName;
	RegisterDevice(g_HubName, MM::HubDevice, g_HubName);
	//RegisterDevice(g_ExtremeName, MM::ShutterDevice, g_ExtremeName);
    RegisterDevice(g_VariaName, MM::GenericDevice, g_VariaName);
	RegisterDevice(g_ExtremeName, MM::ShutterDevice, g_ExtremeName);
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
	if (deviceName == 0) {
	return 0;
	}
	else if (strcmp(deviceName, g_HubName) == 0) {
		SuperKHub* pHub = new SuperKHub();
		return pHub;
	}
	else if (strcmp(deviceName, g_ExtremeName) == 0) {
		return new SuperKExtreme(); 
	}
	else if (strcmp(deviceName, g_VariaName) == 0) {
		return new SuperKVaria();
	}
	else {
		return 0;
	}
}