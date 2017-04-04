// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Ardan.h"
#include "ArdanGhostController.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "ArdanCharacter.h"


AArdanGhostController::AArdanGhostController()
{

}

void AArdanGhostController::BeginPlay() {
  APawn* const Pawn = GetPawn();
  Pawn->SetActorTransform(*locHistory[current_step++]);
}

void AArdanGhostController::EndPlay(const EEndPlayReason::Type EndPlayReason) {

}

void AArdanGhostController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	APawn* const Pawn = GetPawn();
  // FVector sourceLoc = Pawn->GetActorLocation();
	// FRotator sourceRot = Pawn->GetActorRotation();
  // FTransform *transform = new FTransform();
  // *transform = Pawn->GetTransform();
	/* Pop and set actors old location else push current location onto stack */
	// if (bReverse && locHistory.Num() > 0) {
	Pawn->SetActorTransform(*locHistory[current_step++]);
	// }
	// else {
		// locHistory.Push(transform);
	// }


}
// Called to bind functionality to input
void AArdanGhostController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();
}
