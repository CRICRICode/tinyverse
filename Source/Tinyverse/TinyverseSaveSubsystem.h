// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPtr.h"
#include "TinyverseSaveSubsystem.generated.h"

class ATinyverseCharacter;
class UTinyverseSaveGame;
class UWorld;

UCLASS(BlueprintType)
class TINYVERSE_API UTinyverseSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Tinyverse|Save")
	bool SaveCurrentGame();

	UFUNCTION(BlueprintCallable, Category="Tinyverse|Save")
	bool StartNewGame(TSoftObjectPtr<UWorld> NewGameLevel);

	UFUNCTION(BlueprintCallable, Category="Tinyverse|Save")
	bool ContinueGame();

	UFUNCTION(BlueprintPure, Category="Tinyverse|Save")
	bool DoesSaveExist() const;

	UFUNCTION(BlueprintCallable, Category="Tinyverse|Save")
	void MarkActorRemoved(const FGuid& PersistentId);

	UFUNCTION(BlueprintPure, Category="Tinyverse|Save")
	bool IsActorRemoved(const FGuid& PersistentId) const;
	
	void TryRestorePlayer(ATinyverseCharacter* PlayerCharacter);

private:
	static const FString SaveSlotName;
	static constexpr int32 SaveUserIndex = 0;
	static constexpr int32 CurrentSaveVersion = 2;
	bool bCreateInitialSavePending = false;
	bool SavePlayerState(ATinyverseCharacter* PlayerCharacter);
	
	UPROPERTY(Transient)
	TSet<FGuid> RuntimeRemovedActorIds;

	UPROPERTY(Transient)
	TObjectPtr<UTinyverseSaveGame> PendingSave;
};
