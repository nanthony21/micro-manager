#pragma once
#include "../MMDevice/DeviceBase.h"
class Mux :
	public CGenericBase<Mux>
{
	Mux();
	~Mux();

	int Shutdown();
	int Initialize();



};

