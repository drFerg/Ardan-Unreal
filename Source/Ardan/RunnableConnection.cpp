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
															TQueue<rd_kafka_message_t *, EQueueMode::Spsc> *packetQ) {
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
															TQueue<rd_kafka_message_t *, EQueueMode::Spsc> *packetQ) {
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
	char errstr[512];
	char *brokers = "localhost:9092";
	rd_kafka_resp_err_t err;
	/* Initialise networking */
	rd_kafka_conf_t *conf = rd_kafka_conf_new();
	if ((rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr)))) {
		fprintf(stderr,
			"%% Failed to create new consumer: %s\n",
			errstr);
		}

		/* Add brokers */
		if (rd_kafka_brokers_add(rk, brokers) == 0) {
			fprintf(stderr, "%% No valid brokers specified\n");
			exit(1);
		}

		/* Create topic */
		//rkt = rd_kafka_topic_new(rk, topic, NULL);
		//topic_conf = NULL; /* Now owned by topic */
		rd_kafka_topic_partition_list_t *topics = rd_kafka_topic_partition_list_new(1);
		rd_kafka_topic_partition_list_add(topics, "actuator", -1);
											 /* Start consuming */
		if ((err = rd_kafka_subscribe(rk, topics))) {
			fprintf(stderr,
				"%% Failed to start consuming topics: %s\n",
				rd_kafka_err2str(err));
		}
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = Network::createSocket(sockSubSystem, dstport, true);
	if (socket != NULL) return true;
	else return false;
}



/* Runs continously, until killed by endGame */
uint32 FRunnableConnection::Run() {
	UE_LOG(LogNet, Log, TEXT("Connection Thread Started!"));
	
 	//while (!stop) { /* Run until told to stop */
		//while (socket->HasPendingData(size)) {
		//	pkt = (uint8*) malloc(size);
		//	p = (struct pkt*) malloc(sizeof(struct pkt));
		//	if (!pkt || !p) {
		//		UE_LOG(LogNet, Log, TEXT("Connection Thread - Malloc FAILURE!"));
		//		stop = true;
		//		break;
		//	}
		//	/* Data available, non-blocking read */

		//	socket->RecvFrom(pkt, size, bytesRead, *fromAddr);
		//	p->size = bytesRead;
		//	p->data = pkt;
		//	pktQ->Enqueue(p); /* pass on to game-thread */
		//}
 	//}

	while (!stop) {
		rd_kafka_message_t *rkmessage;

		rkmessage = rd_kafka_consumer_poll(rk, 1000);
		if (rkmessage) {
			msg_consume(rkmessage);
			//rd_kafka_message_destroy(rkmessage);
		}
	}
	
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


void FRunnableConnection::msg_consume(rd_kafka_message_t *rkmessage) {
	if (rkmessage->err) {
		if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
			/*fprintf(stderr,
				"%% Consumer reached end of %s [%""] "
				"message queue at offset %"PRId64"\n",
				rd_kafka_topic_name(rkmessage->rkt),
				rkmessage->partition, rkmessage->offset);*/
			return;
		}

		if (rkmessage->rkt)
			fprintf(stderr, "%% Consume error for "
				"topic \"%s\" [%d] "
				"offset %d %s\n",
				rd_kafka_topic_name(rkmessage->rkt),
				rkmessage->partition,
				rkmessage->offset,
				rd_kafka_message_errstr(rkmessage));
		else
			fprintf(stderr, "%% Consumer error: %s: %s\n",
				rd_kafka_err2str(rkmessage->err),
				rd_kafka_message_errstr(rkmessage));

		if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
			rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC)
			stop = true;
		return;
	}

		fprintf(stdout, "%% Message (topic %s [%PRId32, offset %PRId64, %zd bytes):\n",
			rd_kafka_topic_name(rkmessage->rkt),
			rkmessage->partition,
			rkmessage->offset, rkmessage->len);

			/* Data available, non-blocking read */
			pktQ->Enqueue(rkmessage); /* pass on to game-thread */
}
