// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TimeSphere.h"
#include "GameFramework/Character.h"
#include "GhostCharacter.generated.h"

UCLASS()
class ARDAN_API AGhostCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGhostCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TArray<FTransform*> locHistory;
	TArray<ATimeSphere*> timeSpheres;
	uint32 current_step = 1;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;



};
