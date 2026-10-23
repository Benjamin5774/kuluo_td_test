// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SokobanCameraRig.generated.h"

class UCameraComponent;

/** Fixed high-angle camera that frames the whole board and glides when the level changes. */
UCLASS()
class SOKOBANDEMO_API ASokobanCameraRig : public AActor
{
	GENERATED_BODY()

public:
	ASokobanCameraRig();

	virtual void Tick(float DeltaSeconds) override;

	/** Frames Bounds from the fixed angle; glides there unless bInstant. */
	void FrameBounds(const FBox& Bounds, bool bInstant);

	/** Degrees below the horizon. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "20", ClampMax = "89"))
	float Pitch = 62.f;

	/** Horizontal field of view in degrees. A narrow lens keeps the grid readable. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "10", ClampMax = "90"))
	float FieldOfView = 38.f;

	/** Extra room around the board (1 = edge to edge). */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera", meta = (ClampMin = "1"))
	float Margin = 1.18f;

	/** How quickly the camera catches up with a new framing. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float BlendSpeed = 4.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UCameraComponent> Camera;

private:
	/** Camera location that frames FramedBounds at the current viewport aspect ratio. */
	FVector ComputeLocation() const;
	static float GetViewportAspect();

	FBox FramedBounds = FBox(ForceInit);
	FVector TargetLocation = FVector::ZeroVector;
	float FramedAspect = 0.f;
};
