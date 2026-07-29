// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TinyverseSaveGame.generated.h"

UCLASS()
class TINYVERSE_API UTinyverseSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	int32 SaveVersion = 1;

	UPROPERTY(SaveGame)
	FString SavedLevelPath;

	UPROPERTY(SaveGame)
	FTransform PlayerTransform = FTransform::Identity;

	UPROPERTY(SaveGame)
	float CurrentHealth = 3.0f;

	UPROPERTY(SaveGame)
	float MaxHealth = 3.0f;

	UPROPERTY(SaveGame)
	int32 CoinCount = 0;
};
