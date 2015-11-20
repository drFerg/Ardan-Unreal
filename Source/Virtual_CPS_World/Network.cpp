// Author: Fergus Leahy

#include "Virtual_CPS_World.h"
#include "Network.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Engine/Channel.h"
#include "IPAddress.h"
#include "EngineUtils.h"
uint32 maxPortCountToTry = 10;

Network::Network()
{
}

/* Creates and returns a socket */
FSocket* Network::createSocket(ISocketSubsystem* sockSubSystem, int port,
									  bool bReuseAddrPort)
{
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
	localAddr->SetPort(port);
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
