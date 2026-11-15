#include "MarbleTriggers.h"
#include "MarbleColor.h"
#include "MarbleGameMode.h"
#include "MarblePawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor CheckpointIdle(0.1f, 0.8f, 1.f);
	const FLinearColor CheckpointDone(0.15f, 1.f, 0.2f);
	const FLinearColor GoalColor(1.f, 0.75f, 0.05f);
	constexpr float GateHeight = 300.f;
}

// --- AMarbleTrigger -----------------------------------------------------------

AMarbleTrigger::AMarbleTrigger()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(Root);
	Box->SetCollisionProfileName(TEXT("Trigger"));
	Box->SetGenerateOverlapEvents(true);
	Box->SetBoxExtent(FVector(200.f));
	Box->OnComponentBeginOverlap.AddDynamic(this, &AMarbleTrigger::HandleOverlap);
}

void AMarbleTrigger::SetTriggerBox(const FVector& RelativeCenter, const FVector& Extent)
{
	Box->SetRelativeLocation(RelativeCenter);
	Box->SetBoxExtent(Extent);
}

AMarbleGameMode* AMarbleTrigger::GetMarbleGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<AMarbleGameMode>() : nullptr;
}

void AMarbleTrigger::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AMarblePawn* Marble = Cast<AMarblePawn>(OtherActor))
	{
		OnMarbleEnter(Marble);
	}
}

// --- AKillZone ----------------------------------------------------------------

void AKillZone::OnMarbleEnter(AMarblePawn* Marble)
{
	if (AMarbleGameMode* GM = GetMarbleGameMode())
	{
		GM->OnMarbleLost(Marble, TEXT("FELL OFF!"));
	}
}

// --- AMarbleGate --------------------------------------------------------------

AMarbleGate::AMarbleGate()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	auto MakePart = [this](const TCHAR* Name) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		if (CubeMesh.Succeeded())
		{
			Part->SetStaticMesh(CubeMesh.Object);
		}
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		return Part;
	};

	Stripe = MakePart(TEXT("Stripe"));
	Stripe->SetCastShadow(false);
	PostLeft = MakePart(TEXT("PostLeft"));
	PostRight = MakePart(TEXT("PostRight"));
}

void AMarbleGate::ConfigureGate(float Length, float Width)
{
	SetTriggerBox(FVector(0.f, 0.f, GateHeight * 0.5f), FVector(Length * 0.5f, Width * 0.5f, GateHeight * 0.5f));

	Stripe->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
	Stripe->SetRelativeScale3D(FVector(0.4f, Width / 100.f, 0.02f));

	const float PostOffset = Width * 0.5f + 20.f;
	PostLeft->SetRelativeLocation(FVector(0.f, -PostOffset, GateHeight * 0.5f));
	PostRight->SetRelativeLocation(FVector(0.f, PostOffset, GateHeight * 0.5f));
	PostLeft->SetRelativeScale3D(FVector(0.2f, 0.2f, GateHeight / 100.f));
	PostRight->SetRelativeScale3D(FVector(0.2f, 0.2f, GateHeight / 100.f));
}

void AMarbleGate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyGateColors();
}

void AMarbleGate::BeginPlay()
{
	Super::BeginPlay();
	ApplyGateColors();
}

void AMarbleGate::ApplyGateColors()
{
	GateMaterials.Reset();
	for (UStaticMeshComponent* Part : { Stripe.Get(), PostLeft.Get(), PostRight.Get() })
	{
		if (UMaterialInstanceDynamic* MID = MarbleColor::Apply(Part, GateColor))
		{
			GateMaterials.Add(MID);
		}
	}
}

void AMarbleGate::SetGateColor(const FLinearColor& Color)
{
	for (UMaterialInstanceDynamic* MID : GateMaterials)
	{
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}

// --- ACheckpoint --------------------------------------------------------------

ACheckpoint::ACheckpoint()
{
	GateColor = CheckpointIdle;
}

FVector ACheckpoint::GetRespawnLocation() const
{
	return GetActorLocation() + GetActorUpVector() * (AMarblePawn::BallRadius + 30.f);
}

void ACheckpoint::SetReached(bool bReached)
{
	SetGateColor(bReached ? CheckpointDone : GateColor);
}

void ACheckpoint::OnMarbleEnter(AMarblePawn* Marble)
{
	if (AMarbleGameMode* GM = GetMarbleGameMode())
	{
		GM->OnCheckpointReached(this);
	}
}

// --- AGoalZone ----------------------------------------------------------------

AGoalZone::AGoalZone()
{
	GateColor = GoalColor;
}

void AGoalZone::OnMarbleEnter(AMarblePawn* Marble)
{
	if (AMarbleGameMode* GM = GetMarbleGameMode())
	{
		GM->OnGoalReached(Marble);
	}
}
