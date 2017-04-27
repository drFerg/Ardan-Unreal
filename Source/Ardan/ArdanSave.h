// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/SaveGame.h"
#include "ActorManager.h"
#include "ArdanSave.generated.h"

/**
 * 
 */
UCLASS()
class ARDAN_API UArdanSave : public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere, Category = Basic)
	FString PlayerName;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	FHistory currentHistory;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	FHistory currentPawnHistory;
	UArdanSave();
};
