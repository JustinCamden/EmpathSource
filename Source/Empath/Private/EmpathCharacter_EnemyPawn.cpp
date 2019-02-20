// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathCharacter_EnemyPawn.h"
#include "EmpathCharacterMovementComponent.h"
#include "EmpathPlayerCharacter.h"
#include "Runtime/Engine/Classes/Components/CapsuleComponent.h"
#include "Engine.h"

AEmpathCharacter_EnemyPawn::AEmpathCharacter_EnemyPawn(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UEmpathCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{

}

void AEmpathCharacter_EnemyPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AEmpathCharacter_EnemyPawn::BeginAttack() {
	//TODO: Play animation

	//TODO: Activate weapon collision
}


void AEmpathCharacter_EnemyPawn::OnWeaponOverlapBegin(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) {
	
}

void AEmpathCharacter_EnemyPawn::OnWeaponOverlapEnd(class UPrimitiveComponent* HitComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {

}