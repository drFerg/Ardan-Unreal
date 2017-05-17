// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Sign.generated.h"

UCLASS()
class ARDAN_API ASign : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASign();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UFUNCTION(BlueprintNativeEvent)
	void direct_forward();
	UFUNCTION(BlueprintNativeEvent)
	void direct_reverse();
	UFUNCTION(BlueprintNativeEvent)
	void direct_left();
	UFUNCTION(BlueprintNativeEvent)
	void direct_right();
};
