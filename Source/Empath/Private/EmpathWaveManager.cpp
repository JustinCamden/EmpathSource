// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathWaveManager.h"
#include "EmpathEnemySpawner.h"
#include "TimerManager.h"
#include "EmpathTrigger.h"

// Sets default values
AEmpathWaveManager::AEmpathWaveManager(const FObjectInitializer& ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	TimeBetweenWaves = 3.0f;
	bEndWaveOnEnemiesDead = true;

}

// Called when the game starts or when spawned
void AEmpathWaveManager::BeginPlay()
{
	Super::BeginPlay();
	if (SpawnTrigger)
	{
		SpawnTrigger->OnTriggeredDelegate.AddDynamic(this, &AEmpathWaveManager::OnSpawnTriggerTriggered);
	}
}


void AEmpathWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SpawnTrigger)
	{
		SpawnTrigger->OnTriggeredDelegate.RemoveDynamic(this, &AEmpathWaveManager::OnSpawnTriggerTriggered);
		SpawnTrigger = nullptr;
	}
	if (!bDead)
	{
		bDead = true;
		OnWaveManagerDeath();
		OnWaveManagerDeathDelegate.Broadcast(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AEmpathWaveManager::OnSpawnedEnemyDeath(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType)
{
	RemainingWaveEvents--;
	if (RemainingWaveEvents <= 0)
	{
		// Call events and notifies
		OnWaveCompleted();
		OnWaveCompletedDelegate.Broadcast(this);

		// If there are more waves, move on to the next one
		if (SpawnWaveIdx < SpawnWaves.Num() - 1)
		{
			SpawnWaveIdx++;
			if (TimeBetweenWaves > 0.0f)
			{
				GetWorldTimerManager().SetTimer(NextWaveTimerHandle, this, &AEmpathWaveManager::SpawnNextWave, TimeBetweenWaves, false);
			}
			else
			{
				SpawnWave();
			}
		}

		// Otherwise, queue wave manager death
		else
		{
			bDead = true;
			SetLifeSpan(0.0001f);
			OnWaveManagerDeath();
			OnWaveManagerDeathDelegate.Broadcast(this);
		}
	}
}

void AEmpathWaveManager::OnSpawnerDeath(AEmpathEnemySpawner* Spawner)
{
	RemainingWaveEvents--;
	if (RemainingWaveEvents <= 0)
	{
		// Call events and notifies
		OnWaveCompleted();
		OnWaveCompletedDelegate.Broadcast(this);

		// If there are more waves, move on to the next one
		if (SpawnWaveIdx < SpawnWaves.Num() - 1)
		{
			SpawnWaveIdx++;
			if (TimeBetweenWaves > 0.0f)
			{
				GetWorldTimerManager().SetTimer(NextWaveTimerHandle, this, &AEmpathWaveManager::SpawnNextWave, TimeBetweenWaves, false);
			}
			else
			{
				SpawnWave();
			}
		}

		// Otherwise, queue wave manager death
		else
		{
			bDead = true;
			SetLifeSpan(0.0001f);
			OnWaveManagerDeath();
			OnWaveManagerDeathDelegate.Broadcast(this);
		}
	}
}

bool AEmpathWaveManager::SpawnWave()
{
	if (!bDead)
	{
		// Ensure wave events are reset
		if (NextWaveTimerHandle.IsValid())
		{
			GetWorldTimerManager().ClearTimer(NextWaveTimerHandle);
			NextWaveTimerHandle.Invalidate();
		}

		// Check to see if the wave idx is in range
		if (SpawnWaveIdx < SpawnWaves.Num())
		{
			// Spawn the next wave
			TMap<AEmpathEnemySpawner*, FEmpathEnemyWaveNode>& CurrWave = SpawnWaves[SpawnWaveIdx].WaveNodes;
			for (auto& CurrSpawner : CurrWave)
			{
				if (CurrSpawner.Key)
				{
					AEmpathCharacter* SpawnedChar = CurrSpawner.Key->SpawnEnemy(CurrSpawner.Value.OverrideClassToSpawn, CurrSpawner.Value.bDieAfterSpawn);

					// Increment wave events if successful
					if (SpawnedChar)
					{

						// Bind appropriate event
						// Enemies dead tracking
						if (bEndWaveOnEnemiesDead)
						{
							SpawnedChar->OnDeath.AddDynamic(this, &AEmpathWaveManager::OnSpawnedEnemyDeath);
							RemainingWaveEvents++;
						}

						// Spawner dead tracking
						else if (!CurrSpawner.Key->IsDead())
						{
							CurrSpawner.Key->OnSpawnerDeathDelegate.AddDynamic(this, &AEmpathWaveManager::OnSpawnerDeath);
							RemainingWaveEvents++;
						}
					}
				}
			}
		}

		// If we didn't successfully spawn any enemies
		if (RemainingWaveEvents <= 0)
		{
			// If there are more waves, move on to the next one
			if (SpawnWaveIdx < SpawnWaves.Num() - 1)
			{
				SpawnWaveIdx++;
				if (TimeBetweenWaves > 0.0f)
				{
					GetWorldTimerManager().SetTimer(NextWaveTimerHandle, this, &AEmpathWaveManager::SpawnNextWave, TimeBetweenWaves, false);
				}
				else
				{
					SpawnWave();
				}
			}

			// Otherwise, queue wave manager death
			else
			{
				bDead = true;
				SetLifeSpan(0.0001f);
				OnWaveManagerDeath();
				OnWaveManagerDeathDelegate.Broadcast(this);
			}

			return false;
		}
		return true;
	}

	return false;
}

void AEmpathWaveManager::SpawnNextWave()
{
	SpawnWave();
	return;
}

void AEmpathWaveManager::OnSpawnTriggerTriggered(AEmpathTrigger* Trigger, AActor* TriggeringActor)
{
	SpawnWave();
	return;
}

