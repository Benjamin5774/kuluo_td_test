// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SokobanPiece.generated.h"

/** Base for anything that slides between grid cells: glides with easing and can bump against obstacles. */
UCLASS(Abstract)
class SOKOBANDEMO_API ASokobanPiece : public AActor
{
	GENERATED_BODY()

public:
	ASokobanPiece();

	virtual void Tick(float DeltaSeconds) override;

	/** Glides to Location over Duration seconds. */
	void MoveTo(const FVector& Location, float Duration);

	/** Places the piece at Location immediately. */
	void SnapTo(const FVector& Location);

	/** Jumps to the end of any running glide or bump. */
	void FinishMove();

	/** Shoves the visuals Distance along WorldDirection and back, used when a move is refused. */
	void Bump(const FVector& WorldDirection, float Distance, float Duration);

	bool IsMoving() const { return bMoving; }

protected:
	/** Called when a glide completes, including when FinishMove cuts it short. */
	virtual void OnMoveFinished() {}

	/** Holds the visuals so a bump can offset them without moving the piece itself. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<USceneComponent> VisualRoot;

private:
	FVector MoveStart = FVector::ZeroVector;
	FVector MoveTarget = FVector::ZeroVector;
	float MoveDuration = 0.f;
	double MoveStartTime = 0.0;
	bool bMoving = false;

	FVector BumpOffset = FVector::ZeroVector;
	float BumpDuration = 0.f;
	double BumpStartTime = 0.0;
};
