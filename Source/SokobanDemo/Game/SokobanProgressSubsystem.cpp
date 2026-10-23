// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/SokobanProgressSubsystem.h"
#include "SokobanDemo.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* const SlotName = TEXT("SokobanProgress");
}

void USokobanProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveData = Cast<USokobanSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!SaveData)
	{
		SaveData = Cast<USokobanSaveGame>(UGameplayStatics::CreateSaveGameObject(USokobanSaveGame::StaticClass()));
	}
	UE_LOG(LogSokoban, Log, TEXT("Progress loaded: %d levels solved."), SaveData->BestMoves.Num());
}

bool USokobanProgressSubsystem::IsSolved(const FString& LevelId) const
{
	return SaveData->BestMoves.Contains(LevelId);
}

int32 USokobanProgressSubsystem::GetBestMoves(const FString& LevelId) const
{
	const int32* Best = SaveData->BestMoves.Find(LevelId);
	return Best ? *Best : 0;
}

bool USokobanProgressSubsystem::RecordSolve(const FString& LevelId, int32 Moves)
{
	const int32* Best = SaveData->BestMoves.Find(LevelId);
	const bool bImproved = !Best || Moves < *Best;
	if (bImproved)
	{
		SaveData->BestMoves.Add(LevelId, Moves);
		Save();
	}
	return bImproved;
}

const FString& USokobanProgressSubsystem::GetLastLevelId() const
{
	return SaveData->LastLevelId;
}

void USokobanProgressSubsystem::SetLastLevelId(const FString& LevelId)
{
	if (SaveData->LastLevelId != LevelId)
	{
		SaveData->LastLevelId = LevelId;
		Save();
	}
}

void USokobanProgressSubsystem::ResetProgress()
{
	SaveData->BestMoves.Reset();
	SaveData->LastLevelId.Reset();
	Save();
}

void USokobanProgressSubsystem::Save() const
{
	if (!UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0))
	{
		UE_LOG(LogSokoban, Warning, TEXT("Could not save progress to slot %s."), SlotName);
	}
}
