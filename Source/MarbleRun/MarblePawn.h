#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MarblePawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UMaterialInterface;
class UPhysicalMaterial;

/** Physics-driven marble viewed from a fixed isometric camera. Input is screen-relative. */
UCLASS()
class MARBLERUN_API AMarblePawn : public APawn
{
	GENERATED_BODY()

public:
	AMarblePawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
		FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Teleports the marble and kills all its motion. */
	void ResetTo(const FVector& Location);

	/** After the finish line the marble brakes to a stop on its own. */
	void SetFinished(bool bNewFinished) { bFinished = bNewFinished; }

	float GetSpeed() const;
	bool IsGrounded() const;

	static constexpr float BallRadius = 50.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Marble")
	TObjectPtr<UStaticMeshComponent> Ball;

	UPROPERTY(VisibleAnywhere, Category = "Marble")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Marble")
	TObjectPtr<UCameraComponent> Camera;

	/** Acceleration (cm/s^2) applied while steering on the ground. */
	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float RollAcceleration = 1800.f;

	/** Fraction of steering kept while airborne. */
	UPROPERTY(EditAnywhere, Category = "Marble|Handling", meta = (ClampMin = "0", ClampMax = "1"))
	float AirControl = 0.25f;

	/** Above this speed (cm/s) along the input direction, steering stops adding speed. */
	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float MaxDriveSpeed = 2200.f;

	/** Air drag. Kept very low: drag is what makes a falling ball look like a balloon. */
	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float NormalLinearDamping = 0.03f;

	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float NormalAngularDamping = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float BrakeDamping = 3.f;

	/** Landing with a downward speed (cm/s) above this shatters the marble. */
	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float ShatterFallSpeed = 1800.f;

	/** Multiplier on world gravity for the marble only. Higher = heavier, snappier falls and arcs. */
	UPROPERTY(EditAnywhere, Category = "Marble|Weight", meta = (ClampMin = "0.1"))
	float GravityScale = 2.f;

	/** Upward speed cap (cm/s): a hit can knock the marble sideways but never launch it into the air. */
	UPROPERTY(EditAnywhere, Category = "Marble|Weight", meta = (ClampMin = "0"))
	float MaxUpwardSpeed = 600.f;

	/** 0 = dead stop on impact, 1 = perfect bounce. A glass marble on hard floor barely bounces. */
	UPROPERTY(EditAnywhere, Category = "Marble|Weight", meta = (ClampMin = "0", ClampMax = "1"))
	float Bounciness = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Marble|Weight", meta = (ClampMin = "0"))
	float SurfaceFriction = 0.8f;

	/** Material applied to the ball. Defaults to /Game/Materials/M_Ball; if empty, the ball is tinted with BallColor. */
	UPROPERTY(EditAnywhere, Category = "Marble|Look")
	TObjectPtr<UMaterialInterface> BallMaterial;

	UPROPERTY(EditAnywhere, Category = "Marble|Look")
	FLinearColor BallColor = FLinearColor(0.05f, 0.35f, 1.f);

private:
	FVector2D ReadMoveInput() const;
	void ApplyWeight();

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> MarblePhysMaterial;
	void SetBraking(bool bNewBraking);

	FVector PrevVelocity = FVector::ZeroVector;
	float RespawnGrace = 0.f;
	bool bBraking = false;
	bool bFinished = false;
};
