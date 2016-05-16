// Fill out your copyright notice in the Description page of Project Settings.

#include "Virtual_CPS_World.h"
#include "RunnableConnection.h"
#include "Engine/Channel.h"
#include "IPAddress.h"

#include "EngineUtils.h"
#include "UnrealString.h"
#include "Sensor.h"
#include "Network.h"
#include "DrawDebugHelpers.h"

FRunnableConnection* FRunnableConnection::Runnable = NULL;
FRunnableConnection::FRunnableConnection(int port,
															TQueue<uint8*, EQueueMode::Spsc> *packetQ) {
	dstport = port;
	pktQ = packetQ;
	thread = FRunnableThread::Create(this, TEXT("FRunnableConnection"), 0, TPri_AboveNormal);
		 //windows default = 8mb for thread, could specify more
}

FRunnableConnection* FRunnableConnection::create(int port,
															TQueue<uint8*, EQueueMode::Spsc> *packetQ) {
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading()) {
		Runnable = new FRunnableConnection(port, packetQ);
	}
	return Runnable;
}


bool FRunnableConnection::Init() {
	/* Initialise networking */
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = Network::createSocket(sockSubSystem, dstport, true);
	socket->SetNonBlocking(false);
	if (socket != NULL) return true;
	else return false;
}



/* Called every frame */
uint32 FRunnableConnection::Run()
{

	TSharedRef<FInternetAddr> fromAddr = sockSubSystem->CreateInternetAddr();
	/* Get data, if any. */
	uint8* pkt;
	uint32 size;
 	while (!stop) {
		while (socket->HasPendingData(size)) {
			pkt = (uint8*) malloc(size);
			socket->RecvFrom(pkt, MAX_PACKET_SIZE, bytesRead, *fromAddr);
	 		UE_LOG(LogNet, Log, TEXT("RCV: node %i"), data[0]);
			pktQ->Enqueue(pkt);
		}
 	}
	return 0;
}

void FRunnableConnection::Stop(){
	stop = true;
}
