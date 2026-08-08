// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_TinyverseFootstep.h"

#include "Components/SkeletalMeshComponent.h"
#include "TinyverseCharacter.h"

void UAnimNotify_TinyverseFootstep::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	if (ATinyverseCharacter* Character =
		Cast<ATinyverseCharacter>(MeshComp->GetOwner()))
	{
		Character->PlayFootstep(FootSocketName);
	}
}

FString UAnimNotify_TinyverseFootstep::GetNotifyName_Implementation() const
{
	return FootSocketName.IsNone()
		? TEXT("Tinyverse Footstep")
		: FString::Printf(
			TEXT("Footstep (%s)"),
			*FootSocketName.ToString());
}
