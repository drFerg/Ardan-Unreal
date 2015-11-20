// Author: Fergus Leahy

#pragma once

#include "IPAddress.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
/**
 * 
 */
class VIRTUAL_CPS_WORLD_API Network
{
public:
	Network();
	static FSocket* createSocket(ISocketSubsystem* sockSubSystem, int port,
			  	  	  	  	  	 bool bReuseAddrPort);
	~Network();
};
