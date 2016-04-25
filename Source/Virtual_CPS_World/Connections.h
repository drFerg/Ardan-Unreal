// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Sensor.h"
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

private:
	/* Points to all sensors in world */
	TArray<ASensor*> sensors;
	TMap<int, ASensor*> sensorTable;
	/** Underlying socket communication */
	bool bUDPActive = false;
	ISocketSubsystem *sockSubSystem;
	FSocket* socket;
	int port = 5000;
	/** Local address this net driver is associated with */
	FColor colours[12] = {FColor(255, 0, 0),
											 FColor(0, 255, 0),
											 FColor(0, 0, 255),
										 	 FColor(255,255,0),
										  FColor(0,255,255),
											FColor(255,0,255),
											FColor(192,192,192),
											FColor(128,0,0),
											FColor(0,128,0),
											FColor(128,0,128),
											FColor(0,128,128),
											FColor(0,0,128)};


	/* Storage to receive Cooja packets into */
	uint8 data[MAX_PACKET_SIZE];
	int32 bytesRead = 0;

};
