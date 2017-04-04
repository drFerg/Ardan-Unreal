// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "IPAddress.h"
#include "IPv4SubnetMask.h"
#include "IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include <flatbuffers/flatbuffers.h>
#include "RunnableConnection.h"
#include "Sensor.h"
#include "TimeSphere.h"

#include "unrealpkts_generated.h"

#include "GameFramework/PlayerController.h"
#include "ArdanGhostController.generated.h"

UCLASS()
class AArdanGhostController : public APlayerController
{
	GENERATED_BODY()

public:
	AArdanGhostController();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	int32 port = 5011;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	FString address = TEXT("127.0.0.1");

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	uint32 current_step = 1;
	// Begin PlayerController interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	private:
		TArray<FTransform*> locHistory;
		TArray<ATimeSphere*> timeSpheres;



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
