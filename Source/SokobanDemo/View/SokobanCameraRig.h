// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SokobanCameraRig.generated.h"

class UCameraComponent;

/**
 * High-angle camera. Zoomed out (the starting view) it frames the scene around the castle, an area big enough
 * for the largest level of the pack, so the hand-placed scenery looks the same in every level; zoomed in it
 * frames the current level tightly. The player zooms with the mouse wheel; level changes glide.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanCameraRig : public AActor
{
	GENERATED_BODY()

public:
	ASokobanCameraRig();

	virtual void Tick(float DeltaSeconds) override;

	/** Frames a level: zoomed out it shows InSceneBounds, zoomed in InLevelBounds. Glides there unless bInstant. */
	void FrameBounds(const FBox& InLevelBounds, const FBox& InSceneBounds, bool bInstant);

	/** Zooms in (positive) or out (negative) by Steps notches of ZoomStep. */
	void AddZoom(float Steps);

	/** A short vertical jolt of the view after Delay seconds, scaled by Strength (1 = JoltAmplitude). */
	void Jolt(float Strength, float Delay);

	/** Degrees below the horizon when zoomed all the way out (the starting view). */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "20", ClampMax = "89"))
	float WidePitch = 56.f;

	/** Degrees below the horizon when zoomed all the way in. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "20", ClampMax = "89"))
	float ClosePitch = 62.f;

	/** Room around the scene area when zoomed all the way out (1 = edge to edge). */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "1"))
	float WideMargin = 1.95f;

	/** Room around the level when zoomed all the way in. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "1"))
	float CloseMargin = 1.18f;

	/** Horizontal field of view in degrees. A narrow lens keeps the grid readable. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "10", ClampMax = "90"))
	float FieldOfView = 38.f;

	/** Share of the zoom range one mouse-wheel notch covers. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ZoomStep = 0.2f;

	/** How quickly the camera catches up with a new framing or zoom. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float BlendSpeed = 4.f;

	/** Height in cm of a full-strength jolt; 0 turns jolts off. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "0"))
	float JoltAmplitude = 6.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "0.01"))
	float JoltDuration = 0.25f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UCameraComponent> Camera;

private:
	/** Where the camera aims, and how far back along its view it stands. */
	struct FFraming
	{
		FVector Aim = FVector::ZeroVector;
		float Distance = 0.f;
	};

	/** Pitch at the current zoom. */
	float GetPitch() const;

	/** Fits Bounds into the view from Pitch with Margin, at the current viewport aspect ratio. */
	FFraming ComputeFraming(const FBox& Bounds, float Pitch, float Margin) const;

	/** Camera location at the current zoom, between the scene framing and the level framing. */
	FVector ComputeLocation() const;
	static float GetViewportAspect();

	FBox LevelBounds = FBox(ForceInit);
	FBox SceneBounds = FBox(ForceInit);
	float FramedAspect = 0.f;

	/** 0 = wide (the starting view), 1 = close. */
	float Zoom = 0.f;

	/** Where the camera is heading and where it is now, before any jolt. */
	FVector TargetLocation = FVector::ZeroVector;
	FVector CurrentLocation = FVector::ZeroVector;
	float CurrentPitch = 0.f;

	double JoltStartTime = -1.0;
	float JoltStrength = 0.f;
};
