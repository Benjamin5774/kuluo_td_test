// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanHero.h"
#include "Puzzle/SokobanGrid.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

ASokobanHero::ASokobanHero()
{
	// The strongman: KayKit Barbarian (CC0, Kay Lousberg) with clips from KayKit Character Animations.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BarbarianMesh(TEXT("/Game/Sokoban/Characters/Barbarian/Barbarian.Barbarian"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleClip(TEXT("/Game/Sokoban/Characters/Animations/Rig_Medium_General_Idle_A.Rig_Medium_General_Idle_A"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> WalkClip(TEXT("/Game/Sokoban/Characters/Animations/Rig_Medium_MovementBasic_Walking_A.Rig_Medium_MovementBasic_Walking_A"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> CheerClip(TEXT("/Game/Sokoban/Characters/Animations/Rig_Medium_Simulation_Cheering.Rig_Medium_Simulation_Cheering"));

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(VisualRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	// The model faces +Y; turn it so the actor's forward (+X) is the hero's front.
	Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Mesh->SetRelativeScale3D(FVector(0.55f));
	if (BarbarianMesh.Succeeded())
	{
		Mesh->SetSkeletalMeshAsset(BarbarianMesh.Object);
	}

	if (IdleClip.Succeeded())
	{
		ActionClips.Add(ESokobanHeroAction::Idle, IdleClip.Object);
	}
	if (WalkClip.Succeeded())
	{
		ActionClips.Add(ESokobanHeroAction::Walk, WalkClip.Object);
		ActionClips.Add(ESokobanHeroAction::Push, WalkClip.Object);
	}
	if (CheerClip.Succeeded())
	{
		ActionClips.Add(ESokobanHeroAction::Victory, CheerClip.Object);
	}
	ActionPlayRates.Add(ESokobanHeroAction::Walk, 1.5f);
	// There is no dedicated push clip yet: a slower stride plus a forward lean reads as shoving.
	ActionPlayRates.Add(ESokobanHeroAction::Push, 0.9f);
	ActionLeans.Add(ESokobanHeroAction::Push, 14.f);
}

void ASokobanHero::BeginPlay()
{
	Super::BeginPlay();
	PlayAction(ESokobanHeroAction::Idle);
}

void ASokobanHero::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float CurrentYaw = GetActorRotation().Yaw;
	if (!FMath::IsNearlyZero(FRotator::NormalizeAxis(CurrentYaw - TargetYaw), 0.1f))
	{
		SetActorRotation(FRotator(0.f, FMath::FixedTurn(CurrentYaw, TargetYaw, TurnSpeed * DeltaSeconds), 0.f));
	}

	if (IdleCountdown >= 0.f)
	{
		IdleCountdown -= DeltaSeconds;
		if (IdleCountdown < 0.f)
		{
			PlayAction(ESokobanHeroAction::Idle);
		}
	}

	// Ease the body towards the current action's lean (pitching the visuals forward around the feet).
	const float* TargetLean = ActionLeans.Find(CurrentAction);
	CurrentLean = FMath::FInterpTo(CurrentLean, TargetLean ? *TargetLean : 0.f, DeltaSeconds, LeanSpeed);
	VisualRoot->SetRelativeRotation(FRotator(-CurrentLean, 0.f, 0.f));
}

void ASokobanHero::SetFacing(ESokobanDirection Direction, bool bInstant)
{
	TargetYaw = SokobanGrid::ToYaw(Direction);
	if (bInstant)
	{
		SetActorRotation(FRotator(0.f, TargetYaw, 0.f));
	}
}

void ASokobanHero::PlayAction(ESokobanHeroAction Action)
{
	IdleCountdown = -1.f;
	if (bHasAction && Action == CurrentAction)
	{
		// Keep the clip running so consecutive steps don't restart the stride.
		return;
	}

	const TObjectPtr<UAnimSequenceBase>* Clip = ActionClips.Find(Action);
	if (!Clip || !*Clip)
	{
		Clip = ActionClips.Find(ESokobanHeroAction::Idle);
	}
	if (!Clip || !*Clip)
	{
		return;
	}

	CurrentAction = Action;
	bHasAction = true;
	Mesh->PlayAnimation(*Clip, true);
	const float* Rate = ActionPlayRates.Find(Action);
	Mesh->SetPlayRate(Rate ? *Rate : 1.f);
}

void ASokobanHero::OnMoveFinished()
{
	IdleCountdown = StrideGrace;
}
