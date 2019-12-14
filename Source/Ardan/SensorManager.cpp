// Fill out your copyright notice in the Description page of Project Settings.

#include "Ardan.h"
#include "SensorManager.h"
#include "UObjectGlobals.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetStringLibrary.h"
#include "RunnableConnection.h"
#include <vector>
#include "flatbuffers/c/unrealpkts_generated.h"

using namespace UnrealCoojaMsg; 

SensorManager::SensorManager(UWorld* w, AActor *con) {
	world = w;
	controller = con;
}

SensorManager::~SensorManager()
{
}

void SensorManager::AddSensor(ASensor *sensor) {
	sensors.Add(sensor);
	sensorTable.Add(sensor->ID, sensor);
	sensor->AddTickPrerequisiteActor(controller);
}

void SensorManager::FindSensors() {
	/* Find all sensors in the world to handle connections to Cooja */
	for (TActorIterator<ASensor> ActorItr(world); ActorItr; ++ActorItr) {
			AddSensor(ActorItr.operator *());
			
	}
}

void SensorManager::ReceivePacket(struct pkt* pkt) {
	auto v = flatbuffers::Verifier(pkt->data, pkt->size);
	bool ok = VerifyMessageBuffer(v);
	if (!ok) {
		UE_LOG(LogNet, Log, TEXT("Bad buffer %d"), pkt->size);
		return;
	}
	//UE_LOG(LogNet, Log, TEXT("Good buffer %d"), pkt->size);

	auto msg = GetMessage(pkt->data);
	//UE_LOG(LogNet, Log, TEXT("Good buffer type %d"), msg->type());

	if (msg->type() == MsgType_TSTAMP) {
		if (!hasStartTime) {
			hasStartTime = true;
			startTime = msg->tstamp();
		}
		UE_LOG(LogNet, Log, TEXT("SYNC, %lu, %s"), msg->tstamp() - startTime, *(UKismetStringLibrary::TimeSecondsToString(this->world->GetTimeSeconds())));
	}
	/* Display radio event */
	else if (msg->type() == MsgType_RADIO) {
		//UE_LOG(LogNet, Log, TEXT("Got a radio message"));

		ASensor **s = sensorTable.Find(msg->id());
		if (s == NULL) return;
		ASensor* sensor = *s;
		if (sensor == NULL) return;
		FVector sourceLoc = sensor->GetSensorLocation();
		DrawDebugCircle(world, sourceLoc, 70.0, 360, colours[msg->id() % 12], false, 0.5, 0, 5, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
		DrawDebugCircle(world, sourceLoc, 60.0, 360, colours[msg->id() % 12], false, 0.5, 0, 5, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

		auto rcvd = msg->rcvd();
		for (size_t i = 0; i < rcvd->Length(); i++) {
			s = sensorTable.Find(rcvd->Get(i));
			if (s == NULL) continue;
			ASensor* sensor2 = *s;
			if (sensor2 == NULL) continue;
			DrawDebugDirectionalArrow(world, sourceLoc, sensor2->GetSensorLocation(),
				500.0, colours[msg->id() % 12], false, 0.5, 0, 5.0);
		}
	}
	else if (msg->type() == MsgType_TARGET) {
		UE_LOG(LogNet, Log, TEXT("target message type, %d %d"), msg->id(), msg->target());
		ASensor **s = sensorTable.Find(msg->id());
		if (s == NULL) return;
		ASensor* sensor = *s;
		UE_LOG(LogNet, Log, TEXT("target message found sensor"));

		ASensor **next = sensorTable.Find(msg->target());
		if (next == NULL) return;
		UE_LOG(LogNet, Log, TEXT("target message found next target"));

		sensor->nextSensor = *next;
	}
	else {
		//UE_LOG(LogNet, Log, TEXT("Other message type"));
		ASensor **s = sensorTable.Find(msg->id());
		if (s == NULL) return;
		ASensor* sensor = *s;
		if (sensor == NULL) return;
		sensor->ReceivePacket(msg);
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

void SensorManager::CopyOutState(TMap<FString, FSensorHistory>* sensorHistory) {
	for (ASensor* sensor : sensors) {
		sensorHistory->Add(sensor->GetName(), *sensor->history);
	}
}

void SensorManager::CopyInState(TMap<FString, FSensorHistory>* sensorHistory) {
	for (ASensor* sensor : sensors) {
		FSensorHistory* h = sensorHistory->Find(sensor->GetName());
		if (!h) continue;
		sensor->history = h;
		sensor->ResetTimeline();
		sensor->ReflectState();
	}
}

void SensorManager::SetProducer(rd_kafka_t * rk, rd_kafka_topic_t * rkt)
{
	for (ASensor* sensor : sensors) {
		sensor->SetProducer(rk, rkt);
	}
}
