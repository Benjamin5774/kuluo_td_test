// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SokobanEditorScripting.generated.h"

class UMaterial;
class UMaterialExpression;

/** Helpers for the editor Python scripts that build the project's materials. */
UCLASS()
class SOKOBANDEMO_API USokobanEditorScripting : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Wires an expression's output (by name; empty takes the first) into a material's World Position Offset.
	 * Python cannot reach that input: EMaterialProperty hides it, so MaterialEditingLibrary.connect_material_property
	 * has no value to name it by. Works in the editor only; returns false elsewhere or when nothing matched.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sokoban|Editor Scripting")
	static bool ConnectWorldPositionOffset(UMaterial* Material, UMaterialExpression* Expression, const FString& OutputName);
};
