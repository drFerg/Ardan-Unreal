
#include "Ardan.h"
#include "ActorManager.h"
#include "ArdanUtilities.h"

#include "EngineUtils.h"
#include "UObjectGlobals.h"
#include "CameraPawn.h"
#include "Engine/StaticMeshActor.h"

UActorManager::UActorManager() {
	
}

UActorManager::~UActorManager()
{
}

void UActorManager::init(UWorld* w, APlayerController* c) {
	world = w;
	controller = c;
}

void UActorManager::replayActors(FHistory *history, float timeStamp) {
	for (auto &itr : history->histMap) {
		FObjectInfo* info = &(itr.Value);
		UStaticMeshComponent* mesh = ((AStaticMeshActor*)info->actor)->GetStaticMeshComponent();
		TArray<FObjectMeta>* hist = &info->hist;
		/* Make sure the mesh is movable */
		//mesh->SetMobility(EComponentMobility::Movable);
		while (info->index < hist->Num() && (*hist)[info->index].timeStamp <= timeStamp) {
			//UE_LOG(LogNet, Log, TEXT("TSTAMP: %f <:> %f"), (*hist)[index]->timeStamp, curTime);
			FObjectMeta* meta = &(*hist)[info->index];
			mesh->SetSimulatePhysics(false);
			//mesh->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
			info->actor->SetActorTransform(meta->transform);
			mesh->SetPhysicsLinearVelocity(meta->velocity);
			mesh->SetPhysicsAngularVelocity(meta->angularVelocity);
			info->index++;
			//if (info->index >= (*hist).Num()) {
				//UE_LOG(LogNet, Log, TEXT("BREAK"));
				//mesh->SetMobility(EComponentMobility::Stationary);
			//	history->bfinished = true;
			//	mesh->SetSimulatePhysics(false);
			//	break;
		//	}
		}
	}
}

void UActorManager::replayPawnActors(FHistory *history, float timeStamp) {
	for (auto &itr : history->histMap) {
		FObjectInfo* info = &(itr.Value);
		TArray<FObjectMeta>* hist = &info->hist;
		//UE_LOG(LogNet, Log, TEXT("histNum %d/%d"), info->index, hist->Num());
		while (info->index < hist->Num() && (*hist)[info->index].timeStamp <= timeStamp) {
			FObjectMeta *meta = &(*hist)[info->index];
			//UE_LOG(LogNet, Log, TEXT("TSTAMP: %f <:> %f"), meta->timeStamp, timeStamp);
			//UE_LOG(LogNet, Log, TEXT("inputvector: %s"), *(meta->velocity.ToString()));

			info->actor->SetActorTransform(meta->transform);
			//((APawn*)info->actor)->AddMovementInput(meta->velocity);
			info->index++;
			//if (info->index >= hist->Num()) {
				//UE_LOG(LogNet, Log, TEXT("BREAK"));
				/*info->actor->StaticMeshComponent->SetSimulatePhysics(false);*/
				//				mesh->SetMobility(EComponentMobility::Stationary);
				//history->bfinished = true;
				//break;
			//}
		}
	}
}

void UActorManager::replayCurrentActors(float timeStamp) {
	if (!currentHistory->bfinished) replayActors(currentHistory, timeStamp);
}

void UActorManager::replayAllActors(float timeStamp) {
	for (FHistory *history : histories) {
		if (!history->bfinished) replayActors(history, timeStamp);
	}
}

void UActorManager::replayCurrentPawnActors(float timeStamp) {
	if (!currentPawnHistory->bfinished) replayPawnActors(currentPawnHistory, timeStamp);
}

void UActorManager::replayAllPawnActors(float timeStamp) {
	for (FHistory *history : pawnHistories) {
		if (!history->bfinished) replayPawnActors(history, timeStamp);
	}
}


void UActorManager::FixReferences(FHistory* history) {
	/* Find all actors in world referenced by names in history */
	for (TActorIterator<AStaticMeshActor> ActorItr(world); ActorItr; ++ActorItr) {
		AStaticMeshActor *actor = *ActorItr;
		// Check if actor can actually move, if not don't bother fixing.
		if (!actor->GetStaticMeshComponent()->Mobility) continue;
		FObjectInfo* info = history->histMap.Find(actor->GetName());
		if (info == NULL) continue;
		info->actor = actor;
		info->ancestor = history->histMap.Find(info->ancestorName);
		info->descendant = history->histMap.Find(info->descendantName);
	}
}

void UActorManager::FixPawnReferences(FHistory* history) {
	/* Find all pawn actors in world referenced by names in history */
	for (TActorIterator<APawn> ActorItr(world); ActorItr; ++ActorItr) {
		APawn *actor = *ActorItr;
		// Check if actor can actually move, if not don't bother recording.
		FObjectInfo* info = history->histMap.Find(actor->GetName());
		if (info == NULL) continue;
		info->actor = actor;
		info->ancestor = history->histMap.Find(info->ancestorName);
		info->descendant = history->histMap.Find(info->descendantName);
	}
}

