#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TinyverseSaveIdentityComponent.generated.h"

class UTinyverseSaveSubsystem;

UCLASS(
	ClassGroup=(Tinyverse),
	meta=(BlueprintSpawnableComponent))
class TINYVERSE_API UTinyverseSaveIdentityComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UTinyverseSaveIdentityComponent();

	UFUNCTION(BlueprintCallable, Category="Tinyverse|Save")
	void MarkOwnerAsRemoved();

	UFUNCTION(BlueprintPure, Category="Tinyverse|Save")
	bool IsOwnerMarkedAsRemoved() const;

	UFUNCTION(BlueprintPure, Category="Tinyverse|Save")
	FGuid GetPersistentId() const;

	UFUNCTION(CallInEditor, Category="Tinyverse|Save")
	void RegeneratePersistentId();

#if WITH_EDITOR
	void OnRegister() override;
	void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	void OnComponentCreated() override;
	void PostEditImport() override;
#endif

private:
	UTinyverseSaveSubsystem* GetSaveSubsystem() const;

#if WITH_EDITOR
	void EnsurePersistentId();
#endif

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category="Tinyverse|Save",
		meta=(AllowPrivateAccess="true"))
	FGuid PersistentId;
	
	UPROPERTY(
	EditAnywhere,
	BlueprintReadOnly,
	Category="Tinyverse|Save",
	meta=(
		AllowPrivateAccess="true",
		DisplayName="Persist Removal Between Loads"))
	bool bPersistRemoval = true;
};