// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanCrate.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASokobanCrate::ASokobanCrate()
{
	// A wooden crate from the Quaternius Medieval Village MegaKit (CC0); its material has Tint and Glow.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CrateMesh(TEXT("/Game/Sokoban/Environment/Meshes/Prop_Crate.Prop_Crate"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(VisualRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetStaticMesh(CrateMesh.Object);
	// Authored 1.08 m wide with its pivot on the ground: a 78-unit crate.
	Mesh->SetRelativeScale3D(FVector(0.72f));
}

void ASokobanCrate::BeginPlay()
{
	Super::BeginPlay();

	Material = Mesh->CreateDynamicMaterialInstance(0);
	ApplyColor();
}

void ASokobanCrate::SetOnGoal(bool bInOnGoal, bool bInstant)
{
	bPendingOnGoal = bInOnGoal;
	if (bInstant || !IsMoving())
	{
		bOnGoal = bInOnGoal;
		ApplyColor();
	}
}

void ASokobanCrate::PaintLook(UPrimitiveComponent* Target) const
{
	if (UMaterialInstanceDynamic* Look = Target->CreateDynamicMaterialInstance(0, Mesh->GetMaterial(0)))
	{
		Look->SetVectorParameterValue(TEXT("Tint"), Tint);
		Look->SetScalarParameterValue(TEXT("Glow"), 0.f);
	}
}

void ASokobanCrate::OnMoveFinished()
{
	bOnGoal = bPendingOnGoal;
	ApplyColor();
}

void ASokobanCrate::ApplyColor()
{
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Tint"), bOnGoal ? OnGoalTint : Tint);
		Material->SetScalarParameterValue(TEXT("Glow"), bOnGoal ? OnGoalGlow : 0.f);
	}
}
