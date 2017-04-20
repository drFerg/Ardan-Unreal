// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Ardan.h"
#include "ArdanPlayerController.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "Sensor.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "UObjectGlobals.h"
#include "Runtime/Engine/Classes/Camera/CameraActor.h"
#include "ArdanCharacter.h"
#include "ArdanUtilities.h"

AArdanPlayerController::AArdanPlayerController() {
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	/* Allows camera movement when world is paused */
	SetTickableWhenPaused(true);
	bShouldPerformFullTickWhenPaused = true;

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	UBlueprint* blueprint = Cast<UBlueprint>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("Blueprint'/Game/SensorNode.SensorNode'")));
	sensorBlueprint = (UClass*)(blueprint->GeneratedClass);
	if (sensorBlueprint != NULL) {
		UE_LOG(LogNet, Log, TEXT("BName: %s"), *(sensorBlueprint->GetClass()->GetName()));
	} else {
		UE_LOG(LogNet, Log, TEXT("I got nothing"));
	}

  /* Networking setup */
	sockSubSystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	socket = sockSubSystem->CreateSocket(NAME_DGram, TEXT("UDPCONN2"), true);
	if (socket) UE_LOG(LogNet, Log, TEXT("Created Socket"));
	socket->SetReceiveBufferSize(RecvSize, RecvSize);
	socket->SetSendBufferSize(SendSize, SendSize);

	/* Set up destination address:port to send Cooja messages to */
	FIPv4Address::Parse(address, ip);
	addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.Value);
	addr->SetPort(port);
}

void AArdanPlayerController::BeginPlay() {
  Super::BeginPlay();
  conns = FRunnableConnection::create(5000, &packetQ);
  if (conns) {UE_LOG(LogNet, Log, TEXT("Runnable Connection created"));}
  else { UE_LOG(LogNet, Log, TEXT("conns failed"));}
  // conns->Init();
  // conns->Run();

	TSubclassOf<ACameraActor> ClassToFind;
	cameras.Empty();
	currentCam = 0;
	for(TActorIterator<AActor> It(GetWorld(), ACameraActor::StaticClass()); It; ++It)
	{
		ACameraActor* Actor = (ACameraActor*)*It;
		if(!Actor->IsPendingKill()) {
			cameras.Add(Actor);
      Actor->SetTickableWhenPaused(true);
		}
	}
	UE_LOG(LogNet, Log, TEXT("Found %d cameras"), cameras.Num());
	//if (cameras.Num()) NextCamera();

	sensorManager = new SensorManager(GetWorld());
	sensorManager->FindSensors();
  initHist();
  UE_LOG(LogNet, Log, TEXT("--START--"));
}

void AArdanPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (conns) {
    conns->Stop();
    conns->Shutdown();
  }
	delete sensorManager;
}

void AArdanPlayerController::replayActors(FHistory *history) {
	for (auto &itr : history->histMap) {
		FObjectInfo* info = itr.Value;
		UStaticMeshComponent* mesh = ((AStaticMeshActor*)info->actor)->GetStaticMeshComponent();
		TArray<FObjectMeta*>* hist = info->hist;
		/* Make sure the mesh is movable */
		//mesh->SetMobility(EComponentMobility::Movable);
		while (info->index < hist->Num() && (*hist)[info->index]->timeStamp <= curTime) {
			//UE_LOG(LogNet, Log, TEXT("TSTAMP: %f <:> %f"), (*hist)[index]->timeStamp, curTime);
			FObjectMeta *meta = (*hist)[info->index];
			mesh->SetSimulatePhysics(false);
			info->actor->SetActorTransform(meta->transform);
			mesh->SetPhysicsLinearVelocity(meta->velocity);
			mesh->SetPhysicsAngularVelocity(meta->angularVelocity);
			info->index += 1;
			if (info->index >= (*hist).Num()) {
				//UE_LOG(LogNet, Log, TEXT("BREAK"));
				//mesh->SetMobility(EComponentMobility::Stationary);
				history->bfinished = true;
				break;
			}
		}
	}
}

