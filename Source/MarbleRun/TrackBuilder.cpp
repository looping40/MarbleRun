#include "TrackBuilder.h"
#include "MarbleColor.h"
#include "MarbleHazards.h"
#include "MarblePawn.h"
#include "MarbleTriggers.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATrackBuilder::ATrackBuilder()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMaterial(TEXT("/Game/Materials/M_TrackGrid.M_TrackGrid"));
	if (GridMaterial.Succeeded())
	{
		FloorMaterial = GridMaterial.Object;
	}

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakeISM = [this](const TCHAR* Name, bool bCollides) -> UInstancedStaticMeshComponent*
	{
		UInstancedStaticMeshComponent* ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		ISM->SetupAttachment(Root);
		if (CubeMesh.Succeeded())
		{
			ISM->SetStaticMesh(CubeMesh.Object);
		}
		if (bCollides)
		{
			ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
		else
		{
			ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ISM->SetCastShadow(false);
		}
		return ISM;
	};
	FloorA = MakeISM(TEXT("FloorA"), true);
	FloorB = MakeISM(TEXT("FloorB"), true);
	Rails = MakeISM(TEXT("Rails"), true);
	Terrain = MakeISM(TEXT("Terrain"), true);
	Arrows = MakeISM(TEXT("Arrows"), false);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(FRotator(-50.f, -30.f, 0.f));

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogDensity(0.01f);
	Fog->SetFogHeightFalloff(0.05f);

	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(Root);
	if (CubeMesh.Succeeded())
	{
		Backdrop->SetStaticMesh(CubeMesh.Object);
	}
	Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backdrop->SetGenerateOverlapEvents(false);
	Backdrop->SetCastShadow(false);

	// Default course: start pad, descent, checkpoint, narrow bridge, sweeper, drop, moving platform, finish.
	Segments = {
		MakeSegment(ETrackPiece::Flat,           1000.f, 900.f),
		MakeSegment(ETrackPiece::Ramp,           1600.f, 500.f, 400.f),
		MakeSegment(ETrackPiece::Flat,            600.f, 500.f, 0.f, 90.f),
		MakeSegment(ETrackPiece::Checkpoint,      500.f, 600.f),
		MakeSegment(ETrackPiece::Narrow,         1200.f, 170.f, 0.f, 0.f, false),
		MakeSegment(ETrackPiece::Flat,            600.f, 600.f, 0.f, -90.f),
		MakeSegment(ETrackPiece::Sweeper,        1200.f, 900.f),
		MakeSegment(ETrackPiece::Ramp,           1200.f, 400.f, 450.f, 0.f, false),
		MakeSegment(ETrackPiece::Gap,             250.f, 400.f, 250.f),
		MakeSegment(ETrackPiece::Checkpoint,      700.f, 600.f, 0.f, 90.f),
		MakeSegment(ETrackPiece::MovingPlatform, 1600.f, 400.f),
		MakeSegment(ETrackPiece::Ramp,           1000.f, 600.f, 300.f),
		MakeSegment(ETrackPiece::Goal,            800.f, 900.f),
	};
}

FTrackSegment ATrackBuilder::MakeSegment(ETrackPiece Piece, float Length, float Width, float Drop, float TurnAfter, bool bRails)
{
	FTrackSegment S;
	S.Piece = Piece;
	S.Length = Length;
	S.Width = Width;
	S.Drop = Drop;
	S.TurnAfter = TurnAfter;
	S.bRails = bRails;
	return S;
}

// --- Lifecycle ----------------------------------------------------------------

void ATrackBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const TArray<USceneComponent*> Environment = { Sun, SkyLight, Atmosphere, Fog, Backdrop };
	for (USceneComponent* Comp : Environment)
	{
		Comp->SetVisibility(bAddLighting);
	}

	// Live geometry preview in the editor; gameplay actors are generated on demand or at runtime.
	BuildGeometry();
	ApplyColors();
}

#if WITH_EDITOR
void ATrackBuilder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (bGenerateNow)
	{
		bGenerateNow = false;
		GenerateGameplayActors();
	}
}
#endif

void ATrackBuilder::BeginPlay()
{
	Super::BeginPlay();

	if (!bBuiltForPlay)
	{
		BuildTrack(true);
	}
	ApplyColors();
}

void ATrackBuilder::ApplyColors()
{
	MarbleColor::Apply(FloorA, FloorColorA, FloorMaterial);
	MarbleColor::Apply(FloorB, FloorColorB, FloorMaterial);
	MarbleColor::Apply(Rails, RailColor);
	MarbleColor::Apply(Terrain, TerrainColor);
	MarbleColor::Apply(Arrows, ArrowColor);
	MarbleColor::Apply(Backdrop, BackdropColor);
}

