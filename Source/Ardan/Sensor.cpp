// Author: Fergus Leahy

#include "Ardan.h"
#include "ArdanGameMode.h"
#include "ArdanPlayerController.h"
#include "Engine/TriggerBox.h"
#include "Sensor.h"
#include "IPAddress.h"
#include "IPv4SubnetMask.h"

#include "IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Engine/Channel.h"
#include <Vector.h>
#include <Map.h>
#include "RunnableConnection.h"
#include "DrawDebugHelpers.h"
#include "ArdanUtilities.h"

using namespace UnrealCoojaMsg;

#define LEDON 100000.0
#define LEDOFF 0

TArray<AActor*> inMotionRange;
TMap<AActor*, FVector> inMotionPos; /* Contains actor as key and old location */

void ASensor::sendMsgToSim(MsgType type) {
	fbb.Clear();
	MessageBuilder msg(fbb);
	msg.add_id(ID);
	msg.add_type(type);
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
		sent, *addr);
	if (rd_kafka_produce(
		/* Topic object */
		rkt,
		/* Use builtin partitioner to select partition*/
		RD_KAFKA_PARTITION_UA,
		/* Make a copy of the payload. */
		RD_KAFKA_MSG_F_COPY,
		/* Message payload (value) and length */
		fbb.GetBufferPointer(), fbb.GetSize(),
		/* Optional key and its length */
		NULL, 0,
		/* Message opaque, provided in
		* delivery report callback as
		* msg_opaque. */
		NULL) == -1) {
		/**
		* Failed to *enqueue* message for producing.
		*/
		fprintf(stderr,
			"%% Failed to produce to topic %s: %s\n",
			rd_kafka_topic_name(rkt),
			rd_kafka_err2str(rd_kafka_last_error()));
		UE_LOG(LogNet, Log, TEXT("--KAFKAFAIL--"));
	}
	rd_kafka_poll(rk, 0/*non-blocking*/);
}

void ASensor::OnBeginOverlap(class AActor* OverlappedActor, class AActor* otherActor) {
	/* If in replay ignore overlaps - recorded state will already reflect these */
	if (bReplayMode) return;
	/* Start motion tracking the overlapping otherActor (in tick) */
	inMotionPos.Add(otherActor, otherActor->GetActorLocation());
	UE_LOG(LogNet, Log, TEXT("%s: Someone entered (%s)"), *(this->GetName()), *(otherActor->GetName()));
	if (!active) return;
	
	sendMsgToSim(MsgType_PIR);
	active = false;
}

void ASensor::OnEndOverlap(class AActor* OverlappedActor, class AActor* otherActor) {
	/* If in replay ignore overlaps - recorded state will already reflect these */
	if (bReplayMode) return;
	inMotionRange.Remove(otherActor); /* Remove otherActor from motion tracking */
	//UE_LOG(LogNet, Log, TEXT("%s: Someone left (%s)"), *(this->GetName()), *(otherActor->GetName()));

	if (!active) return;
	
	sendMsgToSim(MsgType_PIR);
	active = false;
}

ASensor::ASensor() {
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	/* Networking setup */
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = sockSubSystem->CreateSocket(NAME_DGram, TEXT("UDPCONN2"), true);
	if (socket) UE_LOG(LogNet, Log, TEXT("Created Socket"));
	socket->SetReceiveBufferSize(RecvSize, RecvSize);
	socket->SetSendBufferSize(SendSize, SendSize);
}


