#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarbleHazards.generated.h"

class UStaticMeshComponent;

/** A bar spinning around a central pole that knocks the marble around. Origin sits on the floor surface. */
UCLASS()
class MARBLERUN_API ASweeperHazard : public AActor
{
	GENERATED_BODY()

public:
	ASweeperHazard();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void Configure(float ArmLength);

protected:
	virtual void BeginPlay() override;
	void ApplyColors();

	UPROPERTY(VisibleAnywhere, Category = "Sweeper")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Sweeper")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, Category = "Sweeper")
	TObjectPtr<UStaticMeshComponent> Pole;

	UPROPERTY(VisibleAnywhere, Category = "Sweeper")
	TObjectPtr<UStaticMeshComponent> Arm;

	UPROPERTY(EditAnywhere, Category = "Sweeper")
	float DegreesPerSecond = 100.f;
};

/** A slab that shuttles back and forth between two points, carrying the marble across a gap. */
UCLASS()
class MARBLERUN_API AMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AMovingPlatform();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Start/End are slab-centre positions relative to the actor; Size is the slab size in cm. */
	void Configure(const FVector& LocalStart, const FVector& LocalEnd, const FVector& Size);

protected:
	virtual void BeginPlay() override;
	void ApplyColors();

	UPROPERTY(VisibleAnywhere, Category = "Platform")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Platform")
	TObjectPtr<UStaticMeshComponent> Platform;

	/** Seconds for a full there-and-back trip. */
	UPROPERTY(EditAnywhere, Category = "Platform", meta = (ClampMin = "1"))
	float CycleSeconds = 8.f;

	/** Slab-centre position at one end of the trip, relative to the actor (drag the widget in the viewport). */
	UPROPERTY(EditAnywhere, Category = "Platform", meta = (MakeEditWidget = true))
	FVector StartPoint = FVector(-400.f, 0.f, -30.f);

	/** Slab-centre position at the other end of the trip, relative to the actor. */
	UPROPERTY(EditAnywhere, Category = "Platform", meta = (MakeEditWidget = true))
	FVector EndPoint = FVector(400.f, 0.f, -30.f);

private:
	float Time = 0.f;
};
