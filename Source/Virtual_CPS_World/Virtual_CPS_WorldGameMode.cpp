// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Virtual_CPS_World.h"
#include "Virtual_CPS_WorldGameMode.h"
#include "Virtual_CPS_WorldPlayerController.h"
#include "Virtual_CPS_WorldCharacter.h"

uint8 CPS_sensorID = 0;

AVirtual_CPS_WorldGameMode::AVirtual_CPS_WorldGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = AVirtual_CPS_WorldPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

int AVirtual_CPS_WorldGameMode::getNewSensorID() {
	return CPS_sensorID++;
}
