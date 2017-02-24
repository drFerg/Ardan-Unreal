// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
class ARDAN_API Node
{
public:
	AActor* actor;

	Node(AActor* sensor);
	~Node();

	void Led(int32 led, bool on);
	void SetLed(uint8 R, uint8 G, uint8 B);
	void OnBeginTriggerOverlap(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
private:
	TArray<USpotLightComponent*> Leds;

};
