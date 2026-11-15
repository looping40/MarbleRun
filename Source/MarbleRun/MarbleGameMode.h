#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MarbleGameMode.generated.h"

class ATrackBuilder;
class ACheckpoint;
class AMarblePawn;

UENUM()
enum class EMarbleRunState : uint8
{
	Playing,
	Won,
	TimeUp
};

/** Owns the countdown, checkpoints and respawning. Builds the track if the level has none. */
UCLASS()
class MARBLERUN_API AMarbleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMarbleGameMode();

	virtual void StartPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;

	void OnCheckpointReached(ACheckpoint* Checkpoint);
	void OnGoalReached(AMarblePawn* Marble);
	void OnMarbleLost(AMarblePawn* Marble, const FString& Reason);
	void RestartRun();

	bool IsPlaying() const { return State == EMarbleRunState::Playing; }
	EMarbleRunState GetRunState() const { return State; }
	float GetTimeLeft() const { return TimeLeft; }
	float GetElapsed() const { return Elapsed; }
	int32 GetFalls() const { return Falls; }
	FString GetMessage() const { return MessageTimeLeft > 0.f ? Message : FString(); }

protected:
	UPROPERTY(EditAnywhere, Category = "Marble")
	float TimeLimit = 45.f;

	UPROPERTY(EditAnywhere, Category = "Marble")
	float CheckpointBonus = 10.f;

	UPROPERTY(EditAnywhere, Category = "Marble")
	float FallPenalty = 3.f;

private:
	void EnsureTrack();
	void ResetRunState();
	void ShowMessage(const FString& Text, float Duration = 2.f);

	UPROPERTY()
	TObjectPtr<ATrackBuilder> Track;

	UPROPERTY()
	TArray<TObjectPtr<ACheckpoint>> ReachedCheckpoints;

	FVector RespawnLocation = FVector(0.f, 0.f, 200.f);
	EMarbleRunState State = EMarbleRunState::Playing;
	float TimeLeft = 0.f;
	float Elapsed = 0.f;
	int32 Falls = 0;
	FString Message;
	float MessageTimeLeft = 0.f;
};
