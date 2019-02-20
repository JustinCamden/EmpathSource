// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathAIManager.h"
#include "EmpathAIController.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "EmpathPlayerCharacter.h"
#include "EmpathTypes.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

// Stats for UE Profiler
DECLARE_CYCLE_STAT(TEXT("AI Hearing Checks"), STAT_EMPATH_HearingChecks, STATGROUP_EMPATH_AIManager);

// Log categories
DEFINE_LOG_CATEGORY_STATIC(LogAIManager, Log, All);

const float AEmpathAIManager::HearingDisconnectDist = 500.0f;

// Sets default values
AEmpathAIManager::AEmpathAIManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// Initialize default settings
	PlayerAwarenessState = EEmpathPlayerAwarenessState::PresenceNotKnown;
	bPlayerHasEverBeenSeen = false;
	bIsPlayerLocationKnown = false;
	LostPlayerTimeThreshold = 0.5f;
	StartSearchingTimeThreshold = 3.0f;
}

void AEmpathAIManager::OnPlayerDied(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType)
{
	SetPlayerAwarenessState(EEmpathPlayerAwarenessState::PresenceNotKnown);
	bIsPlayerLocationKnown = false;
}

// Called when the game starts or when spawned
void AEmpathAIManager::BeginPlay()
{
	Super::BeginPlay();

	// Grab an AI controllers that were not already registered with us. Shouldn't ever happen, but just in case.
	for (AEmpathAIController* CurrAICon : TActorRange<AEmpathAIController>(GetWorld()))
	{
		CurrAICon->RegisterAIManager(this);
	}

	// Bind delegates to the player
	APlayerController* PlayerCon = GetWorld()->GetFirstPlayerController();
	if (PlayerCon)
	{
		APawn* PlayerPawn = PlayerCon->GetPawn();
		if (PlayerPawn)
		{
			AEmpathPlayerCharacter* VRPlayer = Cast<AEmpathPlayerCharacter>(PlayerPawn);
			if (VRPlayer)
			{
				VRPlayer->OnTeleportToLocation.AddDynamic(this, &AEmpathAIManager::OnPlayerTeleported);
				VRPlayer->OnDeath.AddDynamic(this, &AEmpathAIManager::OnPlayerDied);
			}
			else
			{
				UE_LOG(LogAIManager, Warning, TEXT("%s: ERROR: Player is not an Empath VR Character!"), *GetNameSafe(this));
			}
		}
		else
		{
			UE_LOG(LogAIManager, Warning, TEXT("%s: ERROR: Player not possessing pawn!"), *GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogAIManager, Warning, TEXT("%s: ERROR: Player not found!"), *GetNameSafe(this));
	}
}

// Called every frame
void AEmpathAIManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEmpathAIManager::CleanUpSecondaryTargets()
{
	for (int32 Idx = 0; Idx < SecondaryAttackTargets.Num(); ++Idx)
	{
		FSecondaryAttackTarget& AITarget = SecondaryAttackTargets[Idx];
		if (AITarget.IsValid() == false)
		{
			SecondaryAttackTargets.RemoveAtSwap(Idx, 1, true);
			--Idx;
		}
	}
}

void AEmpathAIManager::SetPlayerAwarenessState(const EEmpathPlayerAwarenessState NewAwarenessState)
{
	if (NewAwarenessState != PlayerAwarenessState)
	{
		EEmpathPlayerAwarenessState OldAwarenessState = PlayerAwarenessState;
		PlayerAwarenessState = NewAwarenessState;
		OnPlayerAwarenessStateChanged(PlayerAwarenessState, OldAwarenessState);
		OnNewPlayerAwarenessState.Broadcast(PlayerAwarenessState, NewAwarenessState);
	}
}

void AEmpathAIManager::AddSecondaryTarget(AActor* Target, float TargetRatio, float TargetPreference, float TargetRadius)
{
	CleanUpSecondaryTargets();

	if (Target)
	{
		FSecondaryAttackTarget NewAttackTarget;
		NewAttackTarget.TargetActor = Target;
		NewAttackTarget.TargetPreference= TargetPreference;
		NewAttackTarget.TargetingRatio = TargetRatio;
		NewAttackTarget.TargetRadius = TargetRadius;

		SecondaryAttackTargets.Add(NewAttackTarget);
	}
}

void AEmpathAIManager::RemoveSecondaryTarget(AActor* Target)
{
	for (int32 Idx = 0; Idx < SecondaryAttackTargets.Num(); ++Idx)
	{
		FSecondaryAttackTarget& AITarget = SecondaryAttackTargets[Idx];
		if (AITarget.TargetActor == Target)
		{
			SecondaryAttackTargets.RemoveAtSwap(Idx, 1, true);
			--Idx;
		}
	}
}

void AEmpathAIManager::GetNumAITargeting(AActor const* Target, int32& NumAITargetingCandiate, int32& NumTotalAI) const
{
	NumAITargetingCandiate = 0;
	NumTotalAI = 0;

	for (AEmpathAIController* AI : EmpathAICons)
	{
		++NumTotalAI;

		if (AI->GetAttackTarget() == Target)
		{
			++NumAITargetingCandiate;
		}
	}
}

float AEmpathAIManager::GetAttackTargetRadius(AActor* AttackTarget) const
{
	// Check if it is a secondary attack target
	for (FSecondaryAttackTarget const& CurrTarget : SecondaryAttackTargets)
	{
		if (CurrTarget.TargetActor == AttackTarget)
		{
			return CurrTarget.TargetRadius;
		}
	}

	return 0.0f;
}

void AEmpathAIManager::UpdateKnownTargetLocation(AActor const* Target)
{
	// If the target is a player
	const AEmpathPlayerCharacter* Player = Cast<AEmpathPlayerCharacter>(Target);
	if (Player)
	{
		// Update variables
		bIsPlayerLocationKnown = true;
		LastKnownPlayerLocation = Player->GetVRLocation();

		// Stop the "lost player" state flow
		SetPlayerAwarenessState(EEmpathPlayerAwarenessState::KnownLocation);
		GetWorldTimerManager().ClearTimer(LostPlayerTimerHandle);


		// If this is the first time the player has been seen, notify all active
		// AIs that the player is in the area
		if (bPlayerHasEverBeenSeen == false)
		{
			for (AEmpathAIController* AI : EmpathAICons)
			{
				AI->OnTargetSeenForFirstTime();
			}

			bPlayerHasEverBeenSeen = true;
		}
	}

	// Secondary targets are always known, so just ignore any other case
}

bool AEmpathAIManager::IsTargetLocationKnown(AActor const* Target) const
{
	if (Target)
	{
		// If this is the player, return whether the player's location is known.
		if (Cast<AEmpathPlayerCharacter>(Target) != nullptr)
		{
			return bIsPlayerLocationKnown;
		}

		// Else, return true for secondary targets
		for (FSecondaryAttackTarget const& CurrentTarget : SecondaryAttackTargets)
		{
			if (CurrentTarget.IsValid() && (CurrentTarget.TargetActor == Target))
			{
				// secondary targets are always known
				return true;
			}
		}
	}

	return false;
}

bool AEmpathAIManager::IsPlayerPotentiallyLost() const
{
	return PlayerAwarenessState == EEmpathPlayerAwarenessState::PotentiallyLost;
}

void AEmpathAIManager::OnPlayerTeleported(AActor* Player, FVector Origin, FVector Destination, FVector Direction)
{
	if (PlayerAwarenessState == EEmpathPlayerAwarenessState::KnownLocation)
	{
		SetPlayerAwarenessState(EEmpathPlayerAwarenessState::PotentiallyLost);

		// If this timer expires and no AI has found the player, he is officially "lost"
		GetWorldTimerManager().SetTimer(LostPlayerTimerHandle,
			FTimerDelegate::CreateUObject(this, &AEmpathAIManager::OnLostPlayerTimerExpired),
			LostPlayerTimeThreshold, false);
	}
}

void AEmpathAIManager::OnLostPlayerTimerExpired()
{
	switch (PlayerAwarenessState)
	{
	case EEmpathPlayerAwarenessState::KnownLocation:
		// Shouldn't happen, do nothing.
		break;

	case EEmpathPlayerAwarenessState::PotentiallyLost:
		// Move on to lost state
		SetPlayerAwarenessState(EEmpathPlayerAwarenessState::Lost);
		GetWorldTimerManager().SetTimer(LostPlayerTimerHandle,
			FTimerDelegate::CreateUObject(this, &AEmpathAIManager::OnLostPlayerTimerExpired),
			StartSearchingTimeThreshold, false);

		bIsPlayerLocationKnown = false;

		// Signal all AI to respond to losing player
		// #TODO flag the AI closest to players last known position
		for (AEmpathAIController* AI : EmpathAICons)
		{
			if (AI->IsAIRunning())
			{
				AI->OnLostPlayerTarget();
			}
		}

		break;

	case EEmpathPlayerAwarenessState::Lost:
		// Start searching
		SetPlayerAwarenessState(EEmpathPlayerAwarenessState::Searching);

		// Start searching
		// #TODO have flagged AI check player's last known position
		for (AEmpathAIController* AI : EmpathAICons)
		{
			if (AI->IsAIRunning())
			{
				AI->OnSearchForPlayerStarted();
			}
		}
		break;

	case EEmpathPlayerAwarenessState::Searching:
	{
		// Shouldn't happen, do nothing
		break;
	}

	break;

	}
}

void AEmpathAIManager::CheckForAwareAIs()
{
	if (bPlayerHasEverBeenSeen)
	{
		// Check whether any remaining AIs are active
		for (AEmpathAIController* AI : EmpathAICons)
		{
			if (!AI->IsPassive())
			{
				return;
			}
		}

		// If not, we are no longer aware of the player
		bPlayerHasEverBeenSeen = false;
		bIsPlayerLocationKnown = false;
		SetPlayerAwarenessState(EEmpathPlayerAwarenessState::PresenceNotKnown);
	}
	return;
}

void AEmpathAIManager::ReportNoise(AActor* NoiseInstigator, AActor* NoiseMaker, FVector Location, float HearingRadius)
{
	// Track how long it takes to complete this function for the profiler
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_HearingChecks);

	// Since secondary targets are currently always known,
	// we only care about this if the instigator is the player, and we are not currently aware of them
	if (PlayerAwarenessState == EEmpathPlayerAwarenessState::KnownLocation)
	{
		return;
	}
	AEmpathPlayerCharacter* VRCharNoiseInstigator = Cast<AEmpathPlayerCharacter>(NoiseInstigator);
	if (!VRCharNoiseInstigator)
	{
		VRCharNoiseInstigator = Cast<AEmpathPlayerCharacter>(NoiseInstigator->Instigator);
		if (!VRCharNoiseInstigator)
		{
			return;
		}
	}

	// Otherwise, loop through the AIs and see if any of them are in the hearing radius
	bool bHeardSound = false;
	for (AEmpathAIController* AI : EmpathAICons)
	{
		// If the AI is valid
		if (!AI->IsPassive() && !AI->IsDead())
		{
			// Check if it is within hearing distance
			float DistFromSound = (AI->GetPawn()->GetActorLocation() - Location).Size();
			if (DistFromSound <= HearingRadius)
			{
				// If so, end the loop
				bHeardSound = true;
				break;
			}
		}
	}

	// If we heard the sound
	if (bHeardSound)
	{
		// Check if it close enough to the instigator to be effectively the same location
		float InstigatorDist = (Location - VRCharNoiseInstigator->GetVRLocation()).Size();
		if (InstigatorDist <= HearingDisconnectDist)
		{
			// If so, we have found the target
			UpdateKnownTargetLocation(VRCharNoiseInstigator);
		}
		
		// TODO: Flag area for AI to investigate
	}
	return;
}