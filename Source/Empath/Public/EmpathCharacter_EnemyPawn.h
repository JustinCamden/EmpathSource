// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EmpathCharacter.h"
#include "EmpathCharacter_EnemyPawn.generated.h"

/**
 * 
 */
UCLASS()
class EMPATH_API AEmpathCharacter_EnemyPawn : public AEmpathCharacter
{
	GENERATED_BODY()
	
public:
	// Sets default values for this character's properties
	AEmpathCharacter_EnemyPawn(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category = EnemyPawnCharacter)
	void BeginAttack();

	UPROPERTY(EditAnywhere, Category = EnemyPawnCharacter)
	UCapsuleComponent* WeaponCollision = nullptr;

private:
	void OnWeaponOverlapBegin(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	void OnWeaponOverlapEnd(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