// Called when the game starts or when spawned
void ASensor::BeginPlay() {
	Super::BeginPlay();
	
	/* Set up destination address:port to send Cooja messages to */
	FIPv4Address::Parse(address, ip);
	addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.Value);
	addr->SetPort(port);


	rd_kafka_conf_t *conf;  /* Temporary configuration object */
	char errstr[512];       /* librdkafka API error reporting buffer */
	char *buf = "hello";          /* Message value temporary buffer */
	const char *brokers = "localhost:9092";    /* Argument: broker list */
	const char *topic = "sensor"; /* Argument: topic to produce to */

	conf = rd_kafka_conf_new();
	rd_kafka_conf_set(conf, "queue.buffering.max.ms", "0", NULL, 0);
	rd_kafka_conf_set(conf, "group.id", "rdkafka_consumer_example", NULL, 0);
	rd_kafka_conf_set(conf, "batch.num.messages", "1", NULL, 0);


	/* Set bootstrap broker(s) as a comma-separated list of
	* host or host:port (default port 9092).
	* librdkafka will use the bootstrap brokers to acquire the full
	* set of brokers from the cluster. */
	if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers,
		errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		fprintf(stderr, "%s\n", errstr);
	}
	/*
	* Create producer instance.
	*
	* NOTE: rd_kafka_new() takes ownership of the conf object
	*       and the application must not reference it again after
	*       this call.
	*/
	rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
	if (!rk) {
		fprintf(stderr,
			"%% Failed to create new producer: %s\n", errstr);
		//return 1;
	}
	rkt = rd_kafka_topic_new(rk, topic, NULL);
	if (!rkt) {
		fprintf(stderr, "%% Failed to create topic object: %s\n",
			rd_kafka_err2str(rd_kafka_last_error()));
		rd_kafka_destroy(rk);
		//return 1;
	}

	mesh = (UStaticMeshComponent*) this->GetComponentByClass(UStaticMeshComponent::StaticClass());
	if (mesh) {
		UE_LOG(LogNet, Log, TEXT("SensorName: %s"), *(mesh->GetName()));
		ColourSensor(0);
		/* Retrieve all the LEDS and turn them OFF */
		TArray<USceneComponent*> components;
		mesh->GetChildrenComponents(true, components);
		UE_LOG(LogNet, Log, TEXT("SensorName: %d"), components.Num());
		for (USceneComponent *c : components) {
			USpotLightComponent *l = (USpotLightComponent*)c;
			Leds.Add(l);
			UE_LOG(LogNet, Log, TEXT("%s owned by %s"), *(l->GetName()), *(l->GetOwner()->GetName()));
			l->SetIntensity(LEDOFF);
		}
	}
	else {
		UE_LOG(LogNet, Log, TEXT("SensorName:No mesh spawned!"));
	}

	if (PIRSensor) {
		UE_LOG(LogNet, Log, TEXT("SensorName:Have PIR sensor"));
		PIRSensor->OnActorBeginOverlap.AddDynamic(this, &ASensor::OnBeginOverlap);
		PIRSensor->OnActorEndOverlap.AddDynamic(this, &ASensor::OnEndOverlap);
		active = true;
	}

	history = new FSensorHistory();
	state.R = 0;
	state.G = 0;
	state.B = 0;
	SnapshotState(0.0);

	prevLocation = this->GetActorLocation();
	sendLocationUpdate();
	UE_LOG(LogNet, Log, TEXT("%d %s"), ID,
				*(this->GetActorLocation().ToString()));



}

// Called every frame
void ASensor::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	/* Check if actors located within PIR detection range have moved,
	 * checking their current location vs previous */

	if (prevLocation.Equals(this->GetActorLocation(), 10) == false) {
		sendLocationUpdate();
		prevLocation = this->GetActorLocation();
	}
	activeTimer += DeltaTime;
	if (activeTimer > PIRRepeatTime) {
		active = true;
		activeTimer = 0;
		DetectFire();
	}
}

void ASensor::SetReplayMode(bool on) {
	bReplayMode = on;
}
void ASensor::Led(int32 led, bool on) {
	if (led > 3 && led < 0) return;
	Leds[led]->SetIntensity(on ? LEDON : LEDOFF);
}

