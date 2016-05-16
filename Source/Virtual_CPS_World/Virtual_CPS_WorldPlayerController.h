// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "IPAddress.h"
#include "IPv4SubnetMask.h"
#include "IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include <flatbuffers/flatbuffers.h>
#include "RunnableConnection.h"
#include "Sensor.h"

#include "unrealpkts_generated.h"

#include "GameFramework/PlayerController.h"
#include "Virtual_CPS_WorldPlayerController.generated.h"

UCLASS()
class AVirtual_CPS_WorldPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVirtual_CPS_WorldPlayerController();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	int32 port = 5011;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	FString address = TEXT("127.0.0.1");

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	uint32 bSelectItem : 1;
	// Begin PlayerController interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();

	void OnSelectItemPressed();
	void selectItem();
	void ScrollUp();
	void ScrollDown();
	void Pause();
	void NextCamera();
	void update_sensors();
private:
	TArray<ASensor*> sensors;
	TMap<int, ASensor*> sensorTable;
	TSubclassOf<class UObject> sensorBlueprint;
	int currentCam = 0;
	TArray<AActor*> cameras;
	const float SmoothBlendTime = 0.75f;
	float elapsed = 0;
	int tickCount = 0;
	ISocketSubsystem* sockSubSystem;
	FSocket* socket;
	FIPv4Address ip;
	TSharedPtr<FInternetAddr> addr;
	flatbuffers::FlatBufferBuilder fbb;
	int32 RecvSize = 0x8000;
	int32 SendSize = 0x8000;

	FRunnableConnection *conns;
	TQueue<uint8*, EQueueMode::Spsc> packetQ;

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

};
