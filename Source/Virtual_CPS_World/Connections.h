// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Connections.generated.h"

#define LED_PKT 0
#define RADIO_PKT 1

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VIRTUAL_CPS_WORLD_API AConnections : public AActor
{

	GENERATED_BODY()

public:	

	/** Number of ports which will be tried if current one is not available for binding (i.e. if told to bind to port N, will try from N to N+MaxPortCountToTry inclusive) */
	//UPROPERTY(Config)
	

	// Sets default values for this component's properties
	AConnections();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	//UPROPERTY(EditAnywhere)
	//USceneComponent* SensorNodeHardware;
	
};
	
