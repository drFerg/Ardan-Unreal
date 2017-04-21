// Author: Fergus Leahy

#pragma once

#include "GameFramework/Actor.h"
#include "Engine/TriggerBox.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Light.h"

#include "IPAddress.h"
#include "IPv4SubnetMask.h"
#include "IPv4Address.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Engine/Channel.h"

#include <flatbuffers/flatbuffers.h>
#include "unrealpkts_generated.h"

#include "Sensor.generated.h"

USTRUCT()
struct FSensorState {
	GENERATED_BODY()
	unsigned int R : 1;
	unsigned int G : 1;
	unsigned int B : 1;
	float timeStamp;
};

USTRUCT()
struct FSensorHistory {
	GENERATED_BODY()
	TArray<FSensorState*> timeline;
	int index = 0;
};

UCLASS()
class ARDAN_API ASensor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASensor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;
	void ReceivePacket(uint8 * pkt);

	void SnapshotState(float timeStamp);

	void RewindState(float timeStamp);

	void ReplayState(float timeStamp);

	void ChangeTimeline(int index);

	FSensorState * GetStatefromTimeline(int index, float timeStamp);

	bool StateIsEqual(FSensorState* a, FSensorState* b);

	void ReflectState();

	void ResetTimeline();

	void NewTimeline();
	void ColourSensor(int type);
	bool DiffCurrentState(int stateIndex, float timeStamp);

	void Led(int32 led, bool on);
	void SetLed(uint8 R, uint8 G, uint8 B);
	FVector GetSensorLocation();
	UFUNCTION()
	void OnBeginOverlap(class AActor* OverlappedActor, class AActor* OtherActor);
	UFUNCTION()
	void OnEndOverlap(class AActor* OverlappedActor, class AActor* OtherActor);

	void SetSelected();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	FString SensorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	AActor* SensorActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttachedSensors)
	AStaticMeshActor* PIRSensor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttachedSensors)
	float PIRRepeatTime = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor)
	int32 ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttachedOutput)
	ALight* Light1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttachedOutput)
	ALight* Light2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttachedOutput)
	ALight* Light3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	int32 port = 5011;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Network)
	FString address = TEXT("127.0.0.1");

private:
	UStaticMeshComponent* mesh;
	FSensorState* state;
	FSensorHistory* history;
	TArray<FSensorHistory*> histories;

	TArray<USpotLightComponent*> Leds;
	ATriggerBase* tb;
	ISocketSubsystem* sockSubSystem;
	FSocket* socket;
	FVector prevLocation;
	flatbuffers::FlatBufferBuilder fbb;
	FIPv4Address ip;
	TSharedPtr<FInternetAddr> addr;
	int32 RecvSize = 0x8000;
	int32 SendSize = 0x8000;
	bool active = true;
	float activeTimer = 0;
	void sendLocationUpdate();
	bool bstateBeenModified = false;



};
