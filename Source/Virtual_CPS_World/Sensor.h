// Author: Fergus Leahy

#pragma once

#include "GameFramework/Actor.h"
#include "Engine/TriggerBox.h"
#include "IPAddress.h"
#include "IPv4SubnetMask.h"

#include "IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Engine/Channel.h"
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
	UFUNCTION()
	void OnEndOverlap(class AActor* OtherActor);
	UPROPERTY(EditAnywhere)
	AActor* SensorActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SensorName)
	FString SensorName;

private:
	TArray<USpotLightComponent*> Leds;
	ATriggerBox* tb;
	ISocketSubsystem* sockSubSystem;
	FSocket* socket;
	int32 port = 5010;


	
};
