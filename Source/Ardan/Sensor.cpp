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

#include <flatbuffers/flatbuffers.h>
#include "unrealpkts_generated.h"
#include "DrawDebugHelpers.h"


#define LEDON 4500
#define LEDOFF 0

template <typename VictoryObjType>
VictoryObjType* SpawnBP(UWorld* TheWorld, UClass* TheBP, const FVector& Loc,
		const FRotator& Rot, const bool bNoCollisionFail = true,
		AActor* Owner = NULL,	APawn* Instigator = NULL) {

		if(!TheWorld) return NULL;
		if(!TheBP) return NULL;
		//~~

		FActorSpawnParameters SpawnInfo;
		// SpawnInfo.bNoCollisionFail 		= bNoCollisionFail;
		SpawnInfo.Owner 				= Owner;
		SpawnInfo.Instigator			= Instigator;
		SpawnInfo.bDeferConstruction 	= false;

		return TheWorld->SpawnActor<VictoryObjType>(TheBP, Loc ,Rot, SpawnInfo);
	}



TArray<AActor*> inMotionRange;
TMap<AActor*, FVector> inMotionPos; /* Contains actor as key and old location */



void ASensor::OnBeginOverlap(class AActor* OverlappedActor,
														 class AActor* otherActor) {
	/* Start motion tracking the overlapping otherActor */
	inMotionPos.Add(otherActor, otherActor->GetActorLocation());
	//UE_LOG(LogNet, Log, TEXT("%s: Someone entered (%s)"), *(this->GetName()), *(otherActor->GetName()));
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
	inMotionRange.Remove(otherActor); /* Remove otherActor from motion tracking */
	//UE_LOG(LogNet, Log, TEXT("%s: Someone left (%s)"), *(this->GetName()), *(otherActor->GetName()));
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

ASensor::ASensor()
{
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

	/* Find the blueprint of the 3D model */
	UBlueprint* blueprint = Cast<UBlueprint>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("Blueprint'/Game/SensorNode.SensorNode'")));
	TSubclassOf<class UObject> MyItemBlueprint = (UClass*)(blueprint->GeneratedClass);
	if (MyItemBlueprint != NULL) {
		UE_LOG(LogNet, Log, TEXT("BName: %s %s"), *(MyItemBlueprint->GetClass()->GetName()), *(this->GetActorLocation().ToString()));
	} else {
		UE_LOG(LogNet, Log, TEXT("I got nothing"));
	}

	/* Spawn the 3D model for the sensor in the virtual world */
	SensorActor = SpawnBP<AActor>(GetWorld(), MyItemBlueprint, this->GetActorLocation(), this->GetActorRotation());
	if(SensorActor) {
		UE_LOG(LogNet, Log, TEXT("SeName: %s"), *(SensorActor->GetName()));
		this->Children.Add(SensorActor);
		SensorActor->SetOwner(this);
		UE_LOG(LogNet, Log, TEXT("New Sensor: %s"), *(SensorActor->GetName()))
		/* Retrieve all the LEDS and turn them OFF */
		SensorActor->GetComponents(Leds);
		for (USpotLightComponent *l: Leds) {
			if (l == NULL) continue;
			UE_LOG(LogNet, Log, TEXT("%s owned by %s"), *(l->GetName()),
					*(l->GetOwner()->GetName()));
			l->SetIntensity(LEDOFF);
		}
	}
	else {
		UE_LOG(LogNet, Log, TEXT("SeName:Not spawned!"));
	}
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
void ASensor::BeginPlay()
{
	Super::BeginPlay();

	/* Set up destination address:port to send Cooja messages to */
	FIPv4Address::Parse(address, ip);
	addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.Value);
	addr->SetPort(port);

	TArray<AActor*> attachedActors;
	if(SensorActor) {
		SensorActor->GetAttachedActors(attachedActors);
		if (PIRSensor) {
			PIRSensor->OnActorBeginOverlap.AddDynamic(this, &ASensor::OnBeginOverlap);
			PIRSensor->OnActorEndOverlap.AddDynamic(this, &ASensor::OnEndOverlap);
		}
		/* Retrieve all the LEDS and turn them OFF */
		Leds.Empty();
		UE_LOG(LogNet, Log, TEXT("Sensor: %s"), *(SensorActor->GetName()))
		SensorActor->GetComponents(Leds);
		for (USpotLightComponent *l: Leds) {
			if (l == NULL) continue;
			//UE_LOG(LogNet, Log, TEXT("%s owned by %s"), *(l->GetName()),
				//	*(l->GetOwner()->GetName()));
			l->SetIntensity(LEDOFF);
		}
	}
	else {
		UE_LOG(LogNet, Log, TEXT("Node: Not spawned"));
	}
	prevLocation = SensorActor->GetActorLocation();
	sendLocationUpdate();
	UE_LOG(LogNet, Log, TEXT("%d %s"), ID,
				*(SensorActor->GetActorLocation().ToString()));
	sendLocationUpdate();
}

