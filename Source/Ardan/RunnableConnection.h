// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Sensor.h"
#if PLATFORM_WINDOWS
#include "WindowsHWrapper.h"
#include "AllowWindowsPlatformTypes.h"
#endif
//#include <cppkafka/producer.h>
#include <librdkafka/rdkafka.h>
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#define LED_PKT 0
#define RADIO_PKT 1
struct pkt {
	size_t size;
	uint8_t* data;
};

class FRunnableConnection : public FRunnable {
	static FRunnableConnection* Runnable;
public:


	FRunnableConnection(int port, TQueue<rd_kafka_message_t *, EQueueMode::Spsc> *packetQ);
	virtual ~FRunnableConnection();

	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Shutdown();

	static FRunnableConnection* create(int port, TQueue<rd_kafka_message_t *, EQueueMode::Spsc> *packetQ);
private:
	rd_kafka_t *rk;         /* Producer instance handle */
	rd_kafka_topic_t *rkt;  /* Topic object */
	rd_kafka_topic_partition_list_t *topics;
	FRunnableThread* thread;
	/** Underlying socket communication */
	bool bUDPActive = false;
	ISocketSubsystem *sockSubSystem;
	FSocket* socket;
	int dstport = 5000;
	TQueue<rd_kafka_message_t *, EQueueMode::Spsc> *pktQ;
	/** Local address this net driver is associated with */
	bool stop = false;
	bool complete = false;

	/* Storage to receive Cooja packets into */
	uint8 data[MAX_PACKET_SIZE];
	int32 bytesRead = 0;
	void msg_consume(rd_kafka_message_t *rkmessage);

};
