// Author: Fergus Leahy

#pragma once

#include "IPAddress.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
/**
 * 
 */
class ARDAN_API Network
{
public:
	Network();
	static FSocket* createSocket(ISocketSubsystem* sockSubSystem, int port,
			  	  	  	  	  	 bool bReuseAddrPort);
	~Network();
};
