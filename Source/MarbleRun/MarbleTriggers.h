#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarbleTriggers.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class AMarblePawn;
class AMarbleGameMode;

/** Box volume that reacts when the marble enters it. */
UCLASS(Abstract)
class MARBLERUN_API AMarbleTrigger : public AActor
{
	GENERATED_BODY()

public:
	AMarbleTrigger();

	/** Places the trigger box relative to the actor origin. */
	void SetTriggerBox(const FVector& RelativeCenter, const FVector& Extent);

protected:
	virtual void OnMarbleEnter(AMarblePawn* Marble) {}
	AMarbleGameMode* GetMarbleGameMode() const;

	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	TObjectPtr<UBoxComponent> Box;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

/** Large volume under the track: falling into it costs time and respawns the marble. */
UCLASS()
class MARBLERUN_API AKillZone : public AMarbleTrigger
{
	GENERATED_BODY()

protected:
	virtual void OnMarbleEnter(AMarblePawn* Marble) override;
};

/** Trigger with a visible stripe and two posts across the track. Origin sits on the floor surface. */
UCLASS(Abstract)
class MARBLERUN_API AMarbleGate : public AMarbleTrigger
{
	GENERATED_BODY()

public:
	AMarbleGate();

	void ConfigureGate(float Length, float Width);

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	void ApplyGateColors();
	void SetGateColor(const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> Stripe;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> PostLeft;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> PostRight;

	UPROPERTY(EditAnywhere, Category = "Gate")
	FLinearColor GateColor = FLinearColor::White;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GateMaterials;
};

UCLASS()
class MARBLERUN_API ACheckpoint : public AMarbleGate
{
	GENERATED_BODY()

public:
	ACheckpoint();

	/** The marble respawns just above the gate's origin (which sits on the floor). */
	FVector GetRespawnLocation() const;
	void SetReached(bool bReached);

protected:
	virtual void OnMarbleEnter(AMarblePawn* Marble) override;
};

UCLASS()
class MARBLERUN_API AGoalZone : public AMarbleGate
{
	GENERATED_BODY()

public:
	AGoalZone();

protected:
	virtual void OnMarbleEnter(AMarblePawn* Marble) override;
};
