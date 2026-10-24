// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanPiece.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ASokobanPiece::ASokobanPiece()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the player controller, so a move issued this frame is already part of this frame's glide.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

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
		const float Elapsed = static_cast<float>(Now - CurrentGlide.StartTime);
		SetActorLocation(FMath::Lerp(MoveStart, MoveTarget, GlideProgress(Elapsed)));
		if (CurrentGlide.Style == ESokobanGlideStyle::Shove && StrainJitter > 0.f)
		{
			// Shudder against the shove until the load gives.
			const float Shudder = Elapsed < CurrentGlide.Hold ? StrainJitter * FMath::Sin(static_cast<float>(Now) * 18.f * UE_TWO_PI) : 0.f;
			VisualRoot->SetWorldLocation(GetActorLocation() + (MoveTarget - MoveStart).GetSafeNormal() * Shudder);
		}
		if (Elapsed >= CurrentGlide.Duration)
		{
			CompleteGlide();
		}
	}

	if (SettleStartTime >= 0.0)
	{
		// Squash down and spread a little, then spring back.
		const float Alpha = FMath::Clamp(static_cast<float>((Now - SettleStartTime) / SettleDuration), 0.f, 1.f);
		const float Squash = SettleSquash * FMath::Sin(PI * Alpha) * (1.f - Alpha * 0.5f);
		VisualRoot->SetRelativeScale3D(FVector(1.f + Squash * 0.5f, 1.f + Squash * 0.5f, 1.f - Squash));
		if (Alpha >= 1.f)
		{
			SettleStartTime = -1.0;
			VisualRoot->SetRelativeScale3D(FVector::OneVector);
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
	FSokobanGlide Glide;
	Glide.Duration = Duration;
	MoveTo(Location, Glide);
}

void ASokobanPiece::MoveTo(const FVector& Location, const FSokobanGlide& Glide)
{
	FinishMove();

	MoveStart = GetActorLocation();
	MoveTarget = Location;
	CurrentGlide = Glide;
	if (CurrentGlide.StartTime < 0.0)
	{
		CurrentGlide.StartTime = GetWorld()->GetTimeSeconds();
	}
	if (Glide.Style == ESokobanGlideStyle::ShoveOn && SettleStartTime >= 0.0)
	{
		// The shove carries straight on, so it hasn't come to rest after all.
		SettleStartTime = -1.0;
		VisualRoot->SetRelativeScale3D(FVector::OneVector);
	}
	bMoving = true;
}

void ASokobanPiece::SnapTo(const FVector& Location)
{
	bMoving = false;
	BumpDuration = 0.f;
	SettleStartTime = -1.0;
	VisualRoot->SetRelativeLocation(FVector::ZeroVector);
	VisualRoot->SetRelativeScale3D(FVector::OneVector);
	SetActorLocation(Location);
}

float ASokobanPiece::GlideProgress(float Elapsed) const
{
	const float Duration = CurrentGlide.Duration;
	if (Duration <= 0.f)
	{
		return 1.f;
	}

	switch (CurrentGlide.Style)
	{
	case ESokobanGlideStyle::Light:
		return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Elapsed / Duration, 0.f, 1.f), 2.5f);
	case ESokobanGlideStyle::Shove:
	{
		// Strain against the load, then break it loose and keep it sliding at the pace of a shove under way.
		const float Travel = Duration - CurrentGlide.Hold;
		return Travel > 0.f ? FMath::Clamp((Elapsed - CurrentGlide.Hold) / Travel, 0.f, 1.f) : 1.f;
	}
	default:
		return FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	}
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
		CompleteGlide();
	}
}

void ASokobanPiece::CompleteGlide()
{
	bMoving = false;
	SetActorLocation(MoveTarget);
	const bool bShove = CurrentGlide.Style == ESokobanGlideStyle::Shove || CurrentGlide.Style == ESokobanGlideStyle::ShoveOn;
	if (bShove && SettleSquash > 0.f)
	{
		SettleStartTime = GetWorld()->GetTimeSeconds();
	}
	OnMoveFinished();
}

void ASokobanPiece::Bump(const FVector& WorldDirection, float Distance, float Duration)
{
	BumpOffset = WorldDirection.GetSafeNormal() * Distance;
	BumpDuration = Duration;
	BumpStartTime = GetWorld()->GetTimeSeconds();
}
