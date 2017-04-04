
#include "Ardan.h"
#include "TimeSphere.h"


// Sets default values
ATimeSphere::ATimeSphere(const FObjectInitializer &ObjectInitializer) :Super(ObjectInitializer)
{
    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;


    // Create and position a mesh component so we can see where our sphere is
    UStaticMeshComponent* SphereVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualRepresentation"));
    RootComponent = SphereVisual;

    // Our root component will be a sphere that reacts to physics
    USphereComponent* SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
    SphereComponent->InitSphereRadius(10.0f);
    // SphereComponent->SetCollisionProfileName(TEXT("TimeSphere"));
    SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // SphereComponent->SetWorldLocation(target);
    SphereComponent->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereVisualAsset(TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"));
    if (SphereVisualAsset.Succeeded())
    {
        SphereVisual->SetStaticMesh(SphereVisualAsset.Object);
        SphereVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
        SphereVisual->SetWorldScale3D(FVector(0.2f));
        SphereVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    }
    // Create a particle system that we can activate or deactivate
    OurParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("MovementParticles"));
    OurParticleSystem->SetupAttachment(SphereVisual);
    OurParticleSystem->bAutoActivate = false;
    // OurParticleSystem->SetWorldLocation(target);
    OurParticleSystem->SetRelativeLocation(FVector(-20.0f, 0.0f, 20.0f));
    static ConstructorHelpers::FObjectFinder<UParticleSystem> ParticleAsset(TEXT("/Game/StarterContent/Particles/P_Fire.P_Fire"));
    if (ParticleAsset.Succeeded())
    {
        OurParticleSystem->SetTemplate(ParticleAsset.Object);
    }

    // // Use a spring arm to give the camera smooth, natural-feeling motion.
    // USpringArmComponent* SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraAttachmentArm"));
    // SpringArm->SetupAttachment(RootComponent);
    // SpringArm->RelativeRotation = FRotator(-45.f, 0.f, 0.f);
    // SpringArm->TargetArmLength = 400.0f;
    // SpringArm->bEnableCameraLag = true;
    // SpringArm->CameraLagSpeed = 3.0f;
    //
    // // Create a camera and attach to our spring arm
    // UCameraComponent* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("ActualCamera"));
    // Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    //
    // // Take control of the default player
    // AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void ATimeSphere::BeginPlay()
{
    Super::BeginPlay();

}

// Called every frame
void ATimeSphere::Tick( float DeltaTime )
{
    Super::Tick( DeltaTime );

}

// // Called to bind functionality to input
// void ATimeSphere::SetupPlayerInputComponent(class UInputComponent* InputComponent)
// {
//     Super::SetupPlayerInputComponent(InputComponent);
//
// }
