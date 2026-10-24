// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanCameraRig.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

ASokobanCameraRig::ASokobanCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
	Camera->bConstrainAspectRatio = false;

	// Grid moves are short and quick; motion blur only smears them.
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;
	// The sunlit sand floor dominates the frame and pushes auto exposure too bright.
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = -0.6f;
}

void ASokobanCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!LevelBounds.IsValid)
	{
		return;
	}

	// Follow the zoom, and re-frame when the window changes shape so the board always fits.
	FramedAspect = GetViewportAspect();
	TargetLocation = ComputeLocation();

	// Location and pitch blend at the same rate, so the board stays centred while either changes.
	CurrentLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaSeconds, BlendSpeed);
	CurrentPitch = FMath::FInterpTo(CurrentPitch, GetPitch(), DeltaSeconds, BlendSpeed);

	FVector Offset = FVector::ZeroVector;
	const double Now = GetWorld()->GetTimeSeconds();
	if (JoltStartTime >= 0.0 && Now >= JoltStartTime)
	{
		// A few quick, fading bounces, like a heavy weight hitting the ground.
		const float Alpha = static_cast<float>((Now - JoltStartTime) / JoltDuration);
		if (Alpha >= 1.f)
		{
			JoltStartTime = -1.0;
		}
		else
		{
			Offset.Z = JoltAmplitude * JoltStrength * FMath::Sin(Alpha * 3.f * UE_TWO_PI) * FMath::Square(1.f - Alpha);
		}
	}

	SetActorLocationAndRotation(CurrentLocation + Offset, FRotator(-CurrentPitch, 0.f, 0.f));
}

void ASokobanCameraRig::FrameBounds(const FBox& InLevelBounds, const FBox& InSceneBounds, bool bInstant)
{
	LevelBounds = InLevelBounds;
	SceneBounds = InSceneBounds.IsValid ? InSceneBounds + InLevelBounds : InLevelBounds;
	FramedAspect = GetViewportAspect();
	Camera->SetFieldOfView(FieldOfView);

	TargetLocation = ComputeLocation();
	if (bInstant || CurrentLocation.IsZero())
	{
		CurrentLocation = TargetLocation;
		CurrentPitch = GetPitch();
		SetActorLocationAndRotation(CurrentLocation, FRotator(-CurrentPitch, 0.f, 0.f));
	}
}

void ASokobanCameraRig::AddZoom(float Steps)
{
	Zoom = FMath::Clamp(Zoom + Steps * ZoomStep, 0.f, 1.f);
}

void ASokobanCameraRig::Jolt(float Strength, float Delay)
{
	if (JoltAmplitude <= 0.f || Strength <= 0.f)
	{
		return;
	}
	JoltStrength = Strength;
	JoltStartTime = GetWorld()->GetTimeSeconds() + Delay;
}

float ASokobanCameraRig::GetPitch() const
{
	return FMath::Lerp(WidePitch, ClosePitch, Zoom);
}

FVector ASokobanCameraRig::ComputeLocation() const
{
	// Blend the aim point and the distance rather than the two camera positions, so the view stays centred
	// on the way between the scene and the level.
	const FFraming Wide = ComputeFraming(SceneBounds, WidePitch, WideMargin);
	const FFraming Close = ComputeFraming(LevelBounds, ClosePitch, CloseMargin);
	const FVector Aim = FMath::Lerp(Wide.Aim, Close.Aim, Zoom);
	const float Distance = FMath::Lerp(Wide.Distance, Close.Distance, Zoom);
	return Aim - FRotationMatrix(FRotator(-GetPitch(), 0.f, 0.f)).GetUnitAxis(EAxis::X) * Distance;
}

ASokobanCameraRig::FFraming ASokobanCameraRig::ComputeFraming(const FBox& Bounds, float Pitch, float Margin) const
{
	const FRotationMatrix View(FRotator(-Pitch, 0.f, 0.f));
	const FVector Forward = View.GetUnitAxis(EAxis::X);
	const FVector Right = View.GetUnitAxis(EAxis::Y);
	const FVector Up = View.GetUnitAxis(EAxis::Z);

	// Tangents of the half field of view, shrunk by the margin.
	const float TanHalfH = FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f)) / Margin;
	const float TanHalfV = TanHalfH / FramedAspect;

	FVector Corners[8];
	Bounds.GetVertices(Corners);

	// Perspective makes the near edge loom larger, so fit every corner of the bounds to the frustum,
	// then nudge the aim point until the top and bottom margins match. A few passes converge.
	FVector Aim = Bounds.GetCenter();
	float Distance = 0.f;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		Distance = 0.f;
		for (const FVector& Corner : Corners)
		{
			const FVector Local = Corner - Aim;
			const float Depth = FVector::DotProduct(Local, Forward);
			Distance = FMath::Max(Distance, FMath::Abs(FVector::DotProduct(Local, Right)) / TanHalfH - Depth);
			Distance = FMath::Max(Distance, FMath::Abs(FVector::DotProduct(Local, Up)) / TanHalfV - Depth);
		}

		float Lowest = TNumericLimits<float>::Max();
		float Highest = TNumericLimits<float>::Lowest();
		for (const FVector& Corner : Corners)
		{
			const FVector Local = Corner - Aim;
			const float ScreenY = FVector::DotProduct(Local, Up) / (Distance + FVector::DotProduct(Local, Forward));
			Lowest = FMath::Min(Lowest, ScreenY);
			Highest = FMath::Max(Highest, ScreenY);
		}
		Aim += Up * ((Lowest + Highest) * 0.5f * Distance);
	}

	return { Aim, Distance };
}

float ASokobanCameraRig::GetViewportAspect()
{
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Size;
		GEngine->GameViewport->GetViewportSize(Size);
		if (Size.X > 0.f && Size.Y > 0.f)
		{
			return Size.X / Size.Y;
		}
	}
	return 16.f / 9.f;
}