void AArdanPlayerController::replayPawnActors(FHistory *history) {
	for (auto &itr : history->histMap) {
		FObjectInfo* info = itr.Value;
		TArray<FObjectMeta*>* hist = info->hist;
		while (info->index < hist->Num() && (*hist)[info->index]->timeStamp <= curTime) {
			FObjectMeta *meta = (*hist)[info->index];
			//UE_LOG(LogNet, Log, TEXT("inputvector: %s"), *(meta->velocity.ToString()));

			info->actor->SetActorTransform(meta->transform);
			//((APawn*)info->actor)->AddMovementInput(meta->velocity);
			info->index += 1;
			if (info->index >= (*hist).Num()) {
				//UE_LOG(LogNet, Log, TEXT("BREAK"));
				/*info->actor->StaticMeshComponent->SetSimulatePhysics(false);*/
//				mesh->SetMobility(EComponentMobility::Stationary);
				history->bfinished = true;
				break;
			}
		}
	}
}

void AArdanPlayerController::resetActors(FHistory *history) {
	// Reset all actors to their initial start position and spawn new ghost objects to repeat their previous run
	history->bfinished = false;
  for (auto &itr : history->histMap) {
	  FObjectInfo* info = itr.Value;
	  UStaticMeshComponent* mesh = ((AStaticMeshActor*)info->actor)->GetStaticMeshComponent();
	  // Check if actor has an initial position and reset to it
	  if (info->hist->Num()) {
			info->index = 0;
		  FObjectMeta *meta = (*info->hist)[info->index];
			info->actor->SetActorTransform(meta->transform);
			info->actor->SetActorEnableCollision(false);
			//mesh->SetMobility(EComponentMobility::Movable);
			mesh->SetPhysicsLinearVelocity(meta->velocity);
		  mesh->SetPhysicsAngularVelocity(meta->angularVelocity);
			// Apply Ghost material to old model
			ghostActor((AStaticMeshActor*)info->actor, 0.7 / history->level);
			//UE_LOG(LogNet, Log, TEXT("(%d)A: %s : %s : %s"), z++, *info->actor->GetName(), *info->actor->GetClass()->GetName(), *info->actor->GetTransform().ToString());

			
			//UE_LOG(LogNet, Log, TEXT("%s"), (newb != NULL ? TEXT("YES") : TEXT("NO")))
	  }
  }
}

void AArdanPlayerController::resetPawnActors(FHistory *history) {
	// Reset all actors to their initial start position and spawn new ghost objects to repeat their previous run
	history->bfinished = false;
	for (auto &itr : history->histMap) {
		FObjectInfo* info = itr.Value;
		//UStaticMeshComponent* mesh = ((AStaticMeshActor*)info->actor)->GetStaticMeshComponent();
		//info->bisGhost = true;
		// Check if actor has an initial position and reset to it
		if (info->hist->Num()) {
			info->index = 0;
			FObjectMeta *meta = (*info->hist)[info->index];
			info->actor->SetActorTransform(meta->transform);
			info->actor->SetActorEnableCollision(false);
			/*mesh->SetMobility(EComponentMobility::Movable);
			mesh->SetPhysicsLinearVelocity(meta->velocity);
			mesh->SetPhysicsAngularVelocity(meta->angularVelocity);*/
			// Apply Ghost material to old model
			//ghostActor((AStaticMeshActor*)info->actor, 0.7 / history->level);
			//UE_LOG(LogNet, Log, TEXT("(%d)A: %s : %s : %s"), z++, *info->actor->GetName(), *info->actor->GetClass()->GetName(), *info->actor->GetTransform().ToString());


			//UE_LOG(LogNet, Log, TEXT("%s"), (newb != NULL ? TEXT("YES") : TEXT("NO")))
		}
	}
}

void AArdanPlayerController::recordActors(float deltaTime) {
	// Record a single tick for all actors
	for (auto &itr : currentHistory->histMap) {
		FObjectInfo* info = itr.Value;
		info->index++;
		UStaticMeshComponent* mesh = ((AStaticMeshActor*)info->actor)->GetStaticMeshComponent();
		FTransform curTrans = info->actor->GetTransform();
		FObjectMeta *meta = NULL;

		meta = new FObjectMeta();
		meta->transform = curTrans;
		meta->velocity = mesh->GetPhysicsLinearVelocity();
		meta->angularVelocity = mesh->GetPhysicsAngularVelocity();
		meta->deltaTime = deltaTime;
		meta->timeStamp = curTime;

		info->hist->Add(meta);

	}
}

void AArdanPlayerController::recordPawnActors(float deltaTime) {
	// Record a single tick for all actors
	for (auto &itr : currentPawnHistory->histMap) {
		FObjectInfo* info = itr.Value;
		info->index++;
		FTransform curTrans = info->actor->GetTransform();
		FObjectMeta *meta = new FObjectMeta();
		meta->transform = curTrans;
		meta->velocity = ((APawn*)info->actor)->GetPendingMovementInputVector();
		meta->deltaTime = deltaTime;
		meta->timeStamp = curTime;
		info->hist->Add(meta);
	}
}

void AArdanPlayerController::rewindMeshActors(FHistory *history, bool freeze) {
	// Rewind each recorded actor one tick
	
	history->bfinished = false;
	for (auto &itr : history->histMap) {
		FObjectInfo *actorInfo = itr.Value;
		UStaticMeshComponent *mesh = ((AStaticMeshActor *)actorInfo->actor)->GetStaticMeshComponent();
		//UE_LOG(LogNet, Log, TEXT("ATime: %f : %f: %d"), (*actorInfo->hist)[actorInfo->index]->timeStamp, curTime, actorInfo->index);
		while (actorInfo->index > 0 && (*actorInfo->hist)[actorInfo->index - 1]->timeStamp >= curTime) {
			actorInfo->index--;
			FObjectMeta *meta = (*actorInfo->hist)[actorInfo->index];
			mesh->SetMobility(EComponentMobility::Movable);
			actorInfo->actor->SetActorTransform(meta->transform);
			mesh->SetPhysicsLinearVelocity(meta->velocity);
			mesh->SetPhysicsAngularVelocity(meta->angularVelocity);
			//actorInfo->index--;
		}
		// Check we haven't reached the beginning of time, if so set to static so it physics doesn't take over
		//if (freeze) mesh->SetMobility(EComponentMobility::Stationary);
	}
}


void AArdanPlayerController::rewindPawnActors(FHistory *history) {
	// Rewind each recorded actor one tick
	for (auto &itr : history->histMap) {
		FObjectInfo *actorInfo = itr.Value;
		if (actorInfo->hist->Num()) {
			if (actorInfo->index == 0) continue;
			FObjectMeta *meta = (*actorInfo->hist)[--(actorInfo->index)];
			actorInfo->actor->SetActorTransform(meta->transform);
		}
	}
}

void AArdanPlayerController::initHist() {
	currentPawnHistory = new FHistory();
	currentHistory = new FHistory();
	// Initialise and add all static mesh actors records to our history map
	for (TActorIterator<AStaticMeshActor> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		AStaticMeshActor *actor = *ActorItr;

		// Check if actor can actually move, if not don't bother recording.
		if (!actor->GetStaticMeshComponent()->Mobility) continue;
		UE_LOG(LogNet, Log, TEXT("A: %s"), *actor->GetName());

		// Set up actorInfo object to record actor history
		FObjectInfo *actorInfo = new FObjectInfo();
		actorInfo->actor = actor;
		actorInfo->hist = new TArray<FObjectMeta*>();
		TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		currentHistory->histMap.Add(actor->GetName(), actorInfo);
	}

	for (TActorIterator<APawn> ActorItr(GetWorld()); ActorItr; ++ActorItr) {
		APawn *actor = *ActorItr;
		UE_LOG(LogNet, Log, TEXT("A: %s"), *actor->GetName());
		// Set up actorInfo object to record actor history
		FObjectInfo *actorInfo = new FObjectInfo();
		actorInfo->actor = actor;
		actorInfo->hist = new TArray<FObjectMeta*>();
		TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		currentPawnHistory->histMap.Add(actor->GetName(), actorInfo);
	}
}

void AArdanPlayerController::copyActors(FHistory* dstHistory, FHistory *srcHistory) {
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnParams.Template = NULL;
	for (auto &iter : srcHistory->histMap) {
		FObjectInfo* info = iter.Value;
		spawnParams.Template = info->actor;
		AStaticMeshActor *newb = GetWorld()->SpawnActor<AStaticMeshActor>(info->actor->GetClass(), info->actor->GetActorTransform(), spawnParams);
		//force new actor to old actors position 
		newb->SetActorTransform(info->actor->GetActorTransform());
		// renable collision
		newb->SetActorEnableCollision(true);
		
		// Apply normal material to new model
		colourActor(newb);
		/* Create history in current history*/
		FObjectInfo *actorInfo = new FObjectInfo();
		actorInfo->actor = newb;
		actorInfo->ancestor = info;
		info->descendant = actorInfo;
		actorInfo->hist = new TArray<FObjectMeta*>();
		TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		dstHistory->histMap.Add(newb->GetName(), actorInfo);
	}
}

