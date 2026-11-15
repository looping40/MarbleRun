#include "MarbleHazards.h"
#include "MarbleColor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

// --- ASweeperHazard -----------------------------------------------------------

ASweeperHazard::ASweeperHazard()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Pole = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pole"));
	Pole->SetupAttachment(Root);
	if (CylinderMesh.Succeeded())
	{
		Pole->SetStaticMesh(CylinderMesh.Object);
	}
	Pole->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Pole->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	Pole->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.f));

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(Root);

	Arm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arm"));
	Arm->SetupAttachment(Pivot);
	if (CubeMesh.Succeeded())
	{
		Arm->SetStaticMesh(CubeMesh.Object);
	}
	Arm->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Arm->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Arm->SetRelativeScale3D(FVector(6.f, 0.3f, 0.4f));
}

void ASweeperHazard::Configure(float ArmLength)
{
	Arm->SetRelativeScale3D(FVector(ArmLength / 100.f, 0.3f, 0.4f));
}

void ASweeperHazard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColors();
}

void ASweeperHazard::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
}

void ASweeperHazard::ApplyColors()
{
	MarbleColor::Apply(Pole, FLinearColor(0.1f, 0.1f, 0.12f));
	MarbleColor::Apply(Arm, FLinearColor(0.9f, 0.08f, 0.05f));
}

void ASweeperHazard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Non-teleport move so the kinematic arm transfers its velocity to the marble.
	Pivot->AddLocalRotation(FRotator(0.f, DegreesPerSecond * DeltaSeconds, 0.f), false, nullptr, ETeleportType::None);
}

// --- AMovingPlatform ----------------------------------------------------------

AMovingPlatform::AMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	Platform->SetupAttachment(Root);
	if (CubeMesh.Succeeded())
	{
		Platform->SetStaticMesh(CubeMesh.Object);
	}
	Platform->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

void AMovingPlatform::Configure(const FVector& LocalStart, const FVector& LocalEnd, const FVector& Size)
{
	StartPoint = LocalStart;
	EndPoint = LocalEnd;
	Time = 0.f;
	Platform->SetRelativeScale3D(Size / 100.f);
	Platform->SetRelativeLocation(StartPoint, false, nullptr, ETeleportType::TeleportPhysics);
}

void AMovingPlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Platform->SetRelativeLocation(StartPoint);
	ApplyColors();
}

void AMovingPlatform::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
}

void AMovingPlatform::ApplyColors()
{
	MarbleColor::Apply(Platform, FLinearColor(0.55f, 0.2f, 0.9f));
}

void AMovingPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Time += DeltaSeconds;
	const float Alpha = 0.5f - 0.5f * FMath::Cos(2.f * PI * Time / CycleSeconds);
	// Relative move without teleport so the kinematic slab carries the marble.
	Platform->SetRelativeLocation(FMath::Lerp(StartPoint, EndPoint, Alpha), false, nullptr, ETeleportType::None);
}