void UActorManager::resetActors(FHistory *history) {
	// Reset all actors to their initial start position and spawn new ghost objects to repeat their previous run
	history->bfinished = false;
	for (auto &itr : history->histMap) {
		FObjectInfo* info = &(itr.Value);
		if (info->actor == NULL) continue;
		UStaticMeshComponent* mesh = ((AStaticMeshActor*)info->actor)->GetStaticMeshComponent();
		// Check if actor has an initial position and reset to it
		if (info->hist.Num()) {
			info->index = 0;
			FObjectMeta *meta = &(info->hist[info->index]);
			info->actor->SetActorTransform(meta->transform);
			info->actor->SetActorEnableCollision(false);
			mesh->SetPhysicsLinearVelocity(meta->velocity);
			mesh->SetPhysicsAngularVelocity(meta->angularVelocity);
			// Apply Ghost material to old model
			ghostActor((AStaticMeshActor*)info->actor, 0.7 / history->level);
		}
	}
}

void UActorManager::resetPawnActors(FHistory *history) {
	// Reset all actors to their initial start position and spawn new ghost objects to repeat their previous run
	history->bfinished = false;
	for (auto &itr : history->histMap) {
		FObjectInfo* info = &(itr.Value);
		// Check if actor has an initial position and reset to it
		if (info->hist.Num()) {
			info->index = 0;
			FObjectMeta *meta = &(info->hist[info->index]);
			info->actor->SetActorEnableCollision(false);
			info->actor->SetActorTransform(meta->transform);
			// Apply Ghost material to old model
			//ghostActor((AStaticMeshActor*)info->actor, 0.7 / history->level);
		}
	}
}

void UActorManager::recordActors(float deltaTime, float timeStamp) {
	// Record a single tick for all actors
	for (auto &itr : currentHistory->histMap) {
		FObjectInfo* info = &(itr.Value);
		
		UStaticMeshComponent* mesh = ((AStaticMeshActor*)info->actor)->GetStaticMeshComponent();
		FTransform curTrans = info->actor->GetTransform();

		if (info->hist.Num() && curTrans.Equals(info->hist.Last().transform))	continue;

		FObjectMeta meta;
		meta.transform = curTrans;
		meta.velocity = mesh->GetPhysicsLinearVelocity();
		meta.angularVelocity = mesh->GetPhysicsAngularVelocity();
		meta.deltaTime = deltaTime;
		meta.timeStamp = timeStamp;
		info->hist.Add(meta);
		info->index++;

	}
}

void UActorManager::recordPawnActors(float deltaTime, float timeStamp, bool displayPath) {
	// Record a single tick for all actors
	for (auto &itr : currentPawnHistory->histMap) {
		FObjectInfo* info = &(itr.Value);
		if (info == NULL) continue;
		FTransform curTrans = info->actor->GetTransform();
		if (info->hist.Num()) {
			if (curTrans.Equals(info->hist.Last().transform))	continue;
			
			FVector sourceLoc = curTrans.GetLocation();
			FRotator sourceRot = curTrans.GetRotation().Rotator();
			if (displayPath) {
				FActorSpawnParameters SpawnInfo;
				SpawnInfo.Owner = NULL;
				SpawnInfo.Instigator = NULL;
				SpawnInfo.bDeferConstruction = false;
				SpawnInfo.bNoFail = false;
				//SpawnInfo.bNoCollisionFail = bNoCollisionFail;
				ATimeSphere *ts = this->world->SpawnActor<ATimeSphere>(sourceLoc, sourceRot, SpawnInfo);
				bool reset = false;
				if (info->lastSphere && !reset) {
					DrawDebugLine(this->world,
						curTrans.GetLocation(), info->lastSphere->GetActorLocation(),
						
						info->color, false, 60, 0, 4);
				}
				info->lastSphere = ts;
				reset = false;
			}
		}
		FObjectMeta meta;
		meta.transform = curTrans;
		meta.velocity = ((APawn*)info->actor)->GetPendingMovementInputVector();
		meta.deltaTime = deltaTime;
		meta.timeStamp = timeStamp;
		info->hist.Add(meta);
		info->index++;
	}
}

