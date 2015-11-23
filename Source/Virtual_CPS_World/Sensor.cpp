// Author: Fergus Leahy

#include "Virtual_CPS_World.h"
#include "Engine/TriggerBox.h"
#include "Sensor.h"
#include "IPAddress.h"
#include "IPv4SubnetMask.h"

#include "IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Engine/Channel.h"

#define LEDON 10000
#define LEDOFF 0

template <typename VictoryObjType>
VictoryObjType* SpawnBP(
		UWorld* TheWorld,
		UClass* TheBP,
		const FVector& Loc,
		const FRotator& Rot,
		const bool bNoCollisionFail = true,
		AActor* Owner = NULL,
		APawn* Instigator = NULL
	){
		if(!TheWorld) return NULL;
		if(!TheBP) return NULL;
		//~~

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.bNoCollisionFail 		= bNoCollisionFail;
		SpawnInfo.Owner 				= Owner;
		SpawnInfo.Instigator				= Instigator;
		SpawnInfo.bDeferConstruction 	= false;

		return TheWorld->SpawnActor<VictoryObjType>(TheBP, Loc ,Rot, SpawnInfo);
	}

TSubclassOf<UBlueprint> MyItemBlueprint;
FIPv4Address ip;
TSharedPtr<FInternetAddr> addr;
FString address = TEXT("127.0.0.1");
int32 port = 5010;

void ASensor::OnBeginOverlap(class AActor* OtherActor) {
	UE_LOG(LogNet, Log, TEXT("%s detected someone"), *(this->GetName()));
	int sent = 0;
	int size = 3;
	uint8 data[3] = {0, 0, 1};
	bool successful = socket->SendTo(data, size, sent, *addr);
	UE_LOG(LogNet, Log, TEXT("Send to %s: %i-%i"), *(addr->ToString(true)), successful, sent);
}

void ASensor::OnEndOverlap(class AActor* OtherActor) {
	UE_LOG(LogNet, Log, TEXT("%s detected someone"), *(this->GetName()));
	int sent = 0;
	int size = 3;
	uint8 data[3] = {0, 0, 0};
	bool successful = socket->SendTo(data, size, sent, *addr);
	UE_LOG(LogNet, Log, TEXT("Send to %s: %i-%i"), *(addr->ToString(true)), successful, sent);
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
		UE_LOG(LogNet, Log, TEXT("SeName:Not spawned"));
	}

	/* Networking setup */
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = sockSubSystem->CreateSocket(NAME_DGram, TEXT("UDPCONN2"), true);
	if (socket) UE_LOG(LogNet, Log, TEXT("Created Socket"));
	int32 RecvSize = 0x8000;
	int32 SendSize = 0x8000;
	socket->SetReceiveBufferSize(RecvSize, RecvSize);
	socket->SetSendBufferSize(SendSize, SendSize);

	/* Set up address:port to send Cooja messages to */
	FIPv4Address::Parse(address, ip);
	addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.GetValue());
	addr->SetPort(port);
}


// Called when the game starts or when spawned
void ASensor::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> attachedActors;
	//tb = SensorActor->GetComponentByClass(ATriggerBox);
	SensorActor->GetAttachedActors(attachedActors);
	for (AActor *a: attachedActors) {
		UE_LOG(LogNet, Log, TEXT("%s"), *(a->GetName()));
		if (a->GetClass() == ATriggerBox::StaticClass()){
			UE_LOG(LogNet, Log, TEXT("Found: %s"), *(a->GetName()));
			tb = (ATriggerBox*) a;
			tb->OnActorBeginOverlap.AddDynamic(this, &ASensor::OnBeginOverlap);
		}
	}
}

// Called every frame
void ASensor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASensor::Led(int32 led, bool on)
{
	if (led > 3 && led < 0) return;
	UE_LOG(LogNet, Log, TEXT("Node: %s"), *(SensorActor->GetName()))
	UE_LOG(LogNet, Log, TEXT("Led: %s (%i)"), *(Leds[led]->GetName()), led);
	Leds[led]->SetIntensity(on ? LEDON : LEDOFF);
}

void ASensor::SetLed(uint8 R, uint8 G, uint8 B)
{
	//UE_LOG(LogNet, Log, TEXT("Node: %s"), *(actor->GetName()))
	Leds[0]->SetIntensity(R ? LEDON : LEDOFF);
	Leds[1]->SetIntensity(G ? LEDON : LEDOFF);
	Leds[2]->SetIntensity(B ? LEDON : LEDOFF);
}

