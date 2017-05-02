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

void SensorManager::Replay() {
	for (ASensor* sensor : sensors) {
		sensor->ResetTimeline();
		sensor->ReflectState();
		sensor->SetReplayMode(true);
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
		sensor->ReflectState();
	}
}

void SensorManager::FastForwardState(float timeStamp) {
	for (ASensor* sensor : sensors) {
		sensor->ReplayState(timeStamp);
		sensor->ReflectState();
	}
}

void SensorManager::NewTimeline() {
	bHasHistory = true;
	for (ASensor* sensor : sensors) {
		sensor->ResetTimeline();
		sensor->NewTimeline();
		sensor->SnapshotState(0.0);
		sensor->ReflectState();
		sensor->SetReplayMode(false);
	}
}

void SensorManager::ResetTimeline() {
	for (ASensor* sensor : sensors) {
		sensor->ResetTimeline();
		sensor->ReflectState();
	}
}


void SensorManager::ChangeTimeline(int index) {
	for (ASensor* sensor : sensors) {
		sensor->ChangeTimeline(index);
		sensor->ReflectState();
	}
}

bool SensorManager::DiffState(int index, float timeStamp) {
	if (!bHasHistory) return false;
	bool allChanged = false;
	bool equal = false;
	for (ASensor* sensor : sensors) {
		equal = sensor->DiffCurrentState(index, timeStamp);
		if (!equal) sensor->ColourSensor(1);
		else sensor->ColourSensor(0);
	}
	return equal;
}