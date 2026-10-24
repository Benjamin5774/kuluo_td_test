// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Puzzle/SokobanTypes.h"
#include "View/SokobanPiece.h"
#include "SokobanHero.generated.h"

class UAnimSequenceBase;
class USkeletalMesh;
class USkeletalMeshComponent;

/** What the hero is doing, used to pick a clip from the action library. */
UENUM(BlueprintType)
enum class ESokobanHeroAction : uint8
{
	Idle,
	Walk,
	Push,
	Victory
};

/** The playable hero: the strongman, who shoves a whole row of crates. */
UCLASS()
class SOKOBANDEMO_API ASokobanHero : public ASokobanPiece
{
	GENERATED_BODY()

public:
	ASokobanHero();

	virtual void Tick(float DeltaSeconds) override;

	/** Turns towards Direction: a quick swivel, or immediately when bInstant. */
	void SetFacing(ESokobanDirection Direction, bool bInstant);

	/** Plays the clip registered for Action in the action library. */
	void PlayAction(ESokobanHeroAction Action);

	/** The body; the board's editor preview copies it for its stand-in. */
	USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/** Clip registered for Action, or null. */
	UAnimSequenceBase* GetActionClip(ESokobanHeroAction Action) const { return ActionClips.FindRef(Action); }

	/**
	 * Gives Body (this hero's own, or the board preview's stand-in) a follower component per attached part.
	 * Components made in OnConstruction pass UserConstructionScript, so a rerun clears them.
	 */
	void AddPartsTo(USkeletalMeshComponent* Body, EComponentCreationMethod CreationMethod) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnMoveFinished() override;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** Meshes skinned to the body's skeleton (beard, hair); they follow the body's pose. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TArray<TObjectPtr<USkeletalMesh>> AttachedParts;

	/** Action library: the clip each action plays. Actions without a clip fall back to Idle. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Animation")
	TMap<ESokobanHeroAction, TObjectPtr<UAnimSequenceBase>> ActionClips;

	/** Playback rate per action; 1 when missing. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Animation")
	TMap<ESokobanHeroAction, float> ActionPlayRates;

	/** Forward lean in degrees per action; 0 when missing. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Animation")
	TMap<ESokobanHeroAction, float> ActionLeans;

	/** How far forward the body shifts per action (so pushing hands meet the crate); 0 when missing. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Animation")
	TMap<ESokobanHeroAction, float> ActionReaches;

	/** How quickly the body eases into and out of a lean or reach. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Animation")
	float LeanSpeed = 12.f;

	/** Degrees per second when turning. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Animation")
	float TurnSpeed = 1080.f;

	/** Seconds to keep striding after a step ends, so consecutive steps read as one walk. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Animation")
	float StrideGrace = 0.12f;

private:
	float TargetYaw = 0.f;
	float CurrentLean = 0.f;
	float CurrentReach = 0.f;
	ESokobanHeroAction CurrentAction = ESokobanHeroAction::Idle;
	bool bHasAction = false;
	float IdleCountdown = -1.f;
};