void AArdanPlayerController::copyPawnActors(FHistory* dstHistory, FHistory *srcHistory) {
	FActorSpawnParameters spawnParams;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnParams.Template = NULL;
	//Pause();
	for (auto &iter : srcHistory->histMap) {
		FObjectInfo* info = iter.Value;
		//if (info->actor->GetActorClass() == 
		//spawnParams.Template = info->actor;
		UE_LOG(LogNet, Log, TEXT("A: %s"), *(info->actor->GetClass()->GetName()));
		APawn *newb = GetWorld()->SpawnActor<APawn>(info->actor->GetClass(), info->actor->GetActorTransform(), spawnParams);
		// Possess the new actor
		newb->SpawnDefaultController();
		if (GetPawn() == info->actor) this->Possess(newb);
		//force new actor to old actors position 
		newb->SetActorTransform(info->actor->GetActorTransform());
		// renable collision
		newb->SetActorEnableCollision(true);
		ghostActor(info->actor, 0.5);
		// Apply normal material to new model
		//colourActor(newb);
		/* Create history in current history*/
		FObjectInfo *actorInfo = new FObjectInfo();
		actorInfo->actor = newb;
		actorInfo->ancestor = info;
		info->descendant = actorInfo;
		actorInfo->hist = new TArray<FObjectMeta*>();
		TArray<FObjectMeta*> *hist = new TArray<FObjectMeta*>();
		dstHistory->histMap.Add(newb->GetName(), actorInfo);
	}
	//Pause();
}

void AArdanPlayerController::diff(FObjectInfo* info) {
	if (!info || !info->ancestor) return;
	FObjectInfo *anc = info->ancestor;
	float dist = anc->actor->GetDistanceTo(info->actor);
	UE_LOG(LogNet, Log, TEXT("dIST: %f"), dist);
	if (dist > 50.0) {
		UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("Material'/Game/Materials/TimeSphere.TimeSphere'"));
		((USkeletalMeshComponent*) info->actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()))->SetMaterial(0, mat);
	}
	else {
		UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("Material'/Game/Mannequin/Character/Materials/M_UE4Man_Body.M_UE4Man_Body'"));
		((USkeletalMeshComponent*)info->actor->GetComponentByClass(USkeletalMeshComponent::StaticClass()))->SetMaterial(0, mat);
	}
}

void AArdanPlayerController::replayPressed() {
  bReplay = true;
  curTime = 0;
  replayTime = 0;
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
	int z = 0;
}

void AArdanPlayerController::ghostActor(AActor *mesh, float amount) {
	
	UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("Material'/Game/Materials/Transparency_Material.Transparency_Material'"));
	UMaterialInstanceDynamic* matInst = UMaterialInstanceDynamic::Create(mat, this); //BaseMat must have material parameter called "Color"
	matInst->SetScalarParameterValue(FName("Transparency_Amount"), amount);
	((UMeshComponent*) mesh->GetComponentByClass(UMeshComponent::StaticClass()))->SetMaterial(0, matInst);
	//mesh->GetStaticMeshComponent()->SetMaterial(0, matInst);
}

void AArdanPlayerController::colourActor(AActor *mesh) {
	UMaterialInterface *mat = ArdanUtilities::LoadMatFromPath(TEXT("Material'/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial'"));
	//mesh->GetStaticMeshComponent()->SetMaterial(0, mat);
	((UMeshComponent*)mesh->GetComponentByClass(UMeshComponent::StaticClass()))->SetMaterial(0, mat);
}

