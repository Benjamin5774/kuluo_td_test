// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Puzzle/SokobanTypes.h"
#include "SokobanTitleStage.generated.h"

class ASokobanCrate;
class ASokobanHero;
class UCameraComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The title screen's scene, in front of the castle: a sandbar where the strongman shoves a pair of crates to and
 * fro, turning to the camera to cheer after each shove, filmed by a slowly swaying camera. Hidden except while the
 * main menu is up.
 */
UCLASS()
class SOKOBANDEMO_API ASokobanTitleStage : public AActor
{
	GENERATED_BODY()

public:
	ASokobanTitleStage();

	virtual void Tick(float DeltaSeconds) override;

	/** Shows the scene and starts the show from the top. */
	void Show();

	/** Puts the scene away. */
	void Hide();

	bool IsShown() const { return bShown; }

	/** Where the camera looks, from how far, at what angle, and with what lens. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	FVector Focus = FVector(-150.f, 150.f, 140.f);

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float Distance = 2100.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float Yaw = 18.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float Pitch = -3.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float FieldOfView = 44.f;

	/** How far the camera sways to either side (degrees of yaw) and up and down, and the seconds one sway takes. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float SwayYaw = 4.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float SwayHeight = 22.f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float SwayPeriod = 24.f;

	/** Cell (0, 0) of the strongman's stretch of beach, on the sand. Columns run east and rows south, as on the board. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Beach")
	FVector StageOrigin = FVector(-1040.f, -230.f, -36.f);

	UPROPERTY(EditAnywhere, Category = "Sokoban|Beach")
	float CellSize = 100.f;

	/** Sand of the sandbar: the terrain material in beach colours. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Beach")
	TObjectPtr<UMaterialInterface> SandMaterial;

	/** Seconds per walking step, and per shove once the crates slide, with the strain of the first shove. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Show")
	float WalkSeconds = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Show")
	float PushSeconds = 0.42f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Show")
	float PushStrain = 0.18f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Show")
	float CheerSeconds = 1.9f;

	/**
	 * While the title shows, the height fog thickens past FogStartDistance so the sea fades into the sky towards
	 * the horizon (the board's camera looks down and never sees that far).
	 */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float FogDensity = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	float FogStartDistance = 3200.f;

	/** The fog's colour meanwhile: the sky's blue, so the sea melts into it. */
	UPROPERTY(EditAnywhere, Category = "Sokoban|Camera")
	FLinearColor FogColor = FLinearColor(0.32f, 0.52f, 0.9f);

protected:
	virtual void BeginPlay() override;

private:
	/** One beat of the show. */
	enum class EBeat : uint8
	{
		Wait,
		Walk,
		Push,
		Cheer
	};

	struct FBeat
	{
		EBeat Kind = EBeat::Wait;
		ESokobanDirection Direction = ESokobanDirection::East;
		float Seconds = 0.f;
	};

	/** A piece of beach dressing: a mesh, where it goes relative to the board's centre, its turn and size. */
	UStaticMeshComponent* AddProp(FName Name, UStaticMesh* Mesh, const FVector& Location, float Yaw, const FVector& Scale);

	/** Starts the next beat when the current one is over. */
	void PlayBeat(const FBeat& Beat, double StartTime);

	void PlaceCamera(double Time);

	FVector CellToWorld(const FIntPoint& Cell) const;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Sokoban")
	TArray<TObjectPtr<UStaticMeshComponent>> Props;

	UPROPERTY(Transient)
	TObjectPtr<ASokobanHero> Cat;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASokobanCrate>> Crates;

	TArray<FBeat> Beats;
	int32 NextBeat = 0;
	double BeatEndTime = 0.0;

	/** Whether the shove under way follows another one straight on. */
	bool bShoving = false;

	FIntPoint CatCell = FIntPoint(0, 0);
	TArray<FIntPoint> CrateCells;

	bool bShown = false;
	double ShowStartTime = 0.0;

	/** The map's fog settings, put back when the title is hidden. */
	float SavedFogDensity = -1.f;
	float SavedFogStartDistance = 0.f;
	FLinearColor SavedFogColor;
};
