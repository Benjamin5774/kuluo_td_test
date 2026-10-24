// Fill out your copyright notice in the Description page of Project Settings.

#include "Tools/SokobanEditorScripting.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"

bool USokobanEditorScripting::ConnectWorldPositionOffset(UMaterial* Material, UMaterialExpression* Expression, const FString& OutputName)
{
#if WITH_EDITOR
	if (!Material || !Expression)
	{
		return false;
	}

	FExpressionInput* Input = Material->GetExpressionInputForProperty(MP_WorldPositionOffset);
	const TArray<FExpressionOutput>& Outputs = Expression->GetOutputs();
	for (int32 Index = 0; Input && Index < Outputs.Num(); ++Index)
	{
		if (OutputName.IsEmpty() || Outputs[Index].OutputName.ToString() == OutputName)
		{
			Material->Modify();
			Input->Connect(Index, Expression);
			return true;
		}
	}
#endif
	return false;
}
