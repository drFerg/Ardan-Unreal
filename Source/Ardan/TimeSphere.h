
#pragma once

#include "GameFramework/Character.h"
#include "TimeSphere.generated.h"

UCLASS()
class ARDAN_API ATimeSphere : public ACharacter
{
    GENERATED_BODY()

public:
    // Sets default values for this pawn's properties
    ATimeSphere(const FObjectInitializer &ObjectInitializer);
    void init(FVector target);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick( float DeltaSeconds ) override;

    // Called to bind functionality to input
    /*virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;*/

    UParticleSystemComponent* OurParticleSystem;
};
