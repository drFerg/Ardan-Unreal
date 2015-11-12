// Fill out your copyright notice in the Description page of Project Settings.

#include "Virtual_CPS_World.h"
#include "Connections.h"
#include "Engine/Channel.h"
#include "IPAddress.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "EngineUtils.h"
#include "UnrealString.h"
#include <vector>
#include "Node.h"

ISocketSubsystem *SocketSubsystem;
/** Local address this net driver is associated with */
TSharedPtr<FInternetAddr> LocalAddr;
TArray<AActor*>* MyActor_Children;
TArray<UActorComponent*> comp;

TArray<AActor*> sensors;
std::vector<Node*> nodes;

/** Underlying socket communication */
FSocket* Socket;
uint32 MaxPortCountToTry = 10;

bool bUDPActive = false;


// Sets default values for this component's properties
AConnections::AConnections()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	PrimaryActorTick.bCanEverTick = true;
 	//MyActor_Children = this->Children;
	MyActor_Children = &(this->Children);
 	UE_LOG(LogNet, Log, TEXT("Got %i children") , MyActor_Children->Num());
 	this->GetComponents(comp);

 	UE_LOG(LogNet, Log, TEXT("Got %i comps") , RootComponent->GetNumChildrenComponents());

 	//SensorNodeHardware = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NODE"));
	//SensorNodeHardware->AttachTo(RootComponent);
}


bool createSocket(bool bReuseAddressAndPort) 
{
	SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (SocketSubsystem == NULL)
	{
		UE_LOG(LogNet, Warning, TEXT("SensorTCP::CreateSocket: Unable to find socket subsystem"));
		return false;
	}

	Socket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDPCONN"), true);
	if( Socket == NULL )
	{
		Socket = 0;
		UE_LOG(LogNet, Warning, TEXT("SOCK: socket failed (%i)"), (int32)SocketSubsystem->GetLastErrorCode() );
		return false;
	}

	if (Socket->SetReuseAddr(bReuseAddressAndPort) == false)
	{
		UE_LOG(LogNet, Warning, TEXT("setsockopt with SO_REUSEADDR failed"));
	}

	if (Socket->SetRecvErr() == false)
	{
		UE_LOG(LogNet, Warning, TEXT("setsockopt with IP_RECVERR failed"));
	}

	int32 RecvSize = 0x8000;
	int32 SendSize = 0x8000;
	Socket->SetReceiveBufferSize(RecvSize,RecvSize);
	Socket->SetSendBufferSize(SendSize,SendSize);
	UE_LOG(LogInit, Log, TEXT("%s: Socket queue %i / %i"), SocketSubsystem->GetSocketAPIName(), RecvSize, SendSize );

	// Bind socket to our port.
	LocalAddr = SocketSubsystem->GetLocalBindAddr(*GLog);
	
	LocalAddr->SetPort(5000);
	
	int32 AttemptPort = LocalAddr->GetPort();
	int32 BoundPort = SocketSubsystem->BindNextPort( Socket, *LocalAddr, MaxPortCountToTry + 1, 1);
	if (BoundPort == 0)
	{
		UE_LOG(LogNet, Log, TEXT("%s: binding to port %i failed (%i)"), SocketSubsystem->GetSocketAPIName(), AttemptPort,
			(int32)SocketSubsystem->GetLastErrorCode());
		return false;
	}
	if(Socket->SetNonBlocking() == false)
	{
		UE_LOG(LogNet, Log, TEXT("%s: SetNonBlocking failed (%i)"), SocketSubsystem->GetSocketAPIName(),
			(int32)SocketSubsystem->GetLastErrorCode());
		return false;
	}
	return true;
}
// Called when the game starts
void AConnections::BeginPlay()
{
	Super::BeginPlay();
	/* Clear all state of environment here.
	 * Constructor only run once per life, BeginPlay is the real initialiser */
	sensors.Empty();
	nodes.clear();

	// FString address = TEXT("127.0.0.1");
	// int32 port = 19834;
	// FIPv4Address ip;
	// FIPv4Address::Parse(address, ip);


	// TSharedRef addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	// addr->SetIp(ip.GetValue());
	// addr->SetPort(port);
	bUDPActive = createSocket(true);
	UE_LOG(LogNet, Log, TEXT("UDPActive: %s"), bUDPActive ? TEXT("Active"): TEXT("Not Active"));
 	for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		if (ActorItr->GetClass()->GetDesc().Equals(TEXT("SensorNode_C"), ESearchCase::CaseSensitive) && !ActorItr->GetClass()->GetDesc().Equals("None")) {
			UE_LOG(LogNet, Log, TEXT("Name: %s"), *(ActorItr->GetName()));
			UE_LOG(LogNet, Log, TEXT("Class: %s"), *(ActorItr->GetClass()->GetDesc()));
			UE_LOG(LogNet, Log, TEXT("Location: %s"), *(ActorItr->GetActorLocation().ToString()));
			AActor *a = ActorItr.operator *();
			sensors.Add(a);
		}
	}
 	UE_LOG(LogNet, Log, TEXT("Sensors: %i"), sensors.Num());
 	for (int32 i = 0; i < sensors.Num(); i++)
	{
		UE_LOG(LogNet, Log, TEXT("%s"), *(sensors[i]->GetName()));
		nodes.push_back(new Node(sensors[i]));
	}
 	UE_LOG(LogNet, Log, TEXT("Sensors/Nodes: %i/%i"), sensors.Num(), nodes.size());
	for (Node* n: nodes)
	{
		UE_LOG(LogNet, Log, TEXT("%s"), *(n->actor->GetName()));
	}
}


// Called every frame
void AConnections::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bUDPActive == false) return;
	uint8 Data[MAX_PACKET_SIZE];
	int32 BytesRead = 0;

 	TSharedRef<FInternetAddr> FromAddr = SocketSubsystem->CreateInternetAddr();

	/* Get data, if any. */
 	while (Socket->RecvFrom(Data, sizeof(Data), BytesRead, *FromAddr)){
// 		UE_LOG(LogNet, Log, TEXT("CONNECTIONS: Received UDP message from %s (%i bytes)"),
// 				*FromAddr->ToString(true),
// 				BytesRead);
// 			UE_LOG(LogNet, Log, TEXT("Node %i: %i %i"), Data[0], Data[1], Data[2]);
 		nodes.at(Data[0])->SetLed(Data[1], Data[2], Data[3]);
 	}



}

