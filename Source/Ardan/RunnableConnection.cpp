// Fill out your copyright notice in the Description page of Project Settings.

#include "Ardan.h"
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
	check(thread);
	UE_LOG(LogNet, Log, TEXT("Created Connection Thread!"));
}

FRunnableConnection::~FRunnableConnection() {
	delete thread;
	thread = NULL;
}

FRunnableConnection* FRunnableConnection::create(int port,
															TQueue<uint8*, EQueueMode::Spsc> *packetQ) {
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading()) {
		Runnable = new FRunnableConnection(port, packetQ);
		check(Runnable);
		UE_LOG(LogNet, Log, TEXT("Created Connection Runnable!"));
		return Runnable;
	}
	UE_LOG(LogNet, Log, TEXT("Connection Runnable already running!"));
	return NULL;
}


bool FRunnableConnection::Init() {
	/* Initialise networking */
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = Network::createSocket(sockSubSystem, dstport, true);
	if (socket != NULL) return true;
	else return false;
}



/* Runs continously, until killed by endGame */
uint32 FRunnableConnection::Run() {
	UE_LOG(LogNet, Log, TEXT("Connection Thread Started!"));
	TSharedRef<FInternetAddr> fromAddr = sockSubSystem->CreateInternetAddr();

	uint8* pkt;
	uint32 size;
 	while (!stop) { /* Run until told to stop */
		while (socket->HasPendingData(size)) {
			pkt = (uint8*) malloc(size);
			/* Data available, non-blocking read */
			socket->RecvFrom(pkt, size, bytesRead, *fromAddr);
			pktQ->Enqueue(pkt); /* pass on to game-thread */
		}
 	}
	socket->Close();
	ISocketSubsystem::Get()->DestroySocket(socket);
	complete = true;
	UE_LOG(LogNet, Log, TEXT("Connection Thread Stopped!"));
	return 0;
}

void FRunnableConnection::Stop() {
	stop = true;
	// socket->Close();
	// ISocketSubsystem::Get()->DestroySocket(socket);
}

void FRunnableConnection::Shutdown() {
	if (Runnable) {
		thread->WaitForCompletion();
		delete Runnable;
		Runnable = NULL;
		complete = false;
		stop = false;
	}
}
