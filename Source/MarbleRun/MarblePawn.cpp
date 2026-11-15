#include "MarblePawn.h"
#include "MarbleColor.h"
#include "MarbleGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

AMarblePawn::AMarblePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultBallMaterial(TEXT("/Game/Materials/M_Ball.M_Ball"));

	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	if (SphereMesh.Succeeded())
	{
		Ball->SetStaticMesh(SphereMesh.Object);
	}
	if (DefaultBallMaterial.Succeeded())
	{
		BallMaterial = DefaultBallMaterial.Object;
		Ball->SetMaterial(0, BallMaterial);
	}
	Ball->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Ball->SetSimulatePhysics(true);
	Ball->SetNotifyRigidBodyCollision(true);
	Ball->SetGenerateOverlapEvents(true);
	Ball->BodyInstance.bUseCCD = true;
	Ball->SetLinearDamping(NormalLinearDamping);
	Ball->SetAngularDamping(NormalAngularDamping);
	Ball->SetMassOverrideInKg(NAME_None, 20.f, true);
	RootComponent = Ball;

	// Fixed isometric view: the arm follows the ball's position but never its spin.
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(Ball);
	CameraArm->SetUsingAbsoluteRotation(true);
	CameraArm->SetRelativeRotation(FRotator(-40.f, 45.f, 0.f));
	CameraArm->TargetArmLength = 3400.f;
	CameraArm->bDoCollisionTest = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 5.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	Camera->SetFieldOfView(30.f);
}

void AMarblePawn::BeginPlay()
{
	Super::BeginPlay();

	if (BallMaterial)
	{
		Ball->SetMaterial(0, BallMaterial);
	}
	else
	{
		MarbleColor::Apply(Ball, BallColor);
	}

	ApplyWeight();
}

void AMarblePawn::ApplyWeight()
{
	// Low bounce, good grip. "Min" combine keeps the bounce low whatever the marble hits.
	MarblePhysMaterial = NewObject<UPhysicalMaterial>(this, TEXT("MarblePhysMaterial"));
	MarblePhysMaterial->Restitution = Bounciness;
	MarblePhysMaterial->bOverrideRestitutionCombineMode = true;
	MarblePhysMaterial->RestitutionCombineMode = EFrictionCombineMode::Min;
	MarblePhysMaterial->Friction = SurfaceFriction;
	Ball->SetPhysMaterialOverride(MarblePhysMaterial);
}

FVector2D AMarblePawn::ReadMoveInput() const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D In = FVector2D::ZeroVector;
	if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up))    { In.Y += 1.f; }
	if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down))  { In.Y -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) { In.X += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left))  { In.X -= 1.f; }
	In.X += PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	In.Y += PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

	return In.GetClampedToMaxSize(1.f);
}

void AMarblePawn::SetBraking(bool bNewBraking)
{
	if (bNewBraking == bBraking)
	{
		return;
	}
	bBraking = bNewBraking;
	Ball->SetLinearDamping(bBraking ? BrakeDamping : NormalLinearDamping);
	Ball->SetAngularDamping(bBraking ? BrakeDamping : NormalAngularDamping);
}

void AMarblePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RespawnGrace = FMath::Max(0.f, RespawnGrace - DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	AMarbleGameMode* GM = GetWorld()->GetAuthGameMode<AMarbleGameMode>();

	if (PC && GM && (PC->WasInputKeyJustPressed(EKeys::R) || PC->WasInputKeyJustPressed(EKeys::Gamepad_Special_Right)))
	{
		GM->RestartRun();
	}

	const bool bCanDrive = !GM || GM->IsPlaying();
	const FVector2D In = bCanDrive ? ReadMoveInput() : FVector2D::ZeroVector;

	// Screen-relative directions derived from the camera yaw.
	const FRotator YawRot(0.f, CameraArm->GetComponentRotation().Yaw, 0.f);
	const FVector ScreenUp = YawRot.RotateVector(FVector::ForwardVector);
	const FVector ScreenRight = YawRot.RotateVector(FVector::RightVector);
	const FVector Dir = ScreenUp * In.Y + ScreenRight * In.X;

	if (!Dir.IsNearlyZero())
	{
		const FVector Vel = Ball->GetPhysicsLinearVelocity();
		const float SpeedAlong = FVector::DotProduct(FVector(Vel.X, Vel.Y, 0.f), Dir.GetSafeNormal());
		const float Limit = FMath::Clamp(1.f - SpeedAlong / MaxDriveSpeed, 0.f, 1.f);
		const float Control = IsGrounded() ? 1.f : AirControl;
		Ball->AddForce(Dir * RollAcceleration * Limit * Control, NAME_None, /*bAccelChange*/ true);
	}

	const bool bBrakeHeld = bCanDrive && PC &&
		(PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom));
	SetBraking(bBrakeHeld || bFinished);

	// Extra gravity on the marble only (mass-independent, like world gravity).
	if (!FMath::IsNearlyEqual(GravityScale, 1.f))
	{
		Ball->AddForce(FVector(0.f, 0.f, GetWorld()->GetGravityZ() * (GravityScale - 1.f)), NAME_None, /*bAccelChange*/ true);
	}

	// Hits may push the marble sideways but must not launch it like a balloon.
	FVector Velocity = Ball->GetPhysicsLinearVelocity();
	if (Velocity.Z > MaxUpwardSpeed)
	{
		Velocity.Z = MaxUpwardSpeed;
		Ball->SetPhysicsLinearVelocity(Velocity);
	}

	PrevVelocity = Velocity;
}

void AMarblePawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
	FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (RespawnGrace > 0.f)
	{
		return;
	}

	// A hard landing on a floor-like surface shatters the marble.
	if (HitNormal.Z > 0.6f && -PrevVelocity.Z > ShatterFallSpeed)
	{
		if (AMarbleGameMode* GM = GetWorld()->GetAuthGameMode<AMarbleGameMode>())
		{
			GM->OnMarbleLost(this, TEXT("SHATTERED!"));
		}
	}
}

void AMarblePawn::ResetTo(const FVector& Location)
{
	SetActorLocationAndRotation(Location, FRotator::ZeroRotator, false, nullptr, ETeleportType::ResetPhysics);
	Ball->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Ball->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PrevVelocity = FVector::ZeroVector;
	RespawnGrace = 0.5f;
	bFinished = false;
}

float AMarblePawn::GetSpeed() const
{
	return Ball->GetPhysicsLinearVelocity().Size();
}

bool AMarblePawn::IsGrounded() const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MarbleGround), false, this);
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, BallRadius + 15.f);
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}