void AArdanPlayerController::PlayerTick(float DeltaTime) {
	Super::PlayerTick(DeltaTime);

	if (UGameplayStatics::IsGamePaused(GetWorld())) return;

	APawn* const Pawn = GetPawn();
  FVector sourceLoc = Pawn->GetActorLocation();
	FRotator sourceRot = Pawn->GetActorRotation();
  FTransform *transform = new FTransform();
  *transform = Pawn->GetTransform();
	/* Pop and set actors old location else push current location onto stack */

	if (bReverse) {
		replayTime -= DeltaTime;
		curTime -= DeltaTime;

		rewindMeshActors(currentHistory, false);
		for (FHistory *history : histories) {
			rewindMeshActors(history, true);
		}

		rewindPawnActors(currentPawnHistory);
		for (FHistory *history : pawnHistories) {
			rewindPawnActors(history);
		}
		return;
	}
	else {
		curTime += DeltaTime;
		recordActors(DeltaTime);
		recordPawnActors(DeltaTime);
		if (bReplay) {
			replayTime += DeltaTime;
			for (FHistory *history : histories) {
				if (!history->bfinished) replayActors(history);
			}
			for (FHistory *history : pawnHistories) {
				if (!history->bfinished) replayPawnActors(history);
			}
			/*recordActors(DeltaTime);
			recordPawnActors(DeltaTime);*/
		}
	}
  
	diff(*(currentPawnHistory->histMap.Find(GetPawn()->GetName())));
	

	// /* Working out FPS */
	elapsed += DeltaTime;
	tenth += DeltaTime;
	tickCount += 1;
	if (elapsed >= 1) {
		UE_LOG(LogNet, Log, TEXT("TSTAMP: %f %f %f %d "), DeltaTime, 1/DeltaTime,
					elapsed/tickCount,
					tickCount);
		elapsed = 0;
		tickCount = 0;
	}
	if (tenth >= 0.1){

		tenth = 0;
		// DrawDebugCircle(GetWorld(), sourceLoc, 5.0, 360, colours[0], false, 30, 0, 1, FVector(1.f,0.f,0.f), FVector(0.f,1.f,0.f), false);
		// ATimeSphere *ts = NewObject<ATimeSphere>();
		// ATimeSphere *ts = SpawnActor<ATimeSphere>(GetWorld(), Pawn->GetActorLocation(), Pawn->GetActorRotation(), NULL, NULL, false);
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = NULL;
		SpawnInfo.Instigator = NULL;
		SpawnInfo.bDeferConstruction = false;
		SpawnInfo.bNoFail 		= false;
		// SpawnInfo.bNoCollisionFail = bNoCollisionFail;
		// UE_LOG(LogNet, Log, TEXT("Location: %s"), *(sourceLoc.ToString()));
	  ATimeSphere *ts = GetWorld()->SpawnActor<ATimeSphere>(sourceLoc, sourceRot, SpawnInfo);
		if (timeSpheres.Num() > 0) {
  		DrawDebugLine(GetWorld(),
  			ts->GetActorLocation(), timeSpheres.Top()->GetActorLocation(),
  			FColor(255,0,0), false, 30, 0, 4);
		}
		if (ts != NULL) {
		// ts->init(sourceLoc);
		timeSpheres.Push(ts);
	}
	else UE_LOG(LogNet, Log, TEXT("NOOOOOOOOOOOOOOO"));
	}
	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}
	update_sensors();
}

void AArdanPlayerController::update_sensors() {
  uint8* pkt;
  int count = 0;
  while (count++ < 5 && !packetQ.IsEmpty()){
    packetQ.Dequeue(pkt);
		sensorManager->ReceivePacket(pkt);
		free(pkt);
  }
}

void AArdanPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &AArdanPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &AArdanPlayerController::OnSetDestinationReleased);

	// support touch devices
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AArdanPlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AArdanPlayerController::MoveToTouchLocation);

	InputComponent->BindAction("ResetVR", IE_Pressed, this, &AArdanPlayerController::OnResetVR);

	InputComponent->BindAction("ScrollUp", IE_Pressed, this, &AArdanPlayerController::ScrollUp);
	InputComponent->BindAction("ScrollDown", IE_Pressed, this, &AArdanPlayerController::ScrollDown);

	InputComponent->BindAction("Pause", IE_Pressed, this, &AArdanPlayerController::Pause).bExecuteWhenPaused = true;
	InputComponent->BindAction("NextCamera", IE_Pressed, this, &AArdanPlayerController::NextCamera);

  InputComponent->BindAction("Slow", IE_Pressed, this, &AArdanPlayerController::speedSlow);
  InputComponent->BindAction("Normal", IE_Pressed, this, &AArdanPlayerController::speedNormal);
  InputComponent->BindAction("Fast", IE_Pressed, this, &AArdanPlayerController::speedFast);
  InputComponent->BindAction("Reverse", IE_Pressed, this, &AArdanPlayerController::reversePressed);
  InputComponent->BindAction("Reverse", IE_Released, this, &AArdanPlayerController::reverseReleased);

  InputComponent->BindAction("ResetReplay", IE_Pressed, this, &AArdanPlayerController::replayPressed);

}