// Called every frame
void ASensor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	/* Check if actors located within PIR detection range have moved,
	 * checking their current location vs previous */

	if (SensorActor && prevLocation.Equals(SensorActor->GetActorLocation(), 10) == false) {
		sendLocationUpdate();
		prevLocation = SensorActor->GetActorLocation();
	}
	activeTimer += DeltaTime;
	if (activeTimer > PIRRepeatTime) {
		active = true;
		activeTimer = 0;
	}
}

void ASensor::Led(int32 led, bool on)
{
	if (led > 3 && led < 0) return;
//UE_LOG(LogNet, Log, TEXT("Node: %s"), *(SensorActor->GetName()))
	//UE_LOG(LogNet, Log, TEXT("Led: %s (%i)"), *(Leds[led]->GetName()), led);
	Leds[led]->SetIntensity(on ? LEDON : LEDOFF);
}

void ASensor::SetLed(uint8 R, uint8 G, uint8 B)
{
	//UE_LOG(LogNet, Log, TEXT("Node: %s"), *(actor->GetName()))
	Leds[0]->SetIntensity(R ? LEDON : LEDOFF);
	Leds[1]->SetIntensity(G ? LEDON : LEDOFF);
	Leds[2]->SetIntensity(B ? LEDON : LEDOFF);
	if (Light1) Light1->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
	if (Light2) Light2->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
	if (Light3) Light3->GetLightComponent()->SetIntensity(R ? LEDON: LEDOFF);
}

void ASensor::SetSelected() {
	DrawDebugCircle(GetWorld(), SensorActor->GetActorLocation(), 150.0, 360, FColor(0, 255, 0), false, 0.3, 0, 5, FVector(0.f,1.f,0.f), FVector(0.f,0.f,1.f), false);
}

FVector ASensor::GetSensorLocation() {
	return SensorActor->GetActorLocation();
}

void ASensor::sendLocationUpdate() {
	FVector locVec = SensorActor->GetActorLocation();

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
//		for (AActor *a: attachedActors) {
//			UE_LOG(LogNet, Log, TEXT("%s:%s"), *(a->GetName()), *(a->GetClass()->GetName()));
//			/* Find a trigger volume aka PIR Sensor */
//			if (a->GetClass() == AStaticMeshActor::StaticClass()){
//				UE_LOG(LogNet, Log, TEXT("Found a Cone Sensor: %s"), *(a->GetName()));
//				tb = (ATriggerBase*) a;
//				tb->OnActorBeginOverlap.AddDynamic(this, &ASensor::OnBeginOverlap);
//				tb->OnActorEndOverlap.AddDynamic(this, &ASensor::OnEndOverlap);
//			}
//			if (a->GetClass()->IsChildOf(ATriggerBase::StaticClass())){
//				UE_LOG(LogNet, Log, TEXT("Found a PIR Sensor: %s"), *(a->GetName()));
//				tb = (ATriggerBase*) a;
//				tb->OnActorBeginOverlap.AddDynamic(this, &ASensor::OnBeginOverlap);
//				tb->OnActorEndOverlap.AddDynamic(this, &ASensor::OnEndOverlap);
//			}

		//}
