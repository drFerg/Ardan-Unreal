// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
class VIRTUAL_CPS_WORLD_API Node
{
public:
	AActor* actor;

	Node(AActor* sensor);
	~Node();

	void Led(int32 led, bool on);
	void SetLed(uint8 R, uint8 G, uint8 B);
private:
	TArray<USpotLightComponent*> Leds;

};