void AArdanPlayerController::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AArdanPlayerController::MoveToMouseCursor()
{
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		if (AArdanCharacter* MyPawn = Cast<AArdanCharacter>(GetPawn()))
		{
			if (MyPawn->GetCursorToWorld())
			{
				UNavigationSystem::SimpleMoveToLocation(this, MyPawn->GetCursorToWorld()->GetComponentLocation());
			}
		}
	}
	else
	{
		// Trace to see what is under the mouse cursor
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit)
		{
			// We hit something, move there
			SetNewMoveDestination(Hit.ImpactPoint);
		}
	}
}

void AArdanPlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void AArdanPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
	{
		UNavigationSystem* const NavSys = GetWorld()->GetNavigationSystem();
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if (NavSys && (Distance > 120.0f))
		{
			NavSys->SimpleMoveToLocation(this, DestLocation);
		}
	}
}

void AArdanPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void AArdanPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}


void AArdanPlayerController::reversePressed() {
  bReverse = true;
}

void AArdanPlayerController::reverseReleased() {
  bReverse = false;
  //Pause();
}

void AArdanPlayerController::NextCamera() {
	if (cameras.Num() == 0) return;
	this->SetViewTargetWithBlend((ACameraActor *)cameras[currentCam], SmoothBlendTime);
	currentCam = (currentCam + 1 ) % cameras.Num();
}

void AArdanPlayerController::Pause() {
  UE_LOG(LogNet, Log, TEXT("Paused"));
	UGameplayStatics::SetGamePaused(GetWorld(), !UGameplayStatics::IsGamePaused(GetWorld()));

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);

	//msg.add_id(ID);
	msg.add_type(UGameplayStatics::IsGamePaused(GetWorld()) ?
                                              UnrealCoojaMsg::MsgType_PAUSE :
                                              UnrealCoojaMsg::MsgType_RESUME);
	//msg.add_pir()
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);

}

void AArdanPlayerController::speedSlow() {
  UE_LOG(LogNet, Log, TEXT("Speed at 0.1"));
  UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.1);

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);
	msg.add_type(slow ? UnrealCoojaMsg::MsgType_SPEED_NORM :
                      UnrealCoojaMsg::MsgType_SPEED_SLOW);
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);
  slow = !slow;
}

void AArdanPlayerController::speedNormal() {
  UE_LOG(LogNet, Log, TEXT("Speed at 1.0"));
  UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0);

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);
	msg.add_type(UnrealCoojaMsg::MsgType_SPEED_NORM);
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);
}

void AArdanPlayerController::speedFast() {
  UE_LOG(LogNet, Log, TEXT("Speed at 2.0"));
  UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 5.0);

  fbb.Clear();
	UnrealCoojaMsg::MessageBuilder msg(fbb);
	msg.add_type(UnrealCoojaMsg::MsgType_SPEED_FAST);
	auto mloc = msg.Finish();
	fbb.Finish(mloc);

	int sent = 0;
	bool successful = socket->SendTo(fbb.GetBufferPointer(), fbb.GetSize(),
										 sent, *addr);
}

void AArdanPlayerController::ScrollUp() {
	AArdanCharacter* const Pawn = (AArdanCharacter *) GetPawn();

		if (Pawn){
			FRotator rot = Pawn->GetCameraBoom()->RelativeRotation;
			rot.Add(5, 0, 0);
			Pawn->GetCameraBoom()->SetWorldRotation(rot);

		}
}

void AArdanPlayerController::ScrollDown() {
	AArdanCharacter* const Pawn = (AArdanCharacter *) GetPawn();

		if (Pawn){
			FRotator rot = Pawn->GetCameraBoom()->RelativeRotation;
			rot.Add(-5, 0, 0);
			Pawn->GetCameraBoom()->SetWorldRotation(rot);
		}
}


