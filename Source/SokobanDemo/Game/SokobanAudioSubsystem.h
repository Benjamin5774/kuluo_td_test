// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SokobanAudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/** The game's sound effects. */
UENUM()
enum class ESokobanSound : uint8
{
	Step,
	Push,
	Goal,
	Plate,
	TrapdoorOpen,
	TrapdoorClose,
	Bump,
	Undo,
	Click,
	Win,
	Star
};

/** What plays behind everything. */
UENUM()
enum class ESokobanMusic : uint8
{
	None,
	Title,
	Game
};

/** The takes of one effect, and which one played last. */
USTRUCT()
struct FSokobanSoundTakes
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> Takes;

	int32 Last = INDEX_NONE;
};

/**
 * The game's sound: music fading between the title and the game, the sea underneath, and effects. An effect plays a
 * little differently every time (another take where there are several, pitch and level nudged), so repeats don't
 * sound mechanical. Sounds are the assets in /Game/Sokoban/Audio, found by name (see import_audio.py), so any of
 * them can be replaced by a file of the same name; a missing one is simply silent.
 */
UCLASS()
class SOKOBANDEMO_API USokobanAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** The audio of WorldContext's game, if it has one. */
	static USokobanAudioSubsystem* Get(const UObject* WorldContext);

	/** Plays Sound once at about Volume and Pitch, after Delay seconds. */
	void Play(ESokobanSound Sound, float Volume = 1.f, float Pitch = 1.f, float Delay = 0.f);

	/** Fades over to Music, and brings the sea nearer (on the title's beach) or further away. */
	void SetMusic(ESokobanMusic Music);

private:
	/** A take of Sound other than the last one played, when there is a choice. */
	USoundBase* PickTake(ESokobanSound Sound);

	/** Starts Sound looping, fading in to Volume. */
	UAudioComponent* StartLoop(USoundBase* Sound, float Volume, float FadeTime);

	UPROPERTY(Transient)
	TMap<ESokobanSound, FSokobanSoundTakes> Sounds;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> TitleMusic;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> GameMusic;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> Sea;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> SeaPlayer;

	ESokobanMusic CurrentMusic = ESokobanMusic::None;
};
