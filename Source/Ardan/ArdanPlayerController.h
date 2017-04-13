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
#include "ArdanPlayerController.generated.h"

USTRUCT()
struct FObjectMeta {
	GENERATED_BODY()
	FVector velocity;
	FVector angularVelocity;
	FTransform transform;
	float deltaTime;
	float timeStamp;
};

USTRUCT()
struct FObjectInfo {
	GENERATED_BODY()
	AActor *actor;
	FObjectInfo *ancestor;
	FObjectInfo *descendant;
	TArray<FObjectMeta*> *hist;
	int index;
	bool bisGhost = false;
};

USTRUCT()
struct FHistory {
	GENERATED_BODY()
	TMap<FString, FObjectInfo*> histMap;
	bool bfinished = false;
	int level = 1;
};

UCLASS()
class AArdanPlayerController : public APlayerController {
	GENERATED_BODY()

public:
	AArdanPlayerController();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	int32 port = 5011;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	FString address = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Time)
	TArray<AStaticMeshActor*> ReplayActors;

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	// Begin PlayerController interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();

	void ScrollUp();
	void ScrollDown();
	void Pause();
	void NextCamera();
	void update_sensors();
	void reversePressed();
	void reverseReleased();
	void speedSlow();
	void speedNormal();
	void speedFast();
	void recordActors(float deltaTime);
	void recordPawnActors(float deltaTime);
	void initHist();
	void copyActors(FHistory* dstHistory, FHistory *srcHistory);
	void copyPawnActors(FHistory * dstHistory, FHistory * srcHistory);
	void diff(FObjectInfo * info);
	void rewindMeshActors(FHistory *history, bool freeze);
	void rewindPawnActors(FHistory *history);
	void replayActors(FHistory *history);
	void replayPawnActors(FHistory * history);
	void resetActors(FHistory *history);
	void resetPawnActors(FHistory * history);
	void replayPressed();

	void ghostActor(AActor * mesh, float amount);

	void colourActor(AActor * mesh);


	private:
		TArray<FHistory*> pawnHistories;
		TArray<FHistory*> histories;
		UObject* NewSubObject;
		TArray<FTransform*> locHistory;
		TArray<ATimeSphere*> timeSpheres;
		TArray<void *> timelines;
		FHistory *currentHistory;
		FHistory *currentPawnHistory;
		int index = 0;
		float replayTime = 0;
		float curTime = 0;
		bool bReverse = false;
		bool bReplay = false;
		TArray<ASensor*> sensors;
		TMap<int, ASensor*> sensorTable;
		TSubclassOf<class UObject> sensorBlueprint;
		int currentCam = 0;
		TArray<AActor*> cameras;
		const float SmoothBlendTime = 0.75f;

		bool slow = false;
		float elapsed = 0;
		float tenth = 0;
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
