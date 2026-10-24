// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/SokobanAudioSubsystem.h"
#include "SokobanDemo.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	/** Music and sea levels, and how long they take to change. */
	constexpr float TitleMusicVolume = 0.5f;
	constexpr float GameMusicVolume = 0.36f;
	constexpr float TitleSeaVolume = 0.6f;
	constexpr float GameSeaVolume = 0.22f;
	constexpr float MusicFadeTime = 1.4f;

	USoundBase* LoadSound(const TCHAR* Name)
	{
		return LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Sokoban/Audio/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

void USokobanAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	auto Register = [this](ESokobanSound Sound, std::initializer_list<const TCHAR*> Names)
	{
		FSokobanSoundTakes& Entry = Sounds.FindOrAdd(Sound);
		for (const TCHAR* Name : Names)
		{
			if (USoundBase* Take = LoadSound(Name))
			{
				Entry.Takes.Add(Take);
			}
		}
	};
	Register(ESokobanSound::Step, { TEXT("sfx_step_1"), TEXT("sfx_step_2"), TEXT("sfx_step_3") });
	Register(ESokobanSound::Push, { TEXT("sfx_push") });
	Register(ESokobanSound::Goal, { TEXT("sfx_goal") });
	Register(ESokobanSound::Plate, { TEXT("sfx_plate") });
	Register(ESokobanSound::TrapdoorOpen, { TEXT("sfx_trapdoor_open") });
	Register(ESokobanSound::TrapdoorClose, { TEXT("sfx_trapdoor_close") });
	Register(ESokobanSound::Bump, { TEXT("sfx_bump") });
	Register(ESokobanSound::Undo, { TEXT("sfx_undo") });
	Register(ESokobanSound::Click, { TEXT("sfx_click") });
	Register(ESokobanSound::Win, { TEXT("sfx_win") });
	Register(ESokobanSound::Star, { TEXT("sfx_star") });
	TitleMusic = LoadSound(TEXT("bgm_title"));
	GameMusic = LoadSound(TEXT("bgm_game"));
	Sea = LoadSound(TEXT("amb_sea"));

	int32 Takes = 0;
	for (const TPair<ESokobanSound, FSokobanSoundTakes>& Entry : Sounds)
	{
		Takes += Entry.Value.Takes.Num();
	}
	UE_LOG(LogSokoban, Log, TEXT("Audio: %d effect takes, title music %s, game music %s, sea %s"), Takes,
		TitleMusic ? TEXT("yes") : TEXT("no"), GameMusic ? TEXT("yes") : TEXT("no"), Sea ? TEXT("yes") : TEXT("no"));
}

USokobanAudioSubsystem* USokobanAudioSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USokobanAudioSubsystem>() : nullptr;
}

USoundBase* USokobanAudioSubsystem::PickTake(ESokobanSound Sound)
{
	FSokobanSoundTakes* Entry = Sounds.Find(Sound);
	if (!Entry || Entry->Takes.Num() == 0)
	{
		return nullptr;
	}
	int32 Index = FMath::RandRange(0, Entry->Takes.Num() - 1);
	if (Entry->Takes.Num() > 1 && Index == Entry->Last)
	{
		Index = (Index + FMath::RandRange(1, Entry->Takes.Num() - 1)) % Entry->Takes.Num();
	}
	Entry->Last = Index;
	return Entry->Takes[Index];
}

void USokobanAudioSubsystem::Play(ESokobanSound Sound, float Volume, float Pitch, float Delay)
{
	UWorld* World = GetGameInstance()->GetWorld();
	USoundBase* Take = PickTake(Sound);
	if (!World || !Take)
	{
		return;
	}

	// A little different every time: a hand never strikes twice the same.
	const float FinalVolume = Volume * FMath::FRandRange(0.86f, 1.04f);
	const float FinalPitch = Pitch * FMath::FRandRange(0.95f, 1.05f);
	if (Delay <= 0.f)
	{
		UGameplayStatics::PlaySound2D(World, Take, FinalVolume, FinalPitch);
		return;
	}
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, Take, FinalVolume, FinalPitch]()
	{
		if (UWorld* Now = GetGameInstance()->GetWorld())
		{
			UGameplayStatics::PlaySound2D(Now, Take, FinalVolume, FinalPitch);
		}
	}), Delay, false);
}

UAudioComponent* USokobanAudioSubsystem::StartLoop(USoundBase* Sound, float Volume, float FadeTime)
{
	UAudioComponent* Player = UGameplayStatics::CreateSound2D(GetGameInstance()->GetWorld(), Sound, 1.f, 1.f, 0.f, nullptr, true, false);
	if (Player)
	{
		Player->FadeIn(FadeTime, Volume);
	}
	return Player;
}

void USokobanAudioSubsystem::SetMusic(ESokobanMusic Music)
{
	if (Music == CurrentMusic || !GetGameInstance()->GetWorld())
	{
		return;
	}
	CurrentMusic = Music;

	if (MusicPlayer)
	{
		MusicPlayer->bAutoDestroy = true;
		MusicPlayer->FadeOut(MusicFadeTime, 0.f);
		MusicPlayer = nullptr;
	}
	USoundBase* Track = Music == ESokobanMusic::Title ? TitleMusic.Get() : (Music == ESokobanMusic::Game ? GameMusic.Get() : nullptr);
	if (Track)
	{
		MusicPlayer = StartLoop(Track, Music == ESokobanMusic::Title ? TitleMusicVolume : GameMusicVolume, MusicFadeTime);
	}

	if (!SeaPlayer && Sea)
	{
		SeaPlayer = StartLoop(Sea, 0.01f, 0.5f);
	}
	if (SeaPlayer)
	{
		SeaPlayer->AdjustVolume(2.5f, Music == ESokobanMusic::Title ? TitleSeaVolume : GameSeaVolume);
	}
}