void UActorManager::rewindMeshActors(FHistory *history, bool freeze, float timeStamp) {
	// Rewind each recorded actor one tick

	history->bfinished = false;
	for (auto &itr : history->histMap) {
		FObjectInfo* actorInfo = &(itr.Value);
		UStaticMeshComponent *mesh = ((AStaticMeshActor *)actorInfo->actor)->GetStaticMeshComponent();
		//UE_LOG(LogNet, Log, TEXT("ATime: %f : %f: %d"), (*actorInfo->hist)[actorInfo->index]->timeStamp, curTime, actorInfo->index);
		while (actorInfo->index > 0 && (actorInfo->hist[actorInfo->index - 1]).timeStamp >= timeStamp) {
			actorInfo->index--;
			FObjectMeta* meta = &(actorInfo->hist[actorInfo->index]);
			mesh->SetMobility(EComponentMobility::Movable);
			actorInfo->actor->SetActorTransform(meta->transform);
			mesh->SetPhysicsLinearVelocity(meta->velocity);
			mesh->SetPhysicsAngularVelocity(meta->angularVelocity);
		}
		//if (freeze) mesh->SetMobility(EComponentMobility::Stationary);
	}
}


void UActorManager::rewindPawnActors(FHistory *history, float timeStamp) {
	// Rewind each recorded actor one tick
	for (auto &itr : history->histMap) {
		FObjectInfo *actorInfo = &(itr.Value);
		/* Rewind until next state's timestamp is less/equal to requested time */
		while (actorInfo->index > 0 && actorInfo->hist[actorInfo->index - 1].timeStamp >= timeStamp) {
			FObjectMeta* meta = &(actorInfo->hist[--actorInfo->index]);
			actorInfo->actor->SetActorTransform(meta->transform);
		}
	}
}

void UActorManager::rewindCurrentMeshActors(bool freeze, float timeStamp) {
	rewindMeshActors(currentHistory, freeze, timeStamp);
}

void UActorManager::rewindCurrentPawnActors(float timeStamp) {
	rewindPawnActors(currentPawnHistory, timeStamp);
}

void UActorManager::rewindAllMeshActors(bool freeze, float timeStamp) {
	for (FHistory *history : histories) {
		rewindMeshActors(history, freeze, timeStamp);
	}
}

void UActorManager::rewindAllPawnActors(float timeStamp) {
	for (FHistory *history : pawnHistories) {
		rewindPawnActors(history, timeStamp);
	}
}

void UActorManager::initHist() {
	currentPawnHistory = new FHistory();
	currentHistory = new FHistory();
	// Initialise and add all static mesh actors records to our history map
	for (TActorIterator<AStaticMeshActor> ActorItr(world); ActorItr; ++ActorItr) {
		AStaticMeshActor *actor = *ActorItr;

		// Check if actor can actually move, if not don't bother recording.
		if (!actor->GetStaticMeshComponent()->Mobility) continue;
		UE_LOG(LogNet, Log, TEXT("A: %s"), *actor->GetName());

		// Set up actorInfo object to record actor history
		FObjectInfo actorInfo;
		actorInfo.actor = actor;
		actorInfo.actorName = actor->GetName();
		actorInfo.ancestor = NULL;
		actorInfo.descendant = NULL;
		actorInfo.index = 0;
		actorInfo.bisGhost = false;
		//actorInfo->hist = new TArray<FObjectMeta*>();
		//TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		currentHistory->histMap.Add(actor->GetName(), actorInfo);
	}

	for (TActorIterator<ACharacter> ActorItr(world); ActorItr; ++ActorItr) {
		APawn *actor = *ActorItr;
		if (actor->GetClass() == ACameraPawn::StaticClass()) continue;
		UE_LOG(LogNet, Log, TEXT("A: %s"), *actor->GetName());
		// Set up actorInfo object to record actor history
		FObjectInfo actorInfo;
		actorInfo.actor = actor;
		actorInfo.actorName = actor->GetName();
		actorInfo.ancestor = NULL;
		actorInfo.descendant = NULL;
		actorInfo.index = 0;
		actorInfo.bisGhost = false;
		actorInfo.lastSphere = NULL;
		actorInfo.color = FColor(FMath::RandRange(0, 255), FMath::RandRange(0, 255), FMath::RandRange(0, 255));
		//actorInfo->hist = new TArray<FObjectMeta*>();
		//TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		currentPawnHistory->histMap.Add(actor->GetName(), actorInfo);
	}
}

void UActorManager::copyActors(FHistory* dstHistory, FHistory *srcHistory) {
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnParams.Template = NULL;
	for (auto &iter : srcHistory->histMap) {
		FObjectInfo* info = &(iter.Value);
		spawnParams.Template = info->actor;
		AStaticMeshActor *newb = world->SpawnActor<AStaticMeshActor>(info->actor->GetClass(), info->actor->GetActorTransform(), spawnParams);
		//force new actor to old actors position 
		newb->SetActorTransform(info->actor->GetActorTransform());
		// renable collision
		newb->SetActorEnableCollision(true);

		// Apply normal material to new model
		colourActor(newb);
		/* Create history in current history*/
		FObjectInfo actorInfo;
		actorInfo.actor = newb;
		actorInfo.actorName = newb->GetName();
		actorInfo.ancestor = info;
		actorInfo.ancestorName = info->actorName;
		actorInfo.index = 0;
		info->descendant = &actorInfo;
		info->descendantName = actorInfo.actorName;
		//actorInfo->hist = new TArray<FObjectMeta*>();
		//TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		dstHistory->histMap.Add(newb->GetName(), actorInfo);
	}
}

