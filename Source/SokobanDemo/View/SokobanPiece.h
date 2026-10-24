// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SokobanPiece.generated.h"

/** How a glide between cells moves. */
enum class ESokobanGlideStyle : uint8
{
	/** Sets off at once and eases in: a quick slide back into place (undo, restart). */
	Light,
	/** One steady pace from start to finish, so back-to-back moves join into one continuous motion. */
	Steady,
	/** Strains against the load for the glide's Hold, then slides at a steady pace; lands with a squash. */
	Shove,
	/** A shove already under way: a steady pace with no strain; lands with a squash unless another follows. */
	ShoveOn
};

/** One glide to a neighbouring cell. */
struct FSokobanGlide
{
	ESokobanGlideStyle Style = ESokobanGlideStyle::Light;

	/** Seconds from start to landing, Hold included. */
	float Duration = 0.f;

	/** Seconds a Shove strains before it moves. */
	float Hold = 0.f;

	/** World time the glide starts; now when negative. Starting when the previous glide landed carries on without a pause. */
	double StartTime = -1.0;
};

/** Base for anything that slides between grid cells: glides between them and can bump against obstacles. */
UCLASS(Abstract)
class SOKOBANDEMO_API ASokobanPiece : public AActor
{
	GENERATED_BODY()

public:
	ASokobanPiece();

	virtual void Tick(float DeltaSeconds) override;

	/** Glides to Location over Duration seconds, setting off at once and easing in. */
	void MoveTo(const FVector& Location, float Duration);

	/** Glides to Location as Glide describes. */
	void MoveTo(const FVector& Location, const FSokobanGlide& Glide);

	/** Places the piece at Location immediately. */
	void SnapTo(const FVector& Location);

	/** Jumps to the end of any running glide or bump. */
	void FinishMove();

	/** Shoves the visuals Distance along WorldDirection and back, used when a move is refused. */
	void Bump(const FVector& WorldDirection, float Distance, float Duration);

	bool IsMoving() const { return bMoving; }

	/** World time the latest glide lands (or landed). */
	double GetGlideEndTime() const { return CurrentGlide.StartTime + CurrentGlide.Duration; }

protected:
	/** Called when a glide completes, including when FinishMove cuts it short. */
	virtual void OnMoveFinished() {}

	/** Holds the visuals so a bump can offset them without moving the piece itself. */
	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<USceneComponent> VisualRoot;

	/** How far (cm) the piece shudders along the push while straining; 0 keeps it still. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Motion", meta = (ClampMin = "0"))
	float StrainJitter = 1.2f;

	/** How far the piece squashes (fraction of its height) as a shove comes to rest. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Motion", meta = (ClampMin = "0", ClampMax = "0.5"))
	float SettleSquash = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Motion")
	float SettleDuration = 0.2f;

private:
	/** Share (0..1) of the way the running glide has covered after Elapsed seconds. */
	float GlideProgress(float Elapsed) const;

	/** Lands the running glide: snaps to its target, starts the settle after a shove, notifies. */
	void CompleteGlide();

	FVector MoveStart = FVector::ZeroVector;
	FVector MoveTarget = FVector::ZeroVector;
	FSokobanGlide CurrentGlide;
	bool bMoving = false;

	/** When the squash after a shove began; negative when not settling. */
	double SettleStartTime = -1.0;

	FVector BumpOffset = FVector::ZeroVector;
	float BumpDuration = 0.f;
	double BumpStartTime = 0.0;
};
