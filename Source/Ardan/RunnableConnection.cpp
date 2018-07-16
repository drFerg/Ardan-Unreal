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
	rd_kafka_topic_conf_t *topic_conf;
	/* Initialise networking */
	topic_conf = rd_kafka_topic_conf_new();
	rd_kafka_conf_t *conf = rd_kafka_conf_new();
	rd_kafka_conf_set(conf, "fetch.wait.max.ms", "0", NULL, 0);
	rd_kafka_conf_set(conf, "fetch.error.backoff.ms", "1", NULL, 0);
	rd_kafka_conf_set(conf, "socket.blocking.max.ms", "1", NULL, 0);

	char* group = "rdkafka_consumer_example";
	if (rd_kafka_conf_set(conf, "group.id", group,
		errstr, sizeof(errstr)) !=
		RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%% %s\n", errstr);
		exit(1);
	}

	/* Consumer groups always use broker based offset storage */
	if (rd_kafka_topic_conf_set(topic_conf, "offset.store.method",
		"broker",
		errstr, sizeof(errstr)) !=
		RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%% %s\n", errstr);
		exit(1);
	}

	/* Set default topic config for pattern-matched topics. */
	rd_kafka_conf_set_default_topic_conf(conf, topic_conf);
	rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
	if (!rk) {
		fprintf(stderr,
			"%% Failed to create new consumer: %s\n",
			errstr);
		return false;
		}
	UE_LOG(LogNet, Log, TEXT("Connection Runnable createed kafka ref!"));
		/* Add brokers */
		if (rd_kafka_brokers_add(rk, brokers) == 0) {
			fprintf(stderr, "%% No valid brokers specified\n");
			return false;
		}
		rd_kafka_poll_set_consumer(rk);
		/* Create topic */
		//rkt = rd_kafka_topic_new(rk, topic, NULL);
		//topic_conf = NULL; /* Now owned by topic */
		topics = rd_kafka_topic_partition_list_new(1);
		rd_kafka_topic_partition_list_add(topics, "actuator", -1);
											 /* Start consuming */
		UE_LOG(LogNet, Log, TEXT("Connection Runnable created kafka topic part!"));

		err = rd_kafka_subscribe(rk, topics);
		if (err) {
			fprintf(stderr,
				"%% Failed to start consuming topics: %s\n",
				rd_kafka_err2str(err));
		}
		UE_LOG(LogNet, Log, TEXT("Connection Runnable subbed!"));

		return true;
	/*sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = Network::createSocket(sockSubSystem, dstport, true);
	if (socket != NULL) return true;
	else return false;*/
}



/* Runs continously, until killed by endGame */
uint32 FRunnableConnection::Run() {
	UE_LOG(LogNet, Log, TEXT("Connection Thread Started!"));
	rd_kafka_resp_err_t err;

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
	rd_kafka_message_t *rkmessage;
	double detectTime = 0;
	while (!stop) {

		rkmessage = rd_kafka_consumer_poll(rk, 1);
		//UE_LOG(LogNet, Log, TEXT("Spin me around"));
		if (rkmessage) {
			if (is_valid_msg(rkmessage)) {

				pktQ->Enqueue(rkmessage); /* pass on to game-thread */
				detectTime = FPlatformTime::Seconds();
				UE_LOG(LogActor, Warning, TEXT("Received at: %.6f"), detectTime);

			}
			//UE_LOG(LogNet, Log, TEXT("Got msg"));
			//rd_kafka_message_destroy(rkmessage);
		}
	}

	err = rd_kafka_consumer_close(rk);
	if (err)
		fprintf(stderr, "%% Failed to close consumer: %s\n",
			rd_kafka_err2str(err));
	else
		fprintf(stderr, "%% Consumer closed\n");

	rd_kafka_topic_partition_list_destroy(topics);

	rd_kafka_destroy(rk);
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
		UE_LOG(LogNet, Log, TEXT("Connection Thread killed!"));
		thread->WaitForCompletion();
		delete Runnable;
		Runnable = NULL;
		complete = false;
		stop = false;
	}
}


bool FRunnableConnection::is_valid_msg(rd_kafka_message_t *rkmessage) {
	if (rkmessage->err) {
		if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
			fprintf(stderr,
				"%% Consumer reached end of %s [%" PRId32 "] "
				"message queue at offset %" PRId64 "\n",
				rd_kafka_topic_name(rkmessage->rkt),
				rkmessage->partition, rkmessage->offset);
			return false;
		}

		if (rkmessage->rkt) {
			fprintf(stderr, "%% Consume error for "
				"topic \"%s\" [%" PRId32 "] "
				"offset %" PRId64 ": %s\n",
				rd_kafka_topic_name(rkmessage->rkt),
				rkmessage->partition,
				rkmessage->offset,
				rd_kafka_message_errstr(rkmessage));
		}
		else {
			fprintf(stderr, "%% Consumer error: %s: %s\n",
				rd_kafka_err2str(rkmessage->err),
				rd_kafka_message_errstr(rkmessage));
		}

		if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
			rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC)
			stop = true;
		return false;
	}

	fprintf(stdout, "%% Message (topic %s [%" PRId32 "], "
		"offset %" PRId64 ", %zd bytes):\n",
		rd_kafka_topic_name(rkmessage->rkt),
		rkmessage->partition,
		rkmessage->offset, rkmessage->len);
			/* Data available, non-blocking read */
	return true;
}