FVector ATrackBuilder::GetStartLocation() const
{
	return GetActorTransform().TransformPosition(StartLocal);
}

// --- Geometry -----------------------------------------------------------------

void ATrackBuilder::AddBox(UInstancedStaticMeshComponent* Mesh, const FVector& Center, const FRotator& Rotation, const FVector& Size)
{
	// Engine cube is 100 cm per side with a centred pivot.
	Mesh->AddInstance(FTransform(Rotation, Center, Size / 100.f));
	LocalBounds += FBox::BuildAABB(Center, Size * 0.5f);
}

void ATrackBuilder::AddArrow(const FVector& Surface, const FRotator& Rotation, float Width)
{
	const float S = FMath::Clamp(Width / 600.f, 0.45f, 1.3f);
	const FQuat Base = Rotation.Quaternion();
	const FVector Up = Base.GetUpVector();
	const float Thick = 2.f;

	auto Piece = [&](const FVector& LocalCenter, float LocalYaw, const FVector& Size)
	{
		const FQuat Q = Base * FRotator(0.f, LocalYaw, 0.f).Quaternion();
		const FVector Center = Surface + Base.RotateVector(LocalCenter * S) + Up * (Thick * 0.5f + 0.5f);
		Arrows->AddInstance(FTransform(Q, Center, FVector(Size.X * S, Size.Y * S, Thick) / 100.f));
	};

	const float Bar = 26.f;
	Piece(FVector(-35.f, 0.f, 0.f), 0.f, FVector(150.f, Bar, 0.f));       // Shaft
	Piece(FVector(18.f, 26.f, 0.f), 135.f, FVector(95.f, Bar, 0.f));      // Head, right
	Piece(FVector(18.f, -26.f, 0.f), -135.f, FVector(95.f, Bar, 0.f));    // Head, left
}

