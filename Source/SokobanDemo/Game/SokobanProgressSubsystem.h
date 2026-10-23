// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SokobanProgressSubsystem.generated.h"

/** Player progress as written to disk. */
UCLASS()
class SOKOBANDEMO_API USokobanSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Version = 1;

	/** Fewest moves used to solve each level, keyed by level id. */
	UPROPERTY()
	TMap<FString, int32> BestMoves;

	/** Level the player was on last, so the main menu can resume it. */
	UPROPERTY()
	FString LastLevelId;
};

/** Remembers which levels are solved and the best move counts. Saves after every change. */
UCLASS()
class SOKOBANDEMO_API USokobanProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	bool IsSolved(const FString& LevelId) const;

	/** Fewest moves recorded for a level, or 0 when it has never been solved. */
	int32 GetBestMoves(const FString& LevelId) const;

	/** Records a solve. Returns true when it is the first solve or beats the previous best. */
	bool RecordSolve(const FString& LevelId, int32 Moves);

	const FString& GetLastLevelId() const;
	void SetLastLevelId(const FString& LevelId);

	/** Forgets every solve. */
	void ResetProgress();

private:
	void Save() const;

	UPROPERTY(Transient)
	TObjectPtr<USokobanSaveGame> SaveData;
};
