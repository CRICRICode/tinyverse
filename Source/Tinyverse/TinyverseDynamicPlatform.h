// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TinyverseDynamicPlatform.generated.h"

class UStaticMeshComponent;

UCLASS()
class TINYVERSE_API ATinyverseDynamicPlatform : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATinyverseDynamicPlatform();
	
	
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* Platform = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Platform Movement", meta=(MakeEditWidget))
	FVector MovementOffset = FVector(0.0f, 0.0f, 0.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Platform Movement", meta=(ClampMin="0.0"))
	float MovementSpeed = 100.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Platform Movement")
	FVector StartLocation;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Platform Movement")
	FVector EndLocation;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Platform Movement")
	bool bIsMovingToEnd = true;
	
	
	FTimerHandle PlatformWaitTimer;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Platform Movement", meta=(ClampMin="0.0"))
	float WaitDuration = 5.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	
	void PlatformSlide (
		float DeltaTime);
	
	void ResumePlatformMovement();
	
	
};