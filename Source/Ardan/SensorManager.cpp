// Fill out your copyright notice in the Description page of Project Settings.

#include "Ardan.h"
#include "SensorManager.h"
#include "UObjectGlobals.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "RunnableConnection.h"

SensorManager::SensorManager(UWorld* w) {
	world = w;
}

SensorManager::~SensorManager()
{
}

void SensorManager::AddSensor(ASensor *sensor) {
	sensors.Add(sensor);
	sensorTable.Add(sensor->ID, sensor);
}

void SensorManager::FindSensors() {
	/* Find all sensors in the world to handle connections to Cooja */
	for (TActorIterator<ASensor> ActorItr(world); ActorItr; ++ActorItr) {
			AddSensor(ActorItr.operator *());
	}
}

void SensorManager::ReceivePacket(uint8* pkt) {
	ASensor* sensor = *sensorTable.Find(pkt[1]);
	if (sensor == NULL) return;
	sensor->ReceivePacket(pkt);
	if (pkt[0] == RADIO_PKT) {
		/* Display radio event */
		FVector sourceLoc = sensor->GetSensorLocation();
		DrawDebugCircle(world, sourceLoc, 50.0, 360, colours[pkt[1] % 12], false, 0.5, 0, 5, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		for (int i = 0; i < pkt[2]; i++) {
			ASensor* sensor2 = *sensorTable.Find(pkt[3 + i]);
			if (sensor2 == NULL) continue;
			DrawDebugDirectionalArrow(world, sourceLoc, sensor2->GetSensorLocation(),
				500.0, colours[pkt[1] % 12], false, 0.5, 0, 5.0);
		}
	}
}

void SensorManager::SnapshotState(float timeStamp) {
	for (ASensor* sensor : sensors) {
		sensor->SnapshotState(timeStamp);
	}
}

void SensorManager::RewindState(float timeStamp) {
	for (ASensor* sensor : sensors) {
		sensor->RewindState(timeStamp);
	}
}