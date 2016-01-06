// Fill out your copyright notice in the Description page of Project Settings.

#include "Virtual_CPS_World.h"
#include "Connections.h"
#include "Engine/Channel.h"
#include "IPAddress.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "EngineUtils.h"
#include "UnrealString.h"
#include "Sensor.h"
#include "Network.h"
#include "DrawDebugHelpers.h"

/* Points to all sensors in world */
TArray<ASensor*> sensors;

/** Underlying socket communication */
bool bUDPActive = false;
ISocketSubsystem *sockSubSystem;
FSocket* socket;
int port = 5000;
/** Local address this net driver is associated with */
FColor colours[3] = {FColor(255, 0, 0),FColor(0, 255, 0),FColor(0, 0, 255)};


/* Storage to receive Cooja packets into */
uint8 data[MAX_PACKET_SIZE];
int32 bytesRead = 0;

AConnections::AConnections()
{
	/* Create physical node in world */
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	PrimaryActorTick.bCanEverTick = true; /* Tick for each frame */
}

void AConnections::BeginPlay()
{
	Super::BeginPlay();
	/* Clear all state of environment here.
	 * Constructor only run once per life, BeginPlay is the real initialiser */
	sensors.Empty();

	/* Initialise networking */
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = Network::createSocket(sockSubSystem, port, true);
	if (socket != NULL) bUDPActive = true;

	UE_LOG(LogNet, Log, TEXT("UDPActive: %s"),
			bUDPActive ? TEXT("Active"): TEXT("Not Active"));

	/* Find all sensors in the world to handle connections to Cooja */
 	for (TActorIterator<ASensor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		if (ActorItr->GetClass() == ASensor::StaticClass()) {
			UE_LOG(LogNet, Log, TEXT("Name: %s"), *(ActorItr->GetName()));
			UE_LOG(LogNet, Log, TEXT("Class: %s"),
					*(ActorItr->GetClass()->GetDesc()));
			UE_LOG(LogNet, Log, TEXT("Location: %s"),
					*(ActorItr->GetActorLocation().ToString()));
			sensors.Add(ActorItr.operator *());
		}
	}
 	UE_LOG(LogNet, Log, TEXT("Found %i sensors in world!"), sensors.Num());
}



/* Called every frame */
void AConnections::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bUDPActive == false) return;
	TSharedRef<FInternetAddr> fromAddr = sockSubSystem->CreateInternetAddr();
	/* Get data, if any. */
 	while (socket->RecvFrom(data, MAX_PACKET_SIZE, bytesRead, *fromAddr)){
 		//UE_LOG(LogNet, Log, TEXT("RCV: node %i"), data[0]);
 		if (data[0] == LED_PKT && data[1] < sensors.Num()) {
 			UE_LOG(LogNet, Log, TEXT("LEDPKT"));
 			sensors[data[1]]->SetLed(data[2], data[3], data[4]);
 		}
 		else if (data[0] == RADIO_PKT) {
 			UE_LOG(LogNet, Log, TEXT("RMPKT %d (%d)"), data[1], data[2]);
 			/* Display radio event */
 			DrawDebugCircle(GetWorld(), sensors[data[1]]->GetSensorLocation(), 50.0, 360, colours[2], false, 0.3, 0, 5, FVector(0.f,1.f,0.f), FVector(0.f,0.f,1.f), false);
 			DrawDebugCircle(GetWorld(), sensors[data[1]]->GetSensorLocation(), 35.0, 360, colours[2], false, 0.3, 0, 5, FVector(0.f,1.f,0.f), FVector(0.f,0.f,1.f), false);
 			for (int i = 0; i < data[2]; i++){
 				DrawDebugDirectionalArrow(
 				 				GetWorld(),
 				 				sensors[data[1]]->GetSensorLocation(),
 								sensors[data[3 + i]]->GetSensorLocation(),
 								20.0,
 				 				colours[i % 3],
 				 				false, 0.3, 0,
 				 				5.0
 				 			);
 			}

 		}
 	}
}
