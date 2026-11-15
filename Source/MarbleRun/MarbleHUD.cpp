#include "MarbleHUD.h"
#include "MarbleGameMode.h"
#include "MarblePawn.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

void AMarbleHUD::DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color)
{
	UFont* Font = GEngine->GetLargeFont();
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, Scale);
	DrawText(Text, Color, (Canvas->ClipX - W) * 0.5f, Y, Font, Scale);
}

void AMarbleHUD::DrawHUD()
{
	Super::DrawHUD();

	const AMarbleGameMode* GM = GetWorld()->GetAuthGameMode<AMarbleGameMode>();
	if (!GM || !Canvas)
	{
		return;
	}

	const float UI = Canvas->ClipY / 1080.f;
	UFont* Font = GEngine->GetLargeFont();

	const FLinearColor TimeColor = GM->GetTimeLeft() < 10.f ? FLinearColor(1.f, 0.2f, 0.15f) : FLinearColor::White;
	DrawCentered(FString::Printf(TEXT("%.1f"), GM->GetTimeLeft()), 30.f * UI, 3.f * UI, TimeColor);

	const FString Message = GM->GetMessage();
	if (!Message.IsEmpty())
	{
		DrawCentered(Message, Canvas->ClipY * 0.38f, 2.5f * UI, FLinearColor(1.f, 0.85f, 0.1f));
	}

	if (const AMarblePawn* Marble = Cast<AMarblePawn>(GetOwningPawn()))
	{
		DrawText(FString::Printf(TEXT("%.1f m/s   falls: %d"), Marble->GetSpeed() / 100.f, GM->GetFalls()),
			FLinearColor::White, 30.f * UI, Canvas->ClipY - 70.f * UI, Font, 1.4f * UI);
	}

	DrawText(TEXT("WASD / stick: roll    Space / A: brake    R / Start: restart"),
		FLinearColor(0.8f, 0.8f, 0.8f), 30.f * UI, Canvas->ClipY - 35.f * UI, Font, 1.f * UI);
}
