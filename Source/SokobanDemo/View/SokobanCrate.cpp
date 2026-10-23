// Fill out your copyright notice in the Description page of Project Settings.

#include "View/SokobanCrate.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASokobanCrate::ASokobanCrate()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolidMaterialAsset(TEXT("/Game/Sokoban/Materials/M_SokobanSolid.M_SokobanSolid"));
	SolidMaterial = SolidMaterialAsset.Object;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(VisualRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetStaticMesh(CubeMesh.Object);
	// The basic cube is 100 units wide with its pivot at the centre: sit a 78-unit crate on the floor.
	Mesh->SetRelativeScale3D(FVector(0.78f));
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 39.f));
}

void ASokobanCrate::BeginPlay()
{
	Super::BeginPlay();

	Material = Mesh->CreateDynamicMaterialInstance(0, SolidMaterial);
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
	if (UMaterialInstanceDynamic* Look = Target->CreateDynamicMaterialInstance(0, SolidMaterial))
	{
		Look->SetVectorParameterValue(TEXT("Color"), Color);
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
		Material->SetVectorParameterValue(TEXT("Color"), bOnGoal ? OnGoalColor : Color);
		Material->SetScalarParameterValue(TEXT("Glow"), bOnGoal ? OnGoalGlow : 0.f);
	}
}
