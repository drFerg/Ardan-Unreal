// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/PlayerController.h"
#include "Virtual_CPS_WorldPlayerController.generated.h"

UCLASS()
class AVirtual_CPS_WorldPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVirtual_CPS_WorldPlayerController();

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	uint32 bSelectItem : 1;
	// Begin PlayerController interface
	virtual void BeginPlay() override;
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
private:
	TSubclassOf<class UObject> sensorBlueprint;
	int currentCam = 0;
	TArray<AActor*> cameras;
	const float SmoothBlendTime = 0.75f;

};


