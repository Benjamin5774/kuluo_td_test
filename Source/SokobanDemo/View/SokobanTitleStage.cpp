// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanTitleStage.h"
#include "Game/SokobanAudioSubsystem.h"
#include "Puzzle/SokobanGrid.h"
#include "View/SokobanCrate.h"
#include "View/SokobanHero.h"
#include "Camera/CameraComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The sandbar's main dome (the engine sphere, scaled): its centre, under the sea, and its radii. */
	const FVector DuneCenter(-1040.f, 20.f, -72.f);
	const FVector DuneRadii(330.f, 600.f, 40.f);

	UStaticMesh* FindStageMesh(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Path);
		return Finder.Object;
	}

	UStaticMesh* FindStageEnvironmentMesh(const TCHAR* Name)
	{
		return FindStageMesh(*FString::Printf(TEXT("/Game/Sokoban/Environment/Meshes/%s.%s"), Name, Name));
	}
}

ASokobanTitleStage::ASokobanTitleStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// The same look as the camera rig's, so the title and the game share one picture.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->bConstrainAspectRatio = false;
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.f;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = -0.5f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SandAsset(TEXT("/Game/Sokoban/Environment/Materials/MI_BeachSand.MI_BeachSand"));
	SandMaterial = SandAsset.Object;

	// The sandbar: two flattened domes rising out of the sea, which lies 60 cm below the castle floor.
	UStaticMesh* Sphere = FindStageMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	AddProp(TEXT("Dune"), Sphere, DuneCenter, 0.f, DuneRadii / 50.f)->SetMaterial(0, SandMaterial);
	AddProp(TEXT("Spit"), Sphere, FVector(-800.f, 350.f, -84.f), 30.f, FVector(4.2f, 6.f, 0.8f))->SetMaterial(0, SandMaterial);

	// The castle's gate, in the middle of its front wall (the castle is 10 by 6 cells, centred on this actor): an arched
	// plank door in a brick frame, from the Quaternius Medieval Village MegaKit (CC0), both turned to face the beach.
	// The door hangs from its left edge, so it sits half its width aside to fill the frame, set back a little in it.
	constexpr float GateScale = 0.65f;
	AddProp(TEXT("GateFrame"), FindStageEnvironmentMesh(TEXT("DoorFrame_Round_Brick")), FVector(-300.f, 0.f, 0.f), 90.f, FVector(GateScale));
	AddProp(TEXT("GateDoor"), FindStageEnvironmentMesh(TEXT("Door_2_Round")), FVector(-298.f, -51.5f * GateScale, 0.f), 90.f, FVector(GateScale));

	// Dressing from the Quaternius Stylized Nature MegaKit (CC0): rocks half buried in the sand, grass along its crest.
	AddProp(TEXT("RockWest"), FindStageEnvironmentMesh(TEXT("Rock_Medium_2")), FVector(-1190.f, -450.f, -54.f), 40.f, FVector(0.42f));
	AddProp(TEXT("RockEast"), FindStageEnvironmentMesh(TEXT("Rock_Medium_3")), FVector(-1240.f, 440.f, -60.f), 160.f, FVector(0.34f));
	AddProp(TEXT("Pebbles"), FindStageEnvironmentMesh(TEXT("Pebble_Round_2")), FVector(-1190.f, 150.f, -42.f), 10.f, FVector(0.7f));
	UStaticMesh* Grass = FindStageEnvironmentMesh(TEXT("Grass_Common_Short"));
	AddProp(TEXT("GrassA"), Grass, FVector(-860.f, -160.f, -42.f), 0.f, FVector(0.3f));
	AddProp(TEXT("GrassB"), Grass, FVector(-880.f, 110.f, -40.f), 70.f, FVector(0.26f));
	AddProp(TEXT("GrassC"), FindStageEnvironmentMesh(TEXT("Grass_Wispy_Short")), FVector(-830.f, 330.f, -46.f), 140.f, FVector(0.3f));
	AddProp(TEXT("Flowers"), FindStageEnvironmentMesh(TEXT("Bush_Common_Flowers")), FVector(-870.f, -360.f, -50.f), 20.f, FVector(0.36f));
}

UStaticMeshComponent* ASokobanTitleStage::AddProp(FName Name, UStaticMesh* Mesh, const FVector& Location, float PropYaw, const FVector& Scale)
{
	UStaticMeshComponent* Prop = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Prop->SetupAttachment(RootComponent);
	Prop->SetStaticMesh(Mesh);
	Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Prop->SetRelativeLocationAndRotation(Location, FRotator(0.f, PropYaw, 0.f));
	Prop->SetRelativeScale3D(Scale);
	Props.Add(Prop);
	return Prop;
}

void ASokobanTitleStage::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(true);

	// The show: shove the pair of crates two cells east, cheer, walk round in front of them, shove them back, cheer,
	// walk back round, and again. Steps and shoves follow on without a pause, as they do in play.
	auto Add = [this](EBeat Kind, ESokobanDirection Direction, float Seconds = 0.f) { Beats.Add({ Kind, Direction, Seconds }); };
	using D = ESokobanDirection;
	for (const D Way : { D::East, D::West })
	{
		const D Back = SokobanGrid::Opposite(Way);
		Add(EBeat::Wait, Way, 1.1f);
		Add(EBeat::Push, Way);
		Add(EBeat::Push, Way);
		Add(EBeat::Wait, Way, 0.35f);
		Add(EBeat::Cheer, D::South, CheerSeconds);
		Add(EBeat::Walk, D::South);
		Add(EBeat::Walk, Way);
		Add(EBeat::Walk, Way);
		Add(EBeat::Walk, Way);
		Add(EBeat::Walk, D::North);
		Add(EBeat::Wait, Back, 0.4f);
		// Face the crates again before the next shove.
		Add(EBeat::Walk, Back, -1.f);
	}
}

void ASokobanTitleStage::Show()
{
	UWorld* World = GetWorld();
	if (!Cat)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Cat = World->SpawnActor<ASokobanHero>(ASokobanHero::StaticClass(), CellToWorld(FIntPoint(0, 0)), FRotator::ZeroRotator, SpawnParams);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			Crates.Add(World->SpawnActor<ASokobanCrate>(ASokobanCrate::StaticClass(), FTransform(CellToWorld(FIntPoint(Index + 1, 0))), SpawnParams));
		}
		// Two more crates stacked off to the east, behind the show, waiting their turn.
		Crates.Add(World->SpawnActor<ASokobanCrate>(ASokobanCrate::StaticClass(), FTransform(FRotator(0.f, 12.f, 0.f), CellToWorld(FIntPoint(6, -1))), SpawnParams));
		Crates.Add(World->SpawnActor<ASokobanCrate>(ASokobanCrate::StaticClass(), FTransform(FRotator(0.f, -20.f, 0.f), CellToWorld(FIntPoint(6, -1)) + FVector(4.f, -3.f, 74.f)), SpawnParams));
	}

	CatCell = FIntPoint(0, 0);
	CrateCells = { FIntPoint(1, 0), FIntPoint(2, 0) };
	Cat->SnapTo(CellToWorld(CatCell));
	Cat->SetFacing(ESokobanDirection::East, true);
	Cat->PlayAction(ESokobanHeroAction::Idle);
	for (int32 Index = 0; Index < CrateCells.Num(); ++Index)
	{
		Crates[Index]->SnapTo(CellToWorld(CrateCells[Index]));
	}

	SetActorHiddenInGame(false);
	Cat->SetActorHiddenInGame(false);
	for (ASokobanCrate* Crate : Crates)
	{
		Crate->SetActorHiddenInGame(false);
	}

	if (TActorIterator<AExponentialHeightFog> Fog(World); Fog && SavedFogDensity < 0.f)
	{
		UExponentialHeightFogComponent* Component = Fog->GetComponent();
		SavedFogDensity = Component->FogDensity;
		SavedFogStartDistance = Component->StartDistance;
		SavedFogColor = Component->FogInscatteringLuminance;
		Component->SetFogDensity(FogDensity);
		Component->SetStartDistance(FogStartDistance);
		Component->SetFogInscatteringColor(FogColor);
	}

	bShown = true;
	bShoving = false;
	ShowStartTime = World->GetTimeSeconds();
	BeatEndTime = ShowStartTime;
	NextBeat = 0;
	PlaceCamera(0.0);
	SetActorTickEnabled(true);
}

void ASokobanTitleStage::Hide()
{
	bShown = false;
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	if (Cat)
	{
		Cat->SetActorHiddenInGame(true);
	}
	for (ASokobanCrate* Crate : Crates)
	{
		Crate->SetActorHiddenInGame(true);
	}
	if (TActorIterator<AExponentialHeightFog> Fog(GetWorld()); Fog && SavedFogDensity >= 0.f)
	{
		Fog->GetComponent()->SetFogDensity(SavedFogDensity);
		Fog->GetComponent()->SetStartDistance(SavedFogStartDistance);
		Fog->GetComponent()->SetFogInscatteringColor(SavedFogColor);
		SavedFogDensity = -1.f;
	}
}

void ASokobanTitleStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Now = GetWorld()->GetTimeSeconds();
	PlaceCamera(Now - ShowStartTime);
	while (Beats.Num() > 0 && Now >= BeatEndTime)
	{
		PlayBeat(Beats[NextBeat], BeatEndTime);
		NextBeat = (NextBeat + 1) % Beats.Num();
	}
}

