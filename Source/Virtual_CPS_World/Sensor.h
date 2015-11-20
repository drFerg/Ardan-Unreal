// Author: Fergus Leahy

#pragma once

#include "GameFramework/Actor.h"
#include "Sensor.generated.h"

UCLASS()
class VIRTUAL_CPS_WORLD_API ASensor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASensor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick(float DeltaSeconds) override;
	void Led(int32 led, bool on);
	void SetLed(uint8 R, uint8 G, uint8 B);
	UFUNCTION()
	void OnBeginOverlap(class AActor* OtherActor);

	UPROPERTY(EditAnywhere)
	AActor* SensorActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SensorName)
	FString SensorName;


	
};
