#include "TinyverseSaveIdentityComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tinyverse.h"
#include "TinyverseSaveSubsystem.h"

UTinyverseSaveIdentityComponent::UTinyverseSaveIdentityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTinyverseSaveIdentityComponent::MarkOwnerAsRemoved()
{
	if (!bPersistRemoval)
	{
		return;
	}
	
	if (!PersistentId.IsValid())
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(
			LogTinyverse,
			Warning,
			TEXT("%s non possiede un PersistentId valido"),
			*GetNameSafe(GetOwner()));
#endif
		return;
	}

	UTinyverseSaveSubsystem* SaveSubsystem = GetSaveSubsystem();

	if (!IsValid(SaveSubsystem))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(
			LogTinyverse,
			Warning,
			TEXT("SaveSubsystem non disponibile per %s"),
			*GetNameSafe(GetOwner()));
#endif
		return;
	}

	SaveSubsystem->MarkActorRemoved(PersistentId);
}

bool UTinyverseSaveIdentityComponent::IsOwnerMarkedAsRemoved() const
{
	if (!bPersistRemoval || !PersistentId.IsValid())
	{
		return false;
	}

	const UTinyverseSaveSubsystem* SaveSubsystem = GetSaveSubsystem();

	return IsValid(SaveSubsystem)
		&& SaveSubsystem->IsActorRemoved(PersistentId);
}

FGuid UTinyverseSaveIdentityComponent::GetPersistentId() const
{
	return PersistentId;
}

UTinyverseSaveSubsystem*
UTinyverseSaveIdentityComponent::GetSaveSubsystem() const
{
	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UTinyverseSaveSubsystem>();
}

void UTinyverseSaveIdentityComponent::RegeneratePersistentId()
{
#if WITH_EDITOR
	if (IsTemplate())
	{
		return;
	}

	Modify();
	PersistentId = FGuid::NewGuid();
	MarkPackageDirty();

	if (AActor* Owner = GetOwner())
	{
		Owner->Modify();
		Owner->MarkPackageDirty();
	}
#endif
}

#if WITH_EDITOR

void UTinyverseSaveIdentityComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	EnsurePersistentId();
}

void UTinyverseSaveIdentityComponent::OnRegister()
{
	Super::OnRegister();
	EnsurePersistentId();
}

void UTinyverseSaveIdentityComponent::PostDuplicate(
	EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	if (DuplicateMode == EDuplicateMode::Normal)
	{
		PersistentId = FGuid();
		EnsurePersistentId();
	}
}

void UTinyverseSaveIdentityComponent::PostEditImport()
{
	Super::PostEditImport();

	PersistentId = FGuid();
	EnsurePersistentId();
}

void UTinyverseSaveIdentityComponent::EnsurePersistentId()
{
	if (IsTemplate() || PersistentId.IsValid())
	{
		return;
	}

	const UWorld* World = GetWorld();

	if (!IsValid(World) || World->WorldType != EWorldType::Editor)
	{
		return;
	}

	RegeneratePersistentId();
}

#endif
