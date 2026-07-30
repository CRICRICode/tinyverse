// Copyright Epic Games, Inc. All Rights Reserved.

#include "TinyverseSaveSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tinyverse.h"
#include "TinyverseCharacter.h"
#include "TinyverseHealthComponent.h"
#include "TinyverseSaveGame.h"
#include "UObject/Package.h"

const FString UTinyverseSaveSubsystem::SaveSlotName = TEXT("TinyverseProgress");

bool UTinyverseSaveSubsystem::SaveCurrentGame()
{
	ATinyverseCharacter* PlayerCharacter = Cast<ATinyverseCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	return SavePlayerState(PlayerCharacter);
}

bool UTinyverseSaveSubsystem::SavePlayerState(ATinyverseCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter)
		|| !IsValid(PlayerCharacter->GetHealthComponent())
		|| !IsValid(PlayerCharacter->GetWorld()))
	{
		return false;
	}

	UTinyverseSaveGame* SaveData = Cast<UTinyverseSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTinyverseSaveGame::StaticClass()));

	if (!IsValid(SaveData))
	{
		return false;
	}

	SaveData->SaveVersion = CurrentSaveVersion;
	SaveData->SavedLevelPath = UWorld::RemovePIEPrefix(
		PlayerCharacter->GetWorld()->GetPackage()->GetName());
	SaveData->PlayerTransform = PlayerCharacter->GetActorTransform();
	SaveData->CurrentHealth = PlayerCharacter->GetHealthComponent()->GetCurrentHealth();
	SaveData->MaxHealth = PlayerCharacter->GetHealthComponent()->GetMaxHealth();
	SaveData->CoinCount = PlayerCharacter->GetCoinCount();
	SaveData->RemoveActorIds = RuntimeRemovedActorIds;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(
		SaveData,
		SaveSlotName,
		SaveUserIndex);

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTinyverse,
		Display,
		TEXT("Salvataggio Tinyverse %s: livello=%s, monete=%d, HP=%.0f/%.0f"),
		bSaved ? TEXT("completato") : TEXT("fallito"),
		*SaveData->SavedLevelPath,
		SaveData->CoinCount,
		SaveData->CurrentHealth,
		SaveData->MaxHealth);
#endif

	return bSaved;
}

bool UTinyverseSaveSubsystem::StartNewGame(TSoftObjectPtr<UWorld> NewGameLevel)
{
	if (NewGameLevel.IsNull())
	{
		return false;
	}

	PendingSave = nullptr;

	if (DoesSaveExist())
	{
		UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	}

	UGameplayStatics::SetGamePaused(this, false);

	bCreateInitialSavePending = true;
	RuntimeRemovedActorIds.Reset();
	
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, NewGameLevel);
	return true;
}

bool UTinyverseSaveSubsystem::ContinueGame()
{
	bCreateInitialSavePending = false;

	UTinyverseSaveGame* SaveData = Cast<UTinyverseSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));

	if (!IsValid(SaveData)
		|| SaveData->SaveVersion > CurrentSaveVersion
		|| SaveData->SavedLevelPath.IsEmpty())
	{
		PendingSave = nullptr;
		return false;
	}

	PendingSave = SaveData;
	UGameplayStatics::SetGamePaused(this, false);
	RuntimeRemovedActorIds = SaveData->RemoveActorIds;
	UGameplayStatics::OpenLevel(this, FName(*SaveData->SavedLevelPath));
	return true;
}

bool UTinyverseSaveSubsystem::DoesSaveExist() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

void UTinyverseSaveSubsystem::TryRestorePlayer(
	ATinyverseCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter)
	|| !IsValid(PlayerCharacter->GetWorld()))
	{
		return;
	}

	if (bCreateInitialSavePending)
	{
		bCreateInitialSavePending = false;
		SavePlayerState(PlayerCharacter);
		return;
	}

	if (!IsValid(PendingSave))
	{
		return;
	}

	const FString CurrentLevelPath = UWorld::RemovePIEPrefix(
		PlayerCharacter->GetWorld()->GetPackage()->GetName());

	if (CurrentLevelPath != PendingSave->SavedLevelPath)
	{
		PendingSave = nullptr;
		return;
	}

	PlayerCharacter->SetActorTransform(
		PendingSave->PlayerTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	PlayerCharacter->RestoreCoinCount(PendingSave->CoinCount);

	if (UTinyverseHealthComponent* HealthComponent =
		PlayerCharacter->GetHealthComponent())
	{
		HealthComponent->RestoreHealthState(
			PendingSave->CurrentHealth,
			PendingSave->MaxHealth);
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTinyverse,
		Display,
		TEXT("Salvataggio applicato a %s: monete=%d, HP=%.0f/%.0f"),
		*GetNameSafe(PlayerCharacter),
		PendingSave->CoinCount,
		PendingSave->CurrentHealth,
		PendingSave->MaxHealth);
#endif

	PendingSave = nullptr;
}

void UTinyverseSaveSubsystem::MarkActorRemoved(const FGuid& PersistentId)
{
	if (!PersistentId.IsValid())
	{
		return;
	}
	
	RuntimeRemovedActorIds.Add(PersistentId);
}

bool UTinyverseSaveSubsystem::IsActorRemoved(const FGuid& PersistentId) const
{
	return PersistentId.IsValid() && RuntimeRemovedActorIds.Contains(PersistentId);
}
