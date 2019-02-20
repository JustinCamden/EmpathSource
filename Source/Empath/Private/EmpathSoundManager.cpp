// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathSoundManager.h"
#include "EmpathAIManager.h"

// Sets default values
AEmpathSoundManager::AEmpathSoundManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEmpathSoundManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEmpathSoundManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEmpathSoundManager::RegisterAIManager(AEmpathAIManager* AIManagerToRegister)
{
	if (AIManager)
	{
		AIManager->OnNewPlayerAwarenessState.RemoveDynamic(this, &AEmpathSoundManager::OnNewPlayerAwarenessState);
	}
	AIManager = AIManagerToRegister;
	if (AIManager)
	{
		AIManager->OnNewPlayerAwarenessState.AddDynamic(this, &AEmpathSoundManager::OnNewPlayerAwarenessState);
		OnAIManagerRegistered();
	}
	return;
}

void AEmpathSoundManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AIManager)
	{
		AIManager->OnNewPlayerAwarenessState.RemoveDynamic(this, &AEmpathSoundManager::OnNewPlayerAwarenessState);
	}
	Super::EndPlay(EndPlayReason);
	return;
}