void UActorManager::copyPawnActors(FHistory* dstHistory, FHistory *srcHistory) {
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnParams.Template = NULL;
	//Pause();
	for (auto &iter : srcHistory->histMap) {
		FObjectInfo* info = &(iter.Value);
		//if (info->actor->GetActorClass() == 
		//spawnParams.Template = info->actor;
		UE_LOG(LogNet, Log, TEXT("A: %s"), *(info->actor->GetClass()->GetName()));
		APawn *newb = world->SpawnActor<APawn>(info->actor->GetClass(), info->actor->GetActorTransform(), spawnParams);
		// Possess the new actor
		newb->SpawnDefaultController();
		ACharacter* c = (ACharacter*)newb;
		c->GetCharacterMovement()->MaxWalkSpeed = ((ACharacter*)info->actor)->GetCharacterMovement()->MaxWalkSpeed;
		if (controller->GetPawn() == info->actor) controller->Possess(newb);
		//force new actor to old actors position 
		newb->SetActorTransform(info->actor->GetActorTransform());
		// renable collision
		newb->SetActorEnableCollision(true);
		ghostActor(info->actor, 0.5);
		// Apply normal material to new model
		//colourActor(newb);
		/* Create history in current history*/
		FObjectInfo actorInfo;
		actorInfo.actor = newb;
		actorInfo.actorName = newb->GetName();
		actorInfo.ancestor = info;
		actorInfo.ancestorName = info->actorName;
		actorInfo.index = 0;
		info->descendant = &actorInfo;
		info->descendantName = actorInfo.actorName;
		//actorInfo->hist = new TArray<FObjectMeta*>();
		//TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		dstHistory->histMap.Add(newb->GetName(), actorInfo);
	}
}

void UActorManager::NewTimeline() {
	FHistory* oldHistory = currentHistory;
	histories.Push(currentHistory);
	for (FHistory *history : histories) {
		resetActors(history);
		history->level++;
	}
	currentHistory = new FHistory();
	copyActors(currentHistory, oldHistory);

	oldHistory = currentPawnHistory;
	pawnHistories.Push(currentPawnHistory);
	for (FHistory *history : pawnHistories) {
		resetPawnActors(history);
		history->level++;
	}

	currentPawnHistory = new FHistory();
	copyPawnActors(currentPawnHistory, oldHistory);
}

void UActorManager::ResetTimelines() {
	resetActors(currentHistory);
	for (FHistory *history : histories) {
		resetActors(history);
	}

	resetPawnActors(currentPawnHistory);
	for (FHistory *history : pawnHistories) {
		resetPawnActors(history);
	}
}

void UActorManager::diff(FObjectInfo* info) {
	info = (currentPawnHistory->histMap.Find(controller->GetPawn()->GetName()));
	if (!info || !info->ancestor) return;
	FObjectInfo *anc = info->ancestor;
	float dist = anc->actor->GetDistanceTo(info->actor);
	UE_LOG(LogNet, Log, TEXT("dIST: %f"), dist);
	if (dist > 50.0) {
		UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("MaterialInstanceConstant'/Game/Materials/Red_Body.Red_Body'"));
		((USkeletalMeshComponent*)info->actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()))->SetMaterial(0, mat);
	}
	else {
		UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("Material'/Game/Mannequin/Character/Materials/M_UE4Man_Body.M_UE4Man_Body'"));
		((USkeletalMeshComponent*)info->actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()))->SetMaterial(0, mat);
	}
}

void UActorManager::ghostActor(AActor *mesh, float amount) {

	UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("Material'/Game/Materials/Transparency_Material.Transparency_Material'"));
	UMaterialInstanceDynamic* matInst = UMaterialInstanceDynamic::Create(mat, NULL); //BaseMat must have material parameter called "Color"
	matInst->SetScalarParameterValue(FName("Transparency_Amount"), amount);
	((UMeshComponent*)mesh->GetComponentByClass(UMeshComponent::StaticClass()))->SetMaterial(0, matInst);
}

void UActorManager::colourActor(AActor *mesh) {
	UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("Material'/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial'"));
	((UMeshComponent*)mesh->GetComponentByClass(UMeshComponent::StaticClass()))->SetMaterial(0, mat);
}