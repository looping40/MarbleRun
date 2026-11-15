#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MarbleHUD.generated.h"

/** Canvas-drawn HUD: countdown, status messages, speed and controls. */
UCLASS()
class MARBLERUN_API AMarbleHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color);
};
