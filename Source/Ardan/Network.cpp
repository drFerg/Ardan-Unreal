// Author: Fergus Leahy

#include "Ardan.h"
#include "Network.h"
#include "Sockets.h"

//#pragma comment(lib, "legacy_stdio_definitions.lib")
//#pragma comment(lib,"Ws2_32.lib")
//#pragma comment(lib,"wsock32.lib")
//extern "C" FILE* __cdecl __iob_func()
//{
//	struct _iobuf_VS2012 { // ...\Microsoft Visual Studio 11.0\VC\include\stdio.h #56
//		char *_ptr;
//		int   _cnt;
//		char *_base;
//		int   _flag;
//		int   _file;
//		int   _charbuf;
//		int   _bufsiz;
//		char *_tmpfname;
//	};
//	 VS2015 has only FILE = struct {void*}
//
//	int const count = sizeof(_iobuf_VS2012) / sizeof(FILE);
//
//	// stdout
//	return (FILE*)(&(__acrt_iob_func(1)->_Placeholder) - count);
//
//	 stderr
//	return (FILE*)(&(__acrt_iob_func(2)->_Placeholder) - 2 * count);
//}
//extern "C" {
//#include "srpc.h"
//}

#include "SocketSubsystem.h"
#include "Engine/Channel.h"
#include "IPAddress.h"
#include "EngineUtils.h"
//#if PLATFORM_WINDOWS
//#include "WindowsHWrapper.h"
//#include "AllowWindowsPlatformTypes.h"
//#endif
//#include <srpc.h>
//#if PLATFORM_WINDOWS
//#include "HideWindowsPlatformTypes.h"
//#endif
uint32 maxPortCountToTry = 10;

Network::Network()
{
}

/* Creates and returns a socket */
FSocket* Network::createSocket(ISocketSubsystem* sockSubSystem, int port,
									  bool bReuseAddrPort)
{
	
	//rpc_init(0);
	if (sockSubSystem == NULL)
	{
		UE_LOG(LogNet, Warning,
				TEXT("NET:: CreateSocket: Unable to find socket subsystem"));
		return NULL;
	}

	FSocket* socket = sockSubSystem->CreateSocket(NAME_DGram, TEXT("UDPCONN"),
												  true);
	if(socket == NULL)
	{
		socket = 0;
		UE_LOG(LogNet, Warning, TEXT("SOCK: socket failed (%i)"),
				(int32)sockSubSystem->GetLastErrorCode());
		return NULL;
	}

	if (socket->SetReuseAddr(bReuseAddrPort) == false)
	{
		UE_LOG(LogNet, Warning, TEXT("setsockopt with SO_REUSEADDR failed"));
	}

	if (socket->SetRecvErr() == false)
	{
		UE_LOG(LogNet, Warning, TEXT("setsockopt with IP_RECVERR failed"));
	}

	int32 RecvSize = 0x8000;
	int32 SendSize = 0x8000;
	socket->SetReceiveBufferSize(RecvSize, RecvSize);
	socket->SetSendBufferSize(SendSize, SendSize);
	UE_LOG(LogInit, Log, TEXT("%s: Socket queue %i / %i"),
			sockSubSystem->GetSocketAPIName(), RecvSize, SendSize);

	/* Bind socket and set port. */
	TSharedPtr<FInternetAddr> localAddr = sockSubSystem->GetLocalBindAddr(*GLog);
	//localAddr->SetPort(port);
	int32 attemptPort = localAddr->GetPort();
	int32 boundPort = sockSubSystem->BindNextPort(socket, *localAddr,
												  maxPortCountToTry + 1, 1);
	if (boundPort == 0)
	{
		UE_LOG(LogNet, Log, TEXT("%s: binding to port %i failed (%i)"),
				sockSubSystem->GetSocketAPIName(),
				attemptPort,
				(int32)sockSubSystem->GetLastErrorCode());
		free(socket);
		return NULL;
	}

	if(socket->SetNonBlocking() == false)
	{
		UE_LOG(LogNet, Log, TEXT("%s: SetNonBlocking failed (%i)"),
									sockSubSystem->GetSocketAPIName(),
									(int32)sockSubSystem->GetLastErrorCode());
		free(socket);
		return NULL;
	}
	return socket;
}


/* For sending */
// FString address = TEXT("127.0.0.1");
// int32 port = 19834;
// FIPv4Address ip;
// FIPv4Address::Parse(address, ip);


// TSharedRef addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
// addr->SetIp(ip.GetValue());
// addr->SetPort(port);

Network::~Network()
{
}
