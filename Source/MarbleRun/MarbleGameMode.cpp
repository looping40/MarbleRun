#include "MarbleGameMode.h"
#include "MarbleHUD.h"
#include "MarblePawn.h"
#include "MarbleTriggers.h"
#include "TrackBuilder.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AMarbleGameMode::AMarbleGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AMarblePawn::StaticClass();
	HUDClass = AMarbleHUD::StaticClass();
}

void AMarbleGameMode::EnsureTrack()
{
	if (Track)
	{
		return;
	}

	UWorld* World = GetWorld();
	for (TActorIterator<ATrackBuilder> It(World); It; ++It)
	{
		Track = *It;
		break;
	}

	if (!Track)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Track = World->SpawnActor<ATrackBuilder>(ATrackBuilder::StaticClass(), FTransform::Identity, Params);
	}

	if (Track)
	{
		Track->BuildTrack(/*bSpawnGameplay*/ true);
		RespawnLocation = Track->GetStartLocation();
	}
}

void AMarbleGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	// Spawn at the track start; the level does not need a PlayerStart.
	EnsureTrack();
	RestartPlayerAtTransform(NewPlayer, FTransform(FRotator::ZeroRotator, RespawnLocation));
}

void AMarbleGameMode::StartPlay()
{
	EnsureTrack();
	Super::StartPlay();
	ResetRunState();
	ShowMessage(TEXT("GO!"));
}

void AMarbleGameMode::ResetRunState()
{
	State = EMarbleRunState::Playing;
	TimeLeft = TimeLimit;
	Elapsed = 0.f;
	Falls = 0;

	for (ACheckpoint* Checkpoint : ReachedCheckpoints)
	{
		if (IsValid(Checkpoint))
		{
			Checkpoint->SetReached(false);
		}
	}
	ReachedCheckpoints.Reset();

	if (Track)
	{
		RespawnLocation = Track->GetStartLocation();
	}
}

void AMarbleGameMode::RestartRun()
{
	ResetRunState();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (AMarblePawn* Marble = Cast<AMarblePawn>(PC->GetPawn()))
			{
				Marble->ResetTo(RespawnLocation);
			}
		}
	}

	ShowMessage(TEXT("GO!"));
}

void AMarbleGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MessageTimeLeft -= DeltaSeconds;

	if (State != EMarbleRunState::Playing)
	{
		return;
	}

	Elapsed += DeltaSeconds;
	TimeLeft -= DeltaSeconds;
	if (TimeLeft <= 0.f)
	{
		TimeLeft = 0.f;
		State = EMarbleRunState::TimeUp;
		ShowMessage(TEXT("TIME UP - press R"), 1.e6f);
	}
}

void AMarbleGameMode::OnCheckpointReached(ACheckpoint* Checkpoint)
{
	if (!IsPlaying() || !Checkpoint || ReachedCheckpoints.Contains(Checkpoint))
	{
		return;
	}

	ReachedCheckpoints.Add(Checkpoint);
	Checkpoint->SetReached(true);
	RespawnLocation = Checkpoint->GetRespawnLocation();
	TimeLeft += CheckpointBonus;
	ShowMessage(FString::Printf(TEXT("CHECKPOINT  +%.0fs"), CheckpointBonus));
}

void AMarbleGameMode::OnGoalReached()
{
	if (!IsPlaying())
	{
		return;
	}

	State = EMarbleRunState::Won;
	ShowMessage(FString::Printf(TEXT("FINISH!  %.2fs  -  press R"), Elapsed), 1.e6f);
}

void AMarbleGameMode::OnMarbleLost(AMarblePawn* Marble, const FString& Reason)
{
	if (!Marble)
	{
		return;
	}

	++Falls;
	if (IsPlaying())
	{
		TimeLeft = FMath::Max(0.f, TimeLeft - FallPenalty);
		ShowMessage(FString::Printf(TEXT("%s  -%.0fs"), *Reason, FallPenalty));
	}
	Marble->ResetTo(RespawnLocation);
}

void AMarbleGameMode::ShowMessage(const FString& Text, float Duration)
{
	Message = Text;
	MessageTimeLeft = Duration;
}
