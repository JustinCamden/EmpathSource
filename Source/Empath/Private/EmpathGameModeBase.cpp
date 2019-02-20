// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathGameModeBase.h"
#include "EmpathAIManager.h"
#include "EmpathBishopManager.h"
#include "EmpathTimeDilator.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "TimerManager.h"
#include "EmpathGameSettings.h"
#include "EmpathGameInstance.h"
#include "Classes/GameFramework/WorldSettings.h"
#include "EmpathSoundManager.h"

AEmpathGameModeBase::AEmpathGameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIManagerClass = AEmpathAIManager::StaticClass();
	BishopManagerClass = AEmpathBishopManager::StaticClass();
	TimeDilatorClass = AEmpathTimeDilator::StaticClass();
	SoundManagerClass = AEmpathSoundManager::StaticClass();
	TimeBetweenPulses = 0.1f;
}

void AEmpathGameModeBase::BeginPlay()
{
	if (TimeDilatorClass)
	{
		TimeDilator = GetWorld()->SpawnActor<AEmpathTimeDilator>(TimeDilatorClass);
	}

	if (AIManagerClass)
	{
		AIManager = GetWorld()->SpawnActor<AEmpathAIManager>(AIManagerClass);
	}

	if (SoundManagerClass)
	{
		SoundManager = GetWorld()->SpawnActor<AEmpathSoundManager>(SoundManagerClass);
		if (AIManager)
		{
			SoundManager->RegisterAIManager(AIManager);
		}
	}

	if (BishopManagerClass)
	{
		BishopManager = GetWorld()->SpawnActor<AEmpathBishopManager>(BishopManagerClass);
	}

	if (TimeBetweenPulses >= 0.01f)
	{
		GetWorldTimerManager().SetTimer(PulseTimerHandle, this, &AEmpathGameModeBase::Pulse, TimeBetweenPulses, true);
	}

	if (UEmpathGameInstance* EGI = Cast<UEmpathGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (EGI->LoadedSettings)
		{
			GetWorld()->GetWorldSettings()->WorldToMeters = EGI->LoadedSettings->CalibratedWorldMetersScale;
		}
	}

	Super::BeginPlay();
}

void AEmpathGameModeBase::Pulse()
{
	if (UWorld* World = GetWorld())
	{
		float PulseDeltaTime = World->TimeSince(LastPulseTimeStamp);
		LastPulseTimeStamp = World->GetTimeSeconds();
		ReceivePulse(PulseDeltaTime);
		OnPulse.Broadcast(PulseDeltaTime);
	}


}
