// Author: Fergus Leahy

#include "Ardan.h"
#include "ArdanGameMode.h"
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
#include <flatbuffers/flatbuffers.h>
#include "unrealpkts_generated.h"
#include "DrawDebugHelpers.h"
#include "ArdanUtilities.h"


#define LEDON 4500
#define LEDOFF 0





TArray<AActor*> inMotionRange;
TMap<AActor*, FVector> inMotionPos; /* Contains actor as key and old location */



void ASensor::OnBeginOverlap(class AActor* OverlappedActor,
														 class AActor* otherActor) {
	/* If in replay ignore overlaps - recorded state will already reflect these */
	if (bReplayMode) return;
	/* Start motion tracking the overlapping otherActor (in tick) */
	inMotionPos.Add(otherActor, otherActor->GetActorLocation());
	UE_LOG(LogNet, Log, TEXT("%s: Someone entered (%s)"), *(this->GetName()), *(otherActor->GetName()));
	SetLed(1, 1, 1);
	SnapshotState(GetWorld()->TimeSeconds);
	if (!active) return;
	fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);

	msg.add_id(ID);
	msg.add_type(UnrealCoojaMsg::MsgType_PIR);
	//msg.add_pir()
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
									 sent, *addr);
	active = false;
	//UE_LOG(LogNet, Log, TEXT("Send to %s: %i-%i"), *(addr->ToString(true)), successful, sent);
}

void ASensor::OnEndOverlap(class AActor* OverlappedActor,
													 class AActor* otherActor) {
	/* If in replay ignore overlaps - recorded state will already reflect these */
	if (bReplayMode) return;
	inMotionRange.Remove(otherActor); /* Remove otherActor from motion tracking */
	UE_LOG(LogNet, Log, TEXT("%s: Someone left (%s)"), *(this->GetName()), *(otherActor->GetName()));
	SetLed(0,0,0);
	SnapshotState(GetWorld()->TimeSeconds);

	if (!active) return;
	fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);

	msg.add_id(ID);
	msg.add_type(UnrealCoojaMsg::MsgType_PIR);
	//msg.add_pir()
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);
  active = false;
	//UE_LOG(LogNet, Log, TEXT("Send to %s: %i-%i"), *(addr->ToString(true)), successful, sent);
}

ASensor::ASensor() {
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	/* Check if instantiation is real or just for placement guidlines */
	if (UObject::IsTemplate(RF_Transient)){
		return;
	}

//	UObject* something = StaticLoadObject(UObject::StaticClass(), NULL, TEXT("Blueprint'/Game/SensorNode.SensorNode'"));
//	UBlueprint* bp = Cast<UBlueprint>(something);
//	TSubclassOf<class UObject> MyItemBlueprint;
//	MyItemBlueprint = (UClass*)bp->GeneratedClass;
//	TSubclassOf<class UObject> MyItemBlueprint;
//	MyItemBlueprint = Cast<UClass>(blueprint->GeneratedClass);
	//UStaticMeshComponent* SphereVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualRepresentation"));
	//RootComponent = SphereVisual;
	/* Find the blueprint of the 3D model */
	//UBlueprint* blueprint = Cast<UBlueprint>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("Blueprint'/Game/SensorNode.SensorNode'")));
	//TSubclassOf<class UObject> MyItemBlueprint = (UClass*)(blueprint->GeneratedClass);
	//if (MyItemBlueprint != NULL) {
	//	UE_LOG(LogNet, Log, TEXT("BName: %s %s"), *(MyItemBlueprint->GetClass()->GetName()), *(this->GetActorLocation().ToString()));
	//} else {
	//	UE_LOG(LogNet, Log, TEXT("I got nothing"));
	//}

	/* Spawn the 3D model for the sensor in the virtual world */
	//SensorActor = ArdanUtilities::SpawnBP<AActor>(GetWorld(), MyItemBlueprint, this->GetActorLocation(), this->GetActorRotation());
	
//	AArdanGameMode* gm = ((AArdanGameMode*)GetWorld()->GetAuthGameMode());
//	if (gm)	ID = ((AArdanGameMode*)GetWorld()->GetAuthGameMode())->getNewSensorID();

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
	//TODO: FIX THIS
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
	}

	history = new FSensorHistory();
	state = new FSensorState();
	state->R = 0;
	state->G = 0;
	state->B = 0;
	SnapshotState(0.0);

	prevLocation = this->GetActorLocation();
	sendLocationUpdate();
	UE_LOG(LogNet, Log, TEXT("%d %s"), ID,
				*(this->GetActorLocation().ToString()));
	sendLocationUpdate();
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
	}
}

void ASensor::SetReplayMode(bool on) {
	bReplayMode = on;
}
void ASensor::Led(int32 led, bool on) {
	if (led > 3 && led < 0) return;
//UE_LOG(LogNet, Log, TEXT("Node: %s"), *(SensorActor->GetName()))
	//UE_LOG(LogNet, Log, TEXT("Led: %s (%i)"), *(Leds[led]->GetName()), led);
	Leds[led]->SetIntensity(on ? LEDON : LEDOFF);
}