void ATrackBuilder::BuildGeometry()
{
	for (UInstancedStaticMeshComponent* ISM : { FloorA.Get(), FloorB.Get(), Rails.Get(), Terrain.Get(), Arrows.Get() })
	{
		ISM->ClearInstances();
	}
	PieceSpecs.Reset();
	LocalBounds = FBox(ForceInit);

	struct FColumn
	{
		FVector TopCenter; // Bottom of the floor slab
		float Yaw;
		FVector2D Size;
	};
	TArray<FColumn> Columns;

	FVector Pos = FVector::ZeroVector; // Top surface at the start of the current segment.
	float Yaw = 0.f;

	if (Segments.Num() > 0)
	{
		StartLocal = FVector(FMath::Min(300.f, Segments[0].Length * 0.5f), 0.f, AMarblePawn::BallRadius + 30.f);
	}

	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		const FTrackSegment& S = Segments[Index];
		const FVector Dir = FRotator(0.f, Yaw, 0.f).Vector();
		const bool bDrops = S.Piece == ETrackPiece::Ramp || S.Piece == ETrackPiece::Gap;
		const float Drop = bDrops ? S.Drop : 0.f;
		const FVector End = Pos + Dir * S.Length - FVector(0.f, 0.f, Drop);
		const FVector Mid = (Pos + End) * 0.5f;
		const FRotator FlatRot(0.f, Yaw, 0.f);
		UInstancedStaticMeshComponent* FloorMesh = (Index % 2 == 0) ? FloorA : FloorB;

		const bool bHasFloor = S.Piece != ETrackPiece::Gap && S.Piece != ETrackPiece::MovingPlatform;
		if (bHasFloor)
		{
			const float SlopeLength = FVector::Dist(Pos, End);
			const FRotator Rot(FMath::RadiansToDegrees(FMath::Atan2(-Drop, S.Length)), Yaw, 0.f);
			const FVector Up = Rot.RotateVector(FVector::UpVector);
			const FVector Along = Rot.RotateVector(FVector::ForwardVector);
			const FVector Side = Rot.RotateVector(FVector::RightVector);

			AddBox(FloorMesh, Mid - Up * FloorThickness * 0.5f, Rot, FVector(SlopeLength, S.Width, FloorThickness));

			// Terrain columns under the floor; ramps are sliced so the flanks follow the slope.
			const int32 Slices = Drop > 0.f ? FMath::Max(FMath::CeilToInt(S.Length / 400.f), FMath::CeilToInt(Drop / 40.f)) : 1;
			for (int32 Slice = 0; Slice < Slices; ++Slice)
			{
				const float T = (Slice + 0.5f) / Slices;
				FVector Top = Pos + Dir * S.Length * T;
				Top.Z = Pos.Z - Drop * (Slice + 1.f) / Slices - FloorThickness;
				Columns.Add({ Top, Yaw, FVector2D(S.Length / Slices, S.Width) });
			}

			if (S.bRails && S.Piece != ETrackPiece::Narrow)
			{
				// Leave corners open so rails never cut across the neighbouring segment.
				const bool bTurnBefore = Index > 0 && !FMath::IsNearlyZero(Segments[Index - 1].TurnAfter);
				const bool bTurnAfter = Index + 1 < Segments.Num() && !FMath::IsNearlyZero(S.TurnAfter);
				const float InsetStart = bTurnBefore ? Segments[Index - 1].Width * 0.5f + RailThickness : 0.f;
				const float InsetEnd = bTurnAfter ? Segments[Index + 1].Width * 0.5f + RailThickness : 0.f;
				const float RailLength = SlopeLength - InsetStart - InsetEnd;

				if (RailLength > 50.f)
				{
					const FVector RailMid = Mid + Along * (InsetStart - InsetEnd) * 0.5f + Up * RailHeight * 0.5f;
					const float Offset = S.Width * 0.5f + RailThickness * 0.5f;
					AddBox(Rails, RailMid + Side * Offset, Rot, FVector(RailLength, RailThickness, RailHeight));
					AddBox(Rails, RailMid - Side * Offset, Rot, FVector(RailLength, RailThickness, RailHeight));
				}
			}

			const bool bArrowPiece = S.Piece == ETrackPiece::Flat || S.Piece == ETrackPiece::Ramp || S.Piece == ETrackPiece::Narrow;
			if (bPaintArrows && bArrowPiece)
			{
				AddArrow(Mid, Rot, S.Width);
			}
		}
		else
		{
			LocalBounds += Pos;
			LocalBounds += End;
		}

		// Square pad at a corner fills the outside of the turn.
		if (!FMath::IsNearlyZero(S.TurnAfter) && bHasFloor)
		{
			AddBox(FloorMesh, End - FVector(0.f, 0.f, FloorThickness * 0.5f), FlatRot, FVector(S.Width, S.Width, FloorThickness));
			Columns.Add({ End - FVector(0.f, 0.f, FloorThickness), Yaw, FVector2D(S.Width, S.Width) });
			if (bPaintArrows)
			{
				AddArrow(End, FRotator(0.f, Yaw + S.TurnAfter, 0.f), S.Width);
			}
		}

		switch (S.Piece)
		{
		case ETrackPiece::Checkpoint:
		case ETrackPiece::Goal:
		case ETrackPiece::Sweeper:
		case ETrackPiece::MovingPlatform:
			PieceSpecs.Add({ S.Piece, Mid, Yaw, S.Length, S.Width });
			break;
		default:
			break;
		}

		Pos = End;
		Yaw += S.TurnAfter;
	}

	// Solid terrain: every column goes down to a common base below the lowest floor.
	if (TerrainDepth > 0.f && Columns.Num() > 0)
	{
		const float BaseZ = LocalBounds.Min.Z - TerrainDepth;
		for (const FColumn& C : Columns)
		{
			const float Height = C.TopCenter.Z - BaseZ;
			if (Height > 1.f)
			{
				AddBox(Terrain, FVector(C.TopCenter.X, C.TopCenter.Y, BaseZ + Height * 0.5f), FRotator(0.f, C.Yaw, 0.f),
					FVector(C.Size.X, C.Size.Y, Height));
			}
		}
	}

	if (Segments.Num() > 0)
	{
		// Backdrop: a thin slab 1 km wider than the course on every side, BackdropDepth below its lowest point.
		const FVector Size = LocalBounds.GetSize() + FVector(200000.f, 200000.f, 0.f);
		const FVector Center = LocalBounds.GetCenter();
		Backdrop->SetRelativeLocation(FVector(Center.X, Center.Y, LocalBounds.Min.Z - BackdropDepth - 50.f));
		Backdrop->SetRelativeScale3D(FVector(Size.X / 100.f, Size.Y / 100.f, 1.f));
		Fog->SetRelativeLocation(FVector(Center.X, Center.Y, LocalBounds.Min.Z - BackdropDepth));
	}
}

// --- Gameplay actors ----------------------------------------------------------