void ASensor::SetLed(uint8 R, uint8 G, uint8 B) {
	state.R = R;
	state.G = G;
	state.B = B;
	Leds[0]->SetIntensity(R ? LEDON : LEDOFF);
	Leds[1]->SetIntensity(G ? LEDON : LEDOFF);
	Leds[2]->SetIntensity(B ? LEDON : LEDOFF);
	if (Light1) Light1->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
	if (Light2) Light2->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
	if (Light3) Light3->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
	if (overlay) {
		FVector sourceLoc = this->GetSensorLocation();
		if (R) DrawDebugCircle(GetWorld(), sourceLoc, 50.0, 360, FColor(255, 0, 0), false, 0.5, 0, 5, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		if (G) DrawDebugCircle(GetWorld(), sourceLoc, 40.0, 360, FColor(0, 255, 0), false, 0.5, 0, 5, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		if (B) DrawDebugCircle(GetWorld(), sourceLoc, 30.0, 360, FColor(0, 0, 255), false, 0.5, 0, 5, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}
	if (evac_sign) {
		if (G) evac_sign->direct_left();
		if (B) evac_sign->direct_right();
	}

}

void ASensor::SetSelected() {
	DrawDebugCircle(GetWorld(), this->GetActorLocation(), 150.0, 360, FColor(0, 255, 0), false, 0.3, 0, 5, FVector(0.f,1.f,0.f), FVector(0.f,0.f,1.f), false);
}

FVector ASensor::GetSensorLocation() {
	return this->GetActorLocation();
}

void ASensor::sendLocationUpdate() {
	FVector locVec = this->GetActorLocation();

	fbb.Clear();
	MessageBuilder msg(fbb);

	msg.add_id(ID);
	msg.add_type(MsgType_LOCATION);
	msg.add_location(new Vec3(locVec.X, locVec.Y, locVec.Z));
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
									 sent, *addr);


	if (rd_kafka_produce(
		/* Topic object */
		rkt,
		/* Use builtin partitioner to select partition*/
		RD_KAFKA_PARTITION_UA,
		/* Make a copy of the payload. */
		RD_KAFKA_MSG_F_COPY,
		/* Message payload (value) and length */
		fbb.GetBufferPointer(), fbb.GetSize(),
		/* Optional key and its length */
		NULL, 0,
		/* Message opaque, provided in
		* delivery report callback as
		* msg_opaque. */
		NULL) == -1) {
		/**
		* Failed to *enqueue* message for producing.
		*/
		fprintf(stderr,
			"%% Failed to produce to topic %s: %s\n",
			rd_kafka_topic_name(rkt),
			rd_kafka_err2str(rd_kafka_last_error()));
		UE_LOG(LogNet, Log, TEXT("--KAFKAFAIL--"));
	}
	rd_kafka_poll(rk, 0/*non-blocking*/);
}

void ASensor::ReceivePacket(const UnrealCoojaMsg::Message* msg) {
	if (msg->type() == MsgType_LED) {
		SetLed(msg->led()->Get(0), msg->led()->Get(1), msg->led()->Get(2));
	}
	else if (msg->type() == MsgType_RADIO_DUTY) {
		state.radioDuty = msg->node()->Get(ID)->radioOnRatio();
		state.radioTXRatio = msg->node()->Get(ID)->radioTxRatio();
		state.radioRXRatio = msg->node()->Get(ID)->radioRxRatio();
		state.radioIXRatio = msg->node()->Get(ID)->radioInterferedRatio();
		//UE_LOG(LogNet, Log, TEXT("SENSOR: RadioDuty: %f|%f|%f"), state.radioDuty, state.radioTXRatio, state.radioRXRatio);
		SnapshotState(GetWorld()->TimeSeconds);
	}
}

void ASensor::SnapshotState(float timeStamp) {
	//if (!bstateBeenModified) return;
	FSensorState s;
	s = state;
	s.timeStamp = timeStamp;
	//UE_LOG(LogNet, Log, TEXT("SNAPSENSOR: RadioDuty: %f|%f|%f"), state.radioDuty, state.radioTXRatio, state.radioRXRatio);
	//UE_LOG(LogNet, Log, TEXT("SNAPSENSOR: RadioDuty: %f|%f|%f"), s.radioDuty, s.radioTXRatio, s.radioRXRatio);
	history->timeline.Add(s);
}

void ASensor::RewindState(float requestTime) {
	int i = history->timeline.Num() - 1;
	FSensorState* s = &(history->timeline[i]);
	while (i > 0 && s->timeStamp > requestTime) {
		s = &(history->timeline[--i]);
	}
	state = *s;
}


void ASensor::ReplayState(float timeStamp) {
	state = *GetStatefromTimeline(history, timeStamp);
}

void ASensor::ChangeTimeline(int index) {
	histories.Push(history);
	history = histories[index];
	ResetTimeline();
	ReflectState();
}

FSensorState* ASensor::GetStatefromTimeline(FSensorHistory* h, float timeStamp) {
	FSensorState* s = NULL;
	while (h->index < h->timeline.Num() && h->timeline[h->index].timeStamp <= timeStamp) {
		s = &(h->timeline[h->index]);
		h->currentState = s;
		h->index++;
	}
	UE_LOG(LogNet, Log, TEXT("GET: %d:%f:%f (%d)"), h->index - 1, timeStamp, h->currentState->timeStamp, h->timeline.Num());
	return s ? s : h->currentState;
}

FSensorState* ASensor::GetStatefromTimeline(int index, float timeStamp) {
	return GetStatefromTimeline(histories[index], timeStamp);
}

bool ASensor::StateIsEqual(FSensorState& a, FSensorState& b) {
	UE_LOG(LogNet, Log, TEXT("RADIO DIFF: %f:%f:%f - %f:%f:%f (%f)"), a.radioRXRatio, a.radioTXRatio, a.radioIXRatio, b.radioRXRatio, b.radioTXRatio, b.radioIXRatio, b.timeStamp);
	return (a.radioRXRatio == b.radioRXRatio);
	//return (a.R == b.R && a.G == b.G && a.B == a.B);
}

bool ASensor::DiffCurrentState(int stateIndex, float timeStamp) {
	FSensorState* s = GetStatefromTimeline(stateIndex, timeStamp);
	UE_LOG(LogNet, Log, TEXT("GSFT: SENSOR: R%dG%dB%d %f"), state.R, state.G, state.B, state.timeStamp);
	UE_LOG(LogNet, Log, TEXT("GSFT: SENSOR: R%dG%dB%d %f"), s->R, s->G, s->B, s->timeStamp);
	return StateIsEqual(state, *s);
}

/*Reflects the stored state on the virtual object*/
void ASensor::ReflectState() {
	SetLed(state.R, state.G, state.B);
}

void ASensor::ResetTimeline() {
	/* State has already been stored in history */
	//delete state;
	history->index = 0;
	state = history->timeline[0];
	history->currentState = &state;
}

void ASensor::NewTimeline() {
	histories.Push(history);
	history = new FSensorHistory();
	bstateBeenModified = true;
}

void ASensor::ColourSensor(int type) {
	UMaterialInstance *mat = NULL;
	if (type == 0) {
		mat = ArdanUtilities::LoadObjFromPath<UMaterialInstance>(TEXT("MaterialInstanceConstant'/Game/Materials/SensorStatus_Normal.SensorStatus_Normal'"));
	}
	else if (type == 1) {
		if (!scaled) {
			scaled = true;
			this->SetActorRelativeScale3D(this->GetActorRelativeScale3D() * 2.0f);
		}
		mat = ArdanUtilities::LoadObjFromPath<UMaterialInstance>(TEXT("MaterialInstanceConstant'/Game/Materials/SensorStatus_Warning.SensorStatus_Warning'"));
	}
	if (mesh) mesh->SetMaterial(0, mat);
}

void ASensor::DetectFire() {
	bool fire = false;
	TObjectIterator<AArdanPlayerController> ThePC;
	for (UParticleSystemComponent *p: ThePC->firePoints) {
		FVector direction = GetActorLocation() - p->GetComponentLocation();
		if (direction.Size() < FireDetectionRadius) {
			//UE_LOG(LogNet, Log, TEXT("FIRE detected at sensor %d: %s"), ID, (evac_sign?"Y":"N"));
			fire = true;
		}
	}
	if (fire) {
		sendMsgToSim(MsgType_FIRE);
		if (evac_sign) evac_sign->direct_left();
	}
}