void ASensor::SetLed(uint8 R, uint8 G, uint8 B) {
	//UE_LOG(LogNet, Log, TEXT("Node: %s"), *(actor->GetName()))
	state->R = R;
	state->G = G;
	state->B = B;
	Leds[0]->SetIntensity(R ? LEDON : LEDOFF);
	Leds[1]->SetIntensity(G ? LEDON : LEDOFF);
	Leds[2]->SetIntensity(B ? LEDON : LEDOFF);
	if (Light1) Light1->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
	if (Light2) Light2->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
	if (Light3) Light3->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
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
	UnrealCoojaMsg::MessageBuilder msg(fbb);

	msg.add_id(ID);
	msg.add_type(UnrealCoojaMsg::MsgType_LOCATION);
	msg.add_location(new UnrealCoojaMsg::Vec3(locVec.X, locVec.Y, locVec.Z));
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
									 sent, *addr);
	//UE_LOG(LogNet, Log, TEXT("Loc update %d (%i-%i)"), ID, successful, sent);
}

void ASensor::ReceivePacket(uint8* pkt) {
	if (pkt[0] == LED_PKT) {
		//UE_LOG(LogNet, Log, TEXT("LEDPKT"));
		SetLed(pkt[2], pkt[3], pkt[4]);
	}
}

void ASensor::SnapshotState(float timeStamp) {
	//if (!bstateBeenModified) return;
	FSensorState s;
	s = *state;
	s.timeStamp = timeStamp;
	history->timeline.Add(s);
	//UE_LOG(LogNet, Log, TEXT("SENSOR: R%dG%dB%d %f"), state->R, state->G, state->B, s->timeStamp);
	//UE_LOG(LogNet, Log, TEXT("SENSOR: R%dG%dB%d"), s->R, s->G, s->B);
}

void ASensor::RewindState(float requestTime) {
	int i = history->timeline.Num() - 1;
	FSensorState* s = &(history->timeline[i]);
	while (i > 0 && s->timeStamp > requestTime) {
		s = &(history->timeline[--i]);
	}
	state = s;
}


void ASensor::ReplayState(float timeStamp) {
	//int i = 0;
	//FSensorState* s = history->timeline[i];
	//while (i < history->timeline.Num() && s->timeStamp < timeStamp) {
	//	s = history->timeline[++i];
	//}
	//state = s;
	state = GetStatefromTimeline(history, timeStamp);
}

void ASensor::ChangeTimeline(int index) {
	histories.Push(history);
	history = histories[index];
	ResetTimeline();
	ReflectState();
}

FSensorState* ASensor::GetStatefromTimeline(FSensorHistory* h, float timeStamp) {
	FSensorState* s = NULL;
	//UE_LOG(LogNet, Log, TEXT("GSFT: (%d)(%d)"), h->index, h->timeline.Num());
	while (h->index < h->timeline.Num() && h->timeline[h->index].timeStamp <= timeStamp) {

		s = &(h->timeline[h->index]);
		//UE_LOG(LogNet, Log, TEXT("GSFT: SENSOR: R%dG%dB%d %f"), s->R, s->G, s->B, s->timeStamp);

		h->currentState = *s;
		h->index++;
	}
	return s ? s : &(h->currentState);
}

FSensorState* ASensor::GetStatefromTimeline(int index, float timeStamp) {
	return GetStatefromTimeline(histories[index], timeStamp);
}

bool ASensor::StateIsEqual(FSensorState* a, FSensorState* b) {
	return (a->R == b->R && a->G == b->G && a->B == a->B);
}

bool ASensor::DiffCurrentState(int stateIndex, float timeStamp) {
	FSensorState* s = GetStatefromTimeline(stateIndex, timeStamp);
	UE_LOG(LogNet, Log, TEXT("GSFT: SENSOR: R%dG%dB%d %f"), state->R, state->G, state->B, state->timeStamp);
	UE_LOG(LogNet, Log, TEXT("GSFT: SENSOR: R%dG%dB%d %f"), s->R, s->G, s->B, s->timeStamp);
	return StateIsEqual(state, s);
}

/*Reflects the stored state on the virtual object*/
void ASensor::ReflectState() {
	SetLed(state->R, state->G, state->B);
	//UE_LOG(LogNet, Log, TEXT("SENSOR: R%dG%dB%d"), state->R, state->G, state->B);
}

void ASensor::ResetTimeline() {
	/* State has already been stored in history */
	delete state;
	history->index = 0;
	state = history->timeline[0];
	history->currentState = state;
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
		mat = ArdanUtilities::LoadObjFromPath<UMaterialInstance>(TEXT("MaterialInstanceConstant'/Game/Materials/SensorStatus_Warning.SensorStatus_Warning'"));
	}
	UE_LOG(LogNet, Log, TEXT("SET MAT"));

	mesh->SetMaterial(0, mat);
}