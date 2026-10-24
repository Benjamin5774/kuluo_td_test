// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanHero.h"
#include "Puzzle/SokobanGrid.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

ASokobanHero::ASokobanHero()
{
	// The strongman is a fat cat: "Cat Pilot" by Tomcat94 (OpenGameArt, CC0), fattened and given Idle and
	// Push clips next to its own Walk and Cheer (agent tools: make_fat_cat.py, then import_fat_cat.py).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(TEXT("/Game/Sokoban/Characters/FatCat/FatCat.FatCat"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleClip(TEXT("/Game/Sokoban/Characters/FatCat/Idle.Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> WalkClip(TEXT("/Game/Sokoban/Characters/FatCat/Walk.Walk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> PushClip(TEXT("/Game/Sokoban/Characters/FatCat/Push.Push"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> VictoryClip(TEXT("/Game/Sokoban/Characters/FatCat/Cheer.Cheer"));

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(VisualRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	// The model faces +Y; turn it so the actor's forward (+X) is the hero's front.
	Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	// 2.56 m tall as authored: a little over a cell high, so the round little cat reads from the high camera.
	Mesh->SetRelativeScale3D(FVector(0.46f));
	if (BodyMesh.Succeeded())
	{
		Mesh->SetSkeletalMeshAsset(BodyMesh.Object);
	}

	if (IdleClip.Succeeded())
	{
		ActionClips.Add(ESokobanHeroAction::Idle, IdleClip.Object);
	}
	if (WalkClip.Succeeded())
	{
		ActionClips.Add(ESokobanHeroAction::Walk, WalkClip.Object);
	}
	if (PushClip.Succeeded())
	{
		ActionClips.Add(ESokobanHeroAction::Push, PushClip.Object);
	}
	if (VictoryClip.Succeeded())
	{
		ActionClips.Add(ESokobanHeroAction::Victory, VictoryClip.Object);
	}
	// A step takes a fraction of a second, so both cycles run faster than their natural pace.
	ActionPlayRates.Add(ESokobanHeroAction::Walk, 2.f);
	ActionPlayRates.Add(ESokobanHeroAction::Push, 1.6f);
	// Short arms: the whole cat leans into the crate.
	ActionReaches.Add(ESokobanHeroAction::Push, 34.f);
}

void ASokobanHero::AddPartsTo(USkeletalMeshComponent* Body, EComponentCreationMethod CreationMethod) const
{
	AActor* BodyOwner = Body->GetOwner();
	for (USkeletalMesh* Part : AttachedParts)
	{
		if (!Part)
		{
			continue;
		}
		USkeletalMeshComponent* Follower = NewObject<USkeletalMeshComponent>(BodyOwner, NAME_None, RF_Transient);
		Follower->CreationMethod = CreationMethod;
		Follower->SetupAttachment(Body);
		Follower->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Follower->SetSkeletalMeshAsset(Part);
		Follower->SetLeaderPoseComponent(Body);
		Follower->RegisterComponent();
		if (CreationMethod == EComponentCreationMethod::Instance)
		{
			BodyOwner->AddInstanceComponent(Follower);
		}
	}
}

void ASokobanHero::BeginPlay()
{
	Super::BeginPlay();
	AddPartsTo(Mesh, EComponentCreationMethod::Instance);
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

	// The body (not VisualRoot, which bumps use) shifts towards the crate while pushing.
	const float* TargetReach = ActionReaches.Find(CurrentAction);
	CurrentReach = FMath::FInterpTo(CurrentReach, TargetReach ? *TargetReach : 0.f, DeltaSeconds, LeanSpeed);
	Mesh->SetRelativeLocation(FVector(CurrentReach, 0.f, 0.f));
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
