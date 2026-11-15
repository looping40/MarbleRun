#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrackBuilder.generated.h"

class UInstancedStaticMeshComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UExponentialHeightFogComponent;
class UStaticMeshComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ETrackPiece : uint8
{
	Flat,
	Ramp,           // Slopes down by Drop over Length
	Narrow,         // Flat, never has rails
	Gap,            // No floor; the next piece starts Drop lower
	Checkpoint,     // Flat with a checkpoint gate
	Sweeper,        // Flat with a spinning bar
	MovingPlatform, // No floor; a shuttling slab spans the Length
	Goal            // Flat with the finish gate
};

USTRUCT(BlueprintType)
struct FTrackSegment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Track")
	ETrackPiece Piece = ETrackPiece::Flat;

	UPROPERTY(EditAnywhere, Category = "Track", meta = (ClampMin = "50"))
	float Length = 800.f;

	UPROPERTY(EditAnywhere, Category = "Track", meta = (ClampMin = "50"))
	float Width = 600.f;

	/** Height lost over the piece (Ramp and Gap only). */
	UPROPERTY(EditAnywhere, Category = "Track")
	float Drop = 0.f;

	/** Heading change in degrees applied at the end of the piece. */
	UPROPERTY(EditAnywhere, Category = "Track")
	float TurnAfter = 0.f;

	UPROPERTY(EditAnywhere, Category = "Track")
	bool bRails = true;
};

/**
 * Builds a course from a list of segments, turtle-style: each segment starts where the previous one ended.
 * The geometry (floors, rails, terrain, arrows) is rebuilt live in the editor whenever a property changes.
 * Checkpoints, hazards, the goal and the kill zone can be generated once as regular level actors
 * ("Generate Gameplay Actors" button) and then moved by hand; if none were generated, they are spawned at runtime.
 */
UCLASS()
class MARBLERUN_API ATrackBuilder : public AActor
{
	GENERATED_BODY()

public:
	ATrackBuilder();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Rebuilds the geometry. With bSpawnGameplay, also spawns runtime gameplay actors unless placed ones exist. */
	void BuildTrack(bool bSpawnGameplay);

	/** World position where the marble starts. */
	FVector GetStartLocation() const;

	/** Replaces the placed checkpoints, hazards, goal and kill zone with freshly generated ones. */
	UFUNCTION(CallInEditor, Category = "Track")
	void GenerateGameplayActors();

	/** Deletes the placed gameplay actors (they will then be spawned automatically at runtime). */
	UFUNCTION(CallInEditor, Category = "Track")
	void ClearGameplayActors();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Track")
	TArray<FTrackSegment> Segments;

	UPROPERTY(EditAnywhere, Category = "Track")
	float FloorThickness = 60.f;

	UPROPERTY(EditAnywhere, Category = "Track")
	float RailHeight = 70.f;

	UPROPERTY(EditAnywhere, Category = "Track")
	float RailThickness = 30.f;

	/** How far the solid terrain extends below the lowest floor. 0 disables the terrain flanks. */
	UPROPERTY(EditAnywhere, Category = "Track", meta = (ClampMin = "0"))
	float TerrainDepth = 800.f;

	/** Paints direction arrows on the floor. */
	UPROPERTY(EditAnywhere, Category = "Track")
	bool bPaintArrows = true;

	/** Same as the Generate Gameplay Actors button; tick it (e.g. from a script) to regenerate. */
	UPROPERTY(EditAnywhere, Category = "Track", meta = (DisplayName = "Generate Gameplay Actors Now"))
	bool bGenerateNow = false;

	/** Gameplay actors generated in the editor and saved with the level. */
	UPROPERTY(VisibleAnywhere, Category = "Track")
	TArray<TObjectPtr<AActor>> PlacedActors;

	/** Floor material; needs a "Color" vector parameter. Defaults to /Game/Materials/M_TrackGrid. */
	UPROPERTY(EditAnywhere, Category = "Track|Look")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	UPROPERTY(EditAnywhere, Category = "Track|Look")
	FLinearColor FloorColorA = FLinearColor(0.22f, 0.26f, 0.34f);

	UPROPERTY(EditAnywhere, Category = "Track|Look")
	FLinearColor FloorColorB = FLinearColor(0.14f, 0.17f, 0.24f);

	UPROPERTY(EditAnywhere, Category = "Track|Look")
	FLinearColor RailColor = FLinearColor(0.95f, 0.4f, 0.05f);

	UPROPERTY(EditAnywhere, Category = "Track|Look")
	FLinearColor TerrainColor = FLinearColor(0.09f, 0.1f, 0.16f);

	UPROPERTY(EditAnywhere, Category = "Track|Look")
	FLinearColor ArrowColor = FLinearColor(0.8f, 0.95f, 1.f);

	/** Adds its own sun, sky, fog and backdrop so an empty level is lit. */
	UPROPERTY(EditAnywhere, Category = "Lighting")
	bool bAddLighting = true;

	/** Distance between the bottom of the terrain and the backdrop surface. */
	UPROPERTY(EditAnywhere, Category = "Lighting")
	float BackdropDepth = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Lighting")
	FLinearColor BackdropColor = FLinearColor(0.04f, 0.12f, 0.16f);

	UPROPERTY(VisibleAnywhere, Category = "Track")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Track")
	TObjectPtr<UInstancedStaticMeshComponent> FloorA;

	UPROPERTY(VisibleAnywhere, Category = "Track")
	TObjectPtr<UInstancedStaticMeshComponent> FloorB;

	UPROPERTY(VisibleAnywhere, Category = "Track")
	TObjectPtr<UInstancedStaticMeshComponent> Rails;

	UPROPERTY(VisibleAnywhere, Category = "Track")
	TObjectPtr<UInstancedStaticMeshComponent> Terrain;

	UPROPERTY(VisibleAnywhere, Category = "Track")
	TObjectPtr<UInstancedStaticMeshComponent> Arrows;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

	/** Huge non-colliding ground far below the course, so the downward camera never looks into the void. */
	UPROPERTY(VisibleAnywhere, Category = "Lighting")
	TObjectPtr<UStaticMeshComponent> Backdrop;

private:
	/** A gameplay actor to spawn, computed while building the geometry (local space). */
	struct FPieceSpec
	{
		ETrackPiece Piece = ETrackPiece::Flat;
		FVector Location = FVector::ZeroVector;
		float Yaw = 0.f;
		float Length = 0.f;
		float Width = 0.f;
	};

	static FTrackSegment MakeSegment(ETrackPiece Piece, float Length, float Width, float Drop = 0.f,
		float TurnAfter = 0.f, bool bRails = true);

	void BuildGeometry();
	void ApplyColors();
	void SpawnGameplay(TArray<TObjectPtr<AActor>>& OutActors, bool bForEditor);
	bool HasPlacedActors() const;

	void AddBox(UInstancedStaticMeshComponent* Mesh, const FVector& Center, const FRotator& Rotation, const FVector& Size);
	void AddArrow(const FVector& Surface, const FRotator& Rotation, float Width);

	AActor* SpawnPiece(UClass* Class, const FVector& LocalLocation, const FRotator& LocalRotation);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	TArray<FPieceSpec> PieceSpecs;
	FVector StartLocal = FVector(0.f, 0.f, 120.f);
	FBox LocalBounds = FBox(ForceInit);
	bool bBuiltForPlay = false;
};