void ASokobanTitleStage::PlayBeat(const FBeat& Beat, double StartTime)
{
	// The show is heard too, quietly, under the title music.
	USokobanAudioSubsystem* Audio = USokobanAudioSubsystem::Get(this);
	const float Late = static_cast<float>(FMath::Max(0.0, StartTime - GetWorld()->GetTimeSeconds()));
	switch (Beat.Kind)
	{
	case EBeat::Wait:
		bShoving = false;
		Cat->PlayAction(ESokobanHeroAction::Idle);
		BeatEndTime = StartTime + Beat.Seconds;
		break;

	case EBeat::Cheer:
		// Turn to the camera, arms up, and hop.
		bShoving = false;
		Cat->SetFacing(Beat.Direction, false);
		Cat->PlayAction(ESokobanHeroAction::Victory);
		Cat->Bump(FVector::UpVector, 35.f, 0.35f);
		BeatEndTime = StartTime + Beat.Seconds;
		break;

	case EBeat::Walk:
	{
		bShoving = false;
		Cat->SetFacing(Beat.Direction, false);
		if (Beat.Seconds < 0.f)
		{
			// Only a turn.
			BeatEndTime = StartTime + 0.25f;
			break;
		}
		FSokobanGlide Glide;
		Glide.Style = ESokobanGlideStyle::Steady;
		Glide.Duration = WalkSeconds;
		Glide.StartTime = StartTime;
		CatCell += SokobanGrid::Offset(Beat.Direction);
		Cat->MoveTo(CellToWorld(CatCell), Glide);
		Cat->PlayAction(ESokobanHeroAction::Walk, WalkSeconds);
		if (Audio)
		{
			Audio->Play(ESokobanSound::Step, 0.16f, 1.05f, Late + WalkSeconds * 0.4f);
		}
		BeatEndTime = StartTime + WalkSeconds;
		break;
	}

	case EBeat::Push:
	{
		// The pair slides as one row, cat and crates on the same glide; the first shove strains before it gives.
		FSokobanGlide Glide;
		Glide.Style = bShoving ? ESokobanGlideStyle::ShoveOn : ESokobanGlideStyle::Shove;
		Glide.Hold = bShoving ? 0.f : PushStrain;
		Glide.Duration = PushSeconds + Glide.Hold;
		Glide.StartTime = StartTime;
		const FIntPoint Step = SokobanGrid::Offset(Beat.Direction);
		CatCell += Step;
		Cat->SetFacing(Beat.Direction, false);
		Cat->MoveTo(CellToWorld(CatCell), Glide);
		Cat->PlayAction(ESokobanHeroAction::Push, Glide.Duration);
		for (int32 Index = 0; Index < CrateCells.Num(); ++Index)
		{
			CrateCells[Index] += Step;
			Crates[Index]->MoveTo(CellToWorld(CrateCells[Index]), Glide);
		}
		if (Audio)
		{
			Audio->Play(ESokobanSound::Push, 0.25f, 1.f, Late + Glide.Hold);
		}
		bShoving = true;
		BeatEndTime = StartTime + Glide.Duration;
		break;
	}
	}
}

void ASokobanTitleStage::PlaceCamera(double Time)
{
	// A slow sway from side to side, rising and sinking a little out of step with it.
	const float Phase = static_cast<float>(UE_TWO_PI * Time / SwayPeriod);
	const FRotator Heading(Pitch, Yaw + SwayYaw * FMath::Sin(Phase), 0.f);
	FVector Location = Focus - Heading.Vector() * Distance;
	Location.Z += SwayHeight * FMath::Sin(Phase * 0.7f + 1.f);
	Camera->SetWorldLocationAndRotation(Location, (Focus - Location).Rotation());
	Camera->SetFieldOfView(FieldOfView);
}

FVector ASokobanTitleStage::CellToWorld(const FIntPoint& Cell) const
{
	// Like the board, north (row - 1) is +X and east (column + 1) is +Y; the cat and crates stand on the dune.
	FVector Location = GetActorLocation() + StageOrigin + FVector(-Cell.Y * CellSize, Cell.X * CellSize, 0.f);
	const FVector Offset = (Location - GetActorLocation() - DuneCenter) / DuneRadii;
	const float Height = FMath::Sqrt(FMath::Max(0.f, 1.f - FMath::Square(Offset.X) - FMath::Square(Offset.Y)));
	Location.Z = GetActorLocation().Z + DuneCenter.Z + DuneRadii.Z * Height - 1.f;
	return Location;
}
