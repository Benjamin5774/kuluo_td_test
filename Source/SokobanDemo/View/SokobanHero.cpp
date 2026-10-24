// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanHero.h"
#include "SokobanDemo.h"
#include "Puzzle/SokobanGrid.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"

static TAutoConsoleVariable<bool> CVarTraceHero(TEXT("Sokoban.TraceHero"), false,
	TEXT("Dev: logs where the hero is drawn every frame, for tuning how moves flow into one another."));

ASokobanHero::ASokobanHero()
{
	// The strongman is a chubby ginger cat, modelled, rigged and animated for this project
	// (agent tools: make_cartoon_cat.py in Blender, then import_ginger_cat.py).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(TEXT("/Game/Sokoban/Characters/GingerCat/GingerCat.GingerCat"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleClip(TEXT("/Game/Sokoban/Characters/GingerCat/Idle.Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> WalkClip(TEXT("/Game/Sokoban/Characters/GingerCat/Walk.Walk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> PushClip(TEXT("/Game/Sokoban/Characters/GingerCat/Push.Push"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> VictoryClip(TEXT("/Game/Sokoban/Characters/GingerCat/Cheer.Cheer"));

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(VisualRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	// The model faces +Y; turn it so the actor's forward (+X) is the hero's front.
	Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	// Authored 90 cm to the ear tips; a touch bigger reads as the burly one from the high camera.
	Mesh->SetRelativeScale3D(FVector(1.12f));
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
	// Both clips hold two footfalls: one step per cell, walking or pushing, timed to the move.
	ActionCyclesPerMove.Add(ESokobanHeroAction::Walk, 0.5f);
	ActionCyclesPerMove.Add(ESokobanHeroAction::Push, 0.5f);
	// The push clip leans in with the arms locked forward; shift the body so the paws meet the crate.
	ActionReaches.Add(ESokobanHeroAction::Push, 17.f);
	// Turn quickly enough to face the new way within the first part of a step. The push clip already
	// trembles, so the body doesn't shudder as well.
	TurnSpeed = 1440.f;
	SettleSquash = 0.04f;
	StrainJitter = 0.f;
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

	if (CVarTraceHero.GetValueOnGameThread())
	{
		UE_LOG(LogSokoban, Log, TEXT("Trace %.4f %s"), GetWorld()->GetTimeSeconds(), *GetActorLocation().ToCompactString());
	}
}

void ASokobanHero::SetFacing(ESokobanDirection Direction, bool bInstant)
{
	TargetYaw = SokobanGrid::ToYaw(Direction);
	if (bInstant)
	{
		SetActorRotation(FRotator(0.f, TargetYaw, 0.f));
	}
}

void ASokobanHero::PlayAction(ESokobanHeroAction Action, float MoveSeconds)
{
	IdleCountdown = -1.f;

	const TObjectPtr<UAnimSequenceBase>* Clip = ActionClips.Find(Action);
	if (!Clip || !*Clip)
	{
		Clip = ActionClips.Find(ESokobanHeroAction::Idle);
	}
	if (!Clip || !*Clip)
	{
		return;
	}

	const float* Cycles = ActionCyclesPerMove.Find(Action);
	const float* FixedRate = ActionPlayRates.Find(Action);
	const float Rate = Cycles && MoveSeconds > 0.f ? (*Clip)->GetPlayLength() * *Cycles / MoveSeconds : (FixedRate ? *FixedRate : 1.f);

	// Keep the clip running through consecutive moves, so the stride carries on instead of restarting.
	if (!bHasAction || Action != CurrentAction)
	{
		CurrentAction = Action;
		bHasAction = true;
		Mesh->PlayAnimation(*Clip, true);
	}
	Mesh->SetPlayRate(Rate);
}

void ASokobanHero::OnMoveFinished()
{
	IdleCountdown = StrideGrace;
}
