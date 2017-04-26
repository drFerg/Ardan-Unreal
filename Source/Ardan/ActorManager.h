#pragma once

struct FObjectMeta {
	FVector velocity;
	FVector angularVelocity;
	FTransform transform;
	float deltaTime;
	float timeStamp;
};

struct FObjectInfo {
	AActor* actor;
	FObjectInfo* ancestor;
	FObjectInfo* descendant;
	TArray<FObjectMeta*>* hist;
	int index;
	bool bisGhost = false;
};

struct FHistory {
	TMap<FString, FObjectInfo*> histMap;
	bool bfinished = false;
	int level = 1;
};

/**
 * 
 */
class ARDAN_API ActorManager {
private:
	UWorld* world;
	APlayerController* controller;
	TArray<FHistory*> pawnHistories;
	TArray<FHistory*> histories;
	FHistory* currentHistory;
	FHistory* currentPawnHistory;

	bool bReverse = false;
	bool bReplay = false;
	bool bRecording = false;
public:
	ActorManager(UWorld* w, APlayerController* c);
	~ActorManager();

	void initHist();

	void recordActors(float deltaTime, float timeStamp);
	void recordPawnActors(float deltaTime, float timeStamp);

	void copyActors(FHistory* dstHistory, FHistory *srcHistory);
	void copyPawnActors(FHistory * dstHistory, FHistory * srcHistory);

	void NewTimeline();

	void ResetTimelines();

	void rewindMeshActors(FHistory *history, bool freeze, float timeStamp);
	void rewindPawnActors(FHistory *history);

	void rewindCurrentMeshActors(bool freeze, float timeStamp);
	void rewindCurrentPawnActors();
	void rewindAllMeshActors(bool freeze, float timeStamp);
	void rewindAllPawnActors();

	void replayActors(FHistory *history, float timeStamp);
	void replayPawnActors(FHistory * history, float timeStamp);
	void replayCurrentActors(float timeStamp);
	void replayAllActors(float timeStamp);
	void replayCurrentPawnActors(float timeStamp);
	void replayAllPawnActors(float timestamp);

	void resetActors(FHistory *history);
	void resetPawnActors(FHistory * history);

	void diff(FObjectInfo * info);
	void ghostActor(AActor * mesh, float amount);
	void colourActor(AActor * mesh);
};
