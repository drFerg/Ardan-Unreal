// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Ardan.h"
#include "ArdanGameMode.h"
#include "ArdanPlayerController.h"
#include "ArdanCharacter.h"
#include "DejaVuStateBase.h"

AArdanGameMode::AArdanGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AArdanPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<AGameStateBase> GameStateBPClass (TEXT("/Game/DejaVu/Blueprints/DejaVuState"));
	if (GameStateBPClass.Class != NULL)
	{
		GameStateClass = GameStateBPClass.Class;
	}
	//GameStateClass = ADejaVuStateBase::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}