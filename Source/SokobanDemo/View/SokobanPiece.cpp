// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanPiece.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ASokobanPiece::ASokobanPiece()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);
}

void ASokobanPiece::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Progress comes from the world clock rather than accumulated deltas, so pieces started by the same
	// move stay in step no matter which of them ticks before or after the move was issued this frame.
	const double Now = GetWorld()->GetTimeSeconds();

	if (bMoving)
	{
		const float Alpha = MoveDuration > 0.f ? FMath::Clamp(static_cast<float>((Now - MoveStartTime) / MoveDuration), 0.f, 1.f) : 1.f;
		SetActorLocation(FMath::Lerp(MoveStart, MoveTarget, FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.5f)));
		if (Alpha >= 1.f)
		{
			bMoving = false;
			OnMoveFinished();
		}
	}

	if (BumpDuration > 0.f)
	{
		const float Alpha = FMath::Clamp(static_cast<float>((Now - BumpStartTime) / BumpDuration), 0.f, 1.f);
		VisualRoot->SetWorldLocation(GetActorLocation() + BumpOffset * FMath::Sin(PI * Alpha));
		if (Alpha >= 1.f)
		{
			BumpDuration = 0.f;
			VisualRoot->SetRelativeLocation(FVector::ZeroVector);
		}
	}
}

void ASokobanPiece::MoveTo(const FVector& Location, float Duration)
{
	FinishMove();

	MoveStart = GetActorLocation();
	MoveTarget = Location;
	MoveDuration = Duration;
	MoveStartTime = GetWorld()->GetTimeSeconds();
	bMoving = true;
}

void ASokobanPiece::SnapTo(const FVector& Location)
{
	bMoving = false;
	BumpDuration = 0.f;
	VisualRoot->SetRelativeLocation(FVector::ZeroVector);
	SetActorLocation(Location);
}

void ASokobanPiece::FinishMove()
{
	if (BumpDuration > 0.f)
	{
		BumpDuration = 0.f;
		VisualRoot->SetRelativeLocation(FVector::ZeroVector);
	}

	if (bMoving)
	{
		bMoving = false;
		SetActorLocation(MoveTarget);
		OnMoveFinished();
	}
}

void ASokobanPiece::Bump(const FVector& WorldDirection, float Distance, float Duration)
{
	BumpOffset = WorldDirection.GetSafeNormal() * Distance;
	BumpDuration = Duration;
	BumpStartTime = GetWorld()->GetTimeSeconds();
}
