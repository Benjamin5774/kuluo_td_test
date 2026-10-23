// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "View/SokobanPiece.h"
#include "SokobanCrate.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
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

	/** Material with Color / Roughness / Glow parameters. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	TObjectPtr<UMaterialInterface> SolidMaterial;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor Color = FLinearColor(0.3f, 0.11f, 0.03f);

	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	FLinearColor OnGoalColor = FLinearColor(0.95f, 0.45f, 0.02f);

	/** Emissive strength while resting on a goal. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Look")
	float OnGoalGlow = 0.6f;

private:
	void ApplyColor();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	bool bOnGoal = false;
	bool bPendingOnGoal = false;
};