AActor* ATrackBuilder::SpawnPiece(UClass* Class, const FVector& LocalLocation, const FRotator& LocalRotation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform WorldTransform = FTransform(LocalRotation, LocalLocation) * GetActorTransform();
	return World->SpawnActor<AActor>(Class, WorldTransform, Params);
}

void ATrackBuilder::SpawnGameplay(TArray<TObjectPtr<AActor>>& OutActors, bool bForEditor)
{
	auto Register = [&](AActor* Actor, const TCHAR* Label)
	{
		if (!Actor)
		{
			return;
		}
		OutActors.Add(Actor);
#if WITH_EDITOR
		if (bForEditor)
		{
			Actor->SetActorLabel(FString::Printf(TEXT("%s_%02d"), Label, OutActors.Num()));
			Actor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		}
#endif
	};

	for (const FPieceSpec& Spec : PieceSpecs)
	{
		const FRotator Rot(0.f, Spec.Yaw, 0.f);
		switch (Spec.Piece)
		{
		case ETrackPiece::Checkpoint:
			if (ACheckpoint* Checkpoint = Cast<ACheckpoint>(SpawnPiece(ACheckpoint::StaticClass(), Spec.Location, Rot)))
			{
				Checkpoint->ConfigureGate(Spec.Length, Spec.Width);
				Register(Checkpoint, TEXT("Checkpoint"));
			}
			break;

		case ETrackPiece::Goal:
			if (AGoalZone* Goal = Cast<AGoalZone>(SpawnPiece(AGoalZone::StaticClass(), Spec.Location, Rot)))
			{
				Goal->ConfigureGate(Spec.Length, Spec.Width);
				Register(Goal, TEXT("Goal"));
			}
			break;

		case ETrackPiece::Sweeper:
			if (ASweeperHazard* Sweeper = Cast<ASweeperHazard>(SpawnPiece(ASweeperHazard::StaticClass(), Spec.Location, Rot)))
			{
				Sweeper->Configure(Spec.Width * 0.85f);
				Register(Sweeper, TEXT("Sweeper"));
			}
			break;

		case ETrackPiece::MovingPlatform:
			if (AMovingPlatform* Platform = Cast<AMovingPlatform>(SpawnPiece(AMovingPlatform::StaticClass(), Spec.Location, Rot)))
			{
				const float SlabLength = FMath::Min(400.f, Spec.Length * 0.5f);
				const float Travel = Spec.Length * 0.5f - SlabLength * 0.5f;
				const float Z = -FloorThickness * 0.5f;
				Platform->Configure(FVector(-Travel, 0.f, Z), FVector(Travel, 0.f, Z), FVector(SlabLength, Spec.Width, FloorThickness));
				Register(Platform, TEXT("MovingPlatform"));
			}
			break;

		default:
			break;
		}
	}

	if (Segments.Num() > 0 && LocalBounds.GetVolume() > 0.f)
	{
		// Catch-all volume just below the terrain base.
		const FVector Extent = LocalBounds.GetExtent() + FVector(3000.f, 3000.f, 0.f);
		FVector Center = LocalBounds.GetCenter();
		Center.Z = LocalBounds.Min.Z - 700.f;
		if (AKillZone* KillZone = Cast<AKillZone>(SpawnPiece(AKillZone::StaticClass(), Center, FRotator::ZeroRotator)))
		{
			KillZone->SetTriggerBox(FVector::ZeroVector, FVector(Extent.X, Extent.Y, 600.f));
			Register(KillZone, TEXT("KillZone"));
		}
	}
}

bool ATrackBuilder::HasPlacedActors() const
{
	for (const AActor* Actor : PlacedActors)
	{
		if (IsValid(Actor))
		{
			return true;
		}
	}
	return false;
}

void ATrackBuilder::GenerateGameplayActors()
{
	ClearGameplayActors();
	BuildGeometry();
	SpawnGameplay(PlacedActors, /*bForEditor*/ true);
	Modify();
	MarkPackageDirty();
}

void ATrackBuilder::ClearGameplayActors()
{
	for (AActor* Actor : PlacedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	PlacedActors.Reset();
	Modify();
	MarkPackageDirty();
}

void ATrackBuilder::BuildTrack(bool bSpawnGameplay)
{
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Reset();

	BuildGeometry();

	// Actors generated in the editor are used as-is; otherwise create them for this play session.
	if (bSpawnGameplay && !HasPlacedActors())
	{
		SpawnGameplay(SpawnedActors, /*bForEditor*/ false);
	}

	bBuiltForPlay = bSpawnGameplay;
}
