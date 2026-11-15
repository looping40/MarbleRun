#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MarblePawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UMaterialInterface;

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

	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float NormalLinearDamping = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float NormalAngularDamping = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float BrakeDamping = 3.f;

	/** Landing with a downward speed (cm/s) above this shatters the marble. */
	UPROPERTY(EditAnywhere, Category = "Marble|Handling")
	float ShatterFallSpeed = 1800.f;

	/** Material applied to the ball. Defaults to /Game/NewMaterial; if empty, the ball is tinted with BallColor. */
	UPROPERTY(EditAnywhere, Category = "Marble|Look")
	TObjectPtr<UMaterialInterface> BallMaterial;

	UPROPERTY(EditAnywhere, Category = "Marble|Look")
	FLinearColor BallColor = FLinearColor(0.05f, 0.35f, 1.f);

private:
	FVector2D ReadMoveInput() const;
	void SetBraking(bool bNewBraking);

	FVector PrevVelocity = FVector::ZeroVector;
	float RespawnGrace = 0.f;
	bool bBraking = false;
};
