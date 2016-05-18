// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Sensor.h"

#define LED_PKT 0
#define RADIO_PKT 1

class FRunnableConnection : public FRunnable {
	static FRunnableConnection* Runnable;
public:


	FRunnableConnection(int port, TQueue<uint8*, EQueueMode::Spsc> *packetQ);
	virtual ~FRunnableConnection();

	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Shutdown();

	static FRunnableConnection* create(int port, TQueue<uint8*, EQueueMode::Spsc> *packetQ);
private:

	FRunnableThread* thread;
	/** Underlying socket communication */
	bool bUDPActive = false;
	ISocketSubsystem *sockSubSystem;
	FSocket* socket;
	int dstport = 5000;
	TQueue<uint8*, EQueueMode::Spsc> *pktQ;
	/** Local address this net driver is associated with */
	bool stop = false;
	bool complete = false;

	/* Storage to receive Cooja packets into */
	uint8 data[MAX_PACKET_SIZE];
	int32 bytesRead = 0;

};
