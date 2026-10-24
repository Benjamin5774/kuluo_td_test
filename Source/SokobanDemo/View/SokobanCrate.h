// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "View/SokobanPiece.h"
#include "SokobanCrate.generated.h"

class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UStaticMeshComponent;

/** A pushable crate. Lights up once it settles on a goal. */
UCLASS()
class SOKOBANDEMO_API ASokobanCrate : public ASokobanPiece
{
	GENERATED_BODY()

public:
	ASokobanCrate();

	/** Updates the goal highlight; applied when the crate stops moving unless bInstant. */
	void SetOnGoal(bool bInOnGoal, bool bInstant);

	/** The crate's mesh; the board's editor preview copies it for its stand-ins. */
	UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Gives Target the look of a crate resting off a goal (used for the editor preview's stand-ins). */
	void PaintLook(UPrimitiveComponent* Target) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnMoveFinished() override;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Multiplies the crate's texture (the mesh material's Tint parameter). */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor Tint = FLinearColor(1.f, 1.f, 1.f);

	/** Tint while resting on a goal: the wood warms up towards the goal's gold. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor OnGoalTint = FLinearColor(1.35f, 1.05f, 0.55f);

	/** Emissive strength while resting on a goal. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float OnGoalGlow = 0.45f;

private:
	void ApplyColor();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	bool bOnGoal = false;
	bool bPendingOnGoal = false;
};
