// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Ardan.h"
#include "ArdanPlayerController.h"
#include "AI/Navigation/NavigationSystem.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ArdanSave.h"
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
	bRecording = true;
  conns = FRunnableConnection::create(5000, &packetQ);
  if (conns) {UE_LOG(LogNet, Log, TEXT("Runnable Connection created"));}
  else { UE_LOG(LogNet, Log, TEXT("conns failed"));}


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

	//actorManager = NewNamedObject<UActorManager>(this, FString("ActorManager"), RF_NoFlags, UActorManager::StaticClass()); 
	actorManager = NewObject<UActorManager>();
	actorManager->init(GetWorld(), this);
	actorManager->initHist();
	sensorManager = new SensorManager(GetWorld());
	sensorManager->FindSensors();

  UE_LOG(LogNet, Log, TEXT("--START--"));
}

void AArdanPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (conns) {
    conns->Stop();
    conns->Shutdown();
  }
	delete sensorManager;
}

void AArdanPlayerController::NewTimeline() {
	bRecording = true;
	bReplay = true;
	curTime = 0;
	replayTime = 0;
	actorManager->NewTimeline();
	sensorManager->NewTimeline();
}

void AArdanPlayerController::replayPressed() {
	bReplay = true;
	bRecording = false;
	curTime = 0;
	replayTime = 0;
	actorManager->ResetTimelines();
	sensorManager->ResetTimeline();
}


void AArdanPlayerController::PlayerTick(float DeltaTime) {
	Super::PlayerTick(DeltaTime);

	if (UGameplayStatics::IsGamePaused(GetWorld())) return;
	elapsed += DeltaTime;
	tenth += DeltaTime;
	rtick += DeltaTime;

	APawn* const Pawn = GetPawn();
  FVector sourceLoc = Pawn->GetActorLocation();
	FRotator sourceRot = Pawn->GetActorRotation();
  FTransform *transform = new FTransform();
  *transform = Pawn->GetTransform();
	/* Pop and set actors old location else push current location onto stack */

	if (bReverse) {
		replayTime -= DeltaTime;
		curTime -= DeltaTime;

		actorManager->rewindCurrentMeshActors(false, curTime);
		actorManager->rewindAllMeshActors(true, curTime);
		actorManager->rewindCurrentPawnActors();
		actorManager->rewindAllPawnActors();

		sensorManager->RewindState(curTime);
		return;
	}
	else {
		curTime += DeltaTime;
		if (bReplay) {
			replayTime += DeltaTime;
			//UE_LOG(LogNet, Log, TEXT("Replaying: %f"), replayTime);
			actorManager->replayCurrentActors(replayTime);
			actorManager->replayAllActors(replayTime);
			actorManager->replayCurrentPawnActors(replayTime);
			actorManager->replayAllPawnActors(replayTime);

			sensorManager->FastForwardState(replayTime);
		}
		if (bRecording && rtick >= 0.03) {
			rtick = 0;
			actorManager->recordActors(DeltaTime, curTime);
			actorManager->recordPawnActors(DeltaTime, curTime);
		}
	}
  
	//actorManager->diff(NULL);
	//sensorManager->DiffState(0, curTime);

	// /* Working out FPS */
	
	tickCount += 1;
	if (elapsed >= 1) {
		UE_LOG(LogNet, Log, TEXT("TSTAMP: %f %f %f %d "), DeltaTime, 1/DeltaTime,
					elapsed/tickCount,
					tickCount);
		elapsed = 0;
		tickCount = 0;
	}
	if (!(bReplay || bReverse) && tenth >= 0.1) {

		tenth = 0;
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
			timeSpheres.Push(ts);
		} else UE_LOG(LogNet, Log, TEXT("NOOOOOOOOOOOOOOO"));
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

void AArdanPlayerController::SaveArdanState() {
	UArdanSave* save = Cast<UArdanSave>(UGameplayStatics::CreateSaveGameObject(UArdanSave::StaticClass()));
	save->PlayerName = FString("ARDAN");
	save->currentPawnHistory = *(actorManager->currentPawnHistory);
	save->currentHistory = *(actorManager->currentHistory);
	save->currentHistory.histMap = actorManager->currentHistory->histMap;
	save->currentHistory.name = FString("HELLO");
	UE_LOG(LogNet, Log, TEXT("ARDAN Saved: %s (%d:%d)"), *save->SaveSlotName, 
		actorManager->currentHistory->histMap.Num(), 
		save->currentHistory.histMap.Num());
	UGameplayStatics::SaveGameToSlot(save, save->SaveSlotName, save->UserIndex);
}

void AArdanPlayerController::LoadArdanState() {
	UArdanSave* save = Cast<UArdanSave>(UGameplayStatics::CreateSaveGameObject(UArdanSave::StaticClass()));
	save = Cast<UArdanSave>(UGameplayStatics::LoadGameFromSlot(save->SaveSlotName, save->UserIndex));
	FString PlayerNameToDisplay = save->PlayerName;
	*actorManager->currentHistory = save->currentHistory;
	*actorManager->currentPawnHistory = save->currentPawnHistory;
	UE_LOG(LogNet, Log, TEXT("ARDAN LOADED: %s (%d)"), *save->currentHistory.name, save->currentHistory.histMap.Num());
	actorManager->FixReferences(actorManager->currentHistory);
	actorManager->FixPawnReferences(actorManager->currentPawnHistory);
	replayPressed();

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
	InputComponent->BindAction("NewTimeline", IE_Pressed, this, &AArdanPlayerController::NewTimeline);

	InputComponent->BindAction("Save", IE_Pressed, this, &AArdanPlayerController::SaveArdanState);
	InputComponent->BindAction("Load", IE_Pressed, this, &AArdanPlayerController::LoadArdanState);
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


