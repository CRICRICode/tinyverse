// Fill out your copyright notice in the Description page of Project Settings.


#include "TinyverseEnemyAIController.h"
#include "TinyverseEnemy.h"
#include "BehaviorTree/BehaviorTree.h"

void ATinyverseEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ATinyverseEnemy* Enemy = Cast<ATinyverseEnemy>(InPawn);

	if (!IsValid(Enemy))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("EnemyAICOntroller ha posseduto un Pawn non valido!"));
		return;
	}

	if (!IsValid(BehaviorTreeAsset))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Nessun Beahvior Tree assegnato a %s"),
			*GetNameSafe(this));
		return;
	}

	if (!RunBehaviorTree(BehaviorTreeAsset))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Impossibile avviare il Behavior Tree"));

		return;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Behavior Tree avviato per %s"),
		*GetNameSafe(Enemy));
}
