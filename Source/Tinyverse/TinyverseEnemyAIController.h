#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TinyverseEnemyAIController.generated.h"

class UBehaviorTree;

UCLASS(Blueprintable)
class TINYVERSE_API ATinyverseEnemyAIController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset = nullptr;
};