// Author: Fergus Leahy

#include "Virtual_CPS_World.h"
#include "Sensor.h"

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
// Sets default values
ASensor::ASensor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FObjectFinder<UBlueprint> ItemBlueprint(TEXT("Blueprint'/Game/SensorNode.SensorNode'"));
    if (ItemBlueprint.Object){
        MyItemBlueprint = (UClass*)ItemBlueprint.Object->GeneratedClass;
        UE_LOG(LogNet, Log, TEXT("YAY"));
    }
	//AActor* NewActor = SpawnBP<AActor>(GetWorld(), MyItemBlueprint, this->GetActorLocation(), this->GetActorRotation());
	//UE_LOG(LogNet, Log, TEXT("New Sensor: %s"), *(NewActor->GetName()))
//	actor = sensor;
//	actor->GetComponents(Leds);
//
//	for (USpotLightComponent *l: Leds)
//	{
//		if (l == NULL) continue;
//		UE_LOG(LogNet, Log, TEXT("%s"), *(l->GetName()));
//		UE_LOG(LogNet, Log, TEXT("Owner: %s"), *(l->GetOwner()->GetName()));
//		l->SetIntensity(LEDOFF);
//	}
//	TArray<AActor*> attachedActors;
//	//tb = actor->GetComponentByClass(ATriggerBox);
//	actor->GetAttachedActors(attachedActors);
//	for (AActor *a: attachedActors) {
//		UE_LOG(LogNet, Log, TEXT("%s"), *(a->GetName()));
//		if (a->GetClass() == ATriggerBox){
//			UE_LOG(LogNet, Log, TEXT("Found: %s"), *(a->GetName()));
//			tb = a;
//			tb->on
//			tb->OnActorBeginOverlap.AddDynamic(sensor, Node::OnBeginTriggerOverlap);
//		}
//	}
}

// Called when the game starts or when spawned
void ASensor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASensor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

