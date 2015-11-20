// Author: Fergus Leahy

#include "Virtual_CPS_World.h"
#include "Engine/TriggerBox.h"
#include "Sensor.h"

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

		return TheWorld->SpawnActor<VictoryObjType>(TheBP, Loc ,Rot, SpawnInfo );
	}
TSubclassOf<UBlueprint> MyItemBlueprint;
TArray<USpotLightComponent*> Leds;
ATriggerBox* tb;

void ASensor::OnBeginOverlap(class AActor* OtherActor) {
	UE_LOG(LogNet, Log, TEXT("%s detected someone"), *(this->GetName()));
}

ASensor::ASensor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	if (UObject::IsTemplate(RF_Transient)){
		return;
	}
	UObject* something = StaticLoadObject(UObject::StaticClass(), NULL, TEXT("Blueprint'/Game/SensorNode.SensorNode'"));
	UBlueprint* bp = Cast<UBlueprint>(something);
	TSubclassOf<class UObject> MyItemBlueprint;
	MyItemBlueprint = (UClass*)bp->GeneratedClass;
	if (MyItemBlueprint != NULL){
		UE_LOG(LogNet, Log, TEXT("Name: %s %s"), *(MyItemBlueprint->GetClass()->GetName()), *(this->GetActorLocation().ToString()));
	} else {
		UE_LOG(LogNet, Log, TEXT("I got nothing"));
	}

	SensorActor = SpawnBP<AActor>(GetWorld(), MyItemBlueprint, this->GetActorLocation(), this->GetActorRotation());
	if(SensorActor) {
		UE_LOG(LogNet, Log, TEXT("SeName: %s"), *(SensorActor->GetName()));
		this->Children.Add(SensorActor);
		SensorActor->SetOwner(this);
		UE_LOG(LogNet, Log, TEXT("New Sensor: %s"), *(SensorActor->GetName()))
		SensorActor->GetComponents(Leds);

		for (USpotLightComponent *l: Leds)
		{
			if (l == NULL) continue;
			UE_LOG(LogNet, Log, TEXT("%s"), *(l->GetName()));
			UE_LOG(LogNet, Log, TEXT("Owner: %s"), *(l->GetOwner()->GetName()));
			l->SetIntensity(LEDOFF);
		}
	}
	else {
		UE_LOG(LogNet, Log, TEXT("SeName:NO"));
	}


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
void ASensor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

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

