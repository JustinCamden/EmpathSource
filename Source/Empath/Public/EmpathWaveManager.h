// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathCharacter.h"
#include "EmpathWaveManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveManagerDeathDelegate, AEmpathWaveManager*, WaveManager);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveCompletedDelegate, AEmpathWaveManager*, WaveManager);

class AEmpathEnemySpawner;
class AEmpathTrigger;

USTRUCT(BlueprintType)
struct FEmpathEnemyWaveNode
{
	GENERATED_USTRUCT_BODY();

	/** Override class to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AEmpathCharacter> OverrideClassToSpawn;

	/** Whether the spawner should die after this spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bDieAfterSpawn;

};

USTRUCT(BlueprintType)
struct FEmpathEnemyWave
{
	GENERATED_USTRUCT_BODY();

	/** Spawners and corresponding classes to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<AEmpathEnemySpawner*, FEmpathEnemyWaveNode> WaveNodes;

};

UCLASS()
class EMPATH_API AEmpathWaveManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathWaveManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Spawns the next wave if appropriate. */
	UFUNCTION(BlueprintCallable, Category = EmpathWaveManager)
	bool SpawnWave();

	/** List of enemy spawns by wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathWaveManager)
	TArray<FEmpathEnemyWave> SpawnWaves;

	/*
	* Whether to end the wave when all spawned enemies are dead. 
	* If false, will instead use when all spawners are dead. 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathWaveManager)
	bool bEndWaveOnEnemiesDead;

	/** Time between the end of the last wave and the start of the next wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathWaveManager)
	float TimeBetweenWaves;

	/** Timer for starting the next wave. */
	FTimerHandle NextWaveTimerHandle;

	/** Called when the wave manager dies. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathWaveManager)
	void OnWaveManagerDeath();

	/** Called when the wave manager dies. */
	UPROPERTY(BlueprintAssignable, Category = EmpathWaveManager)
	FOnWaveManagerDeathDelegate OnWaveManagerDeathDelegate;

	/** Called when a wave completes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathWaveManager)
	void OnWaveCompleted();

	/** Called when a wave completes. */
	UPROPERTY(BlueprintAssignable, Category = EmpathWaveManager)
	FOnWaveCompletedDelegate OnWaveCompletedDelegate;

	// Called to spawn the next wave
	void SpawnNextWave();

	/** Optional trigger for beginning to spawn waves of enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathWaveManager)
		AEmpathTrigger* SpawnTrigger;

	/** Called when the spawn trigger is entered.*/
	void OnSpawnTriggerTriggered(AEmpathTrigger* Trigger, AActor* TriggeringActor);

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The current wave index. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathWaveManager)
	int32 SpawnWaveIdx;

	/*
	* The remaining number of events before the end of the wave. 
	* If EndWaveOnEnemiesDead, this is the number of spawned enemies that still need to die.
	* Otherwise, this is the number of spawners that still need to die.
	*/
	UPROPERTY(BlueprintReadOnly, Category = EmpathWaveManager)
	int32 RemainingWaveEvents;

	/** Called when a spawned enemy dies. */
	void OnSpawnedEnemyDeath(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType);

	/** Called when a controlled spawner dies. */
	void OnSpawnerDeath(AEmpathEnemySpawner* Spawner);

	/** Whether the wave manager is currently dead. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathWaveManager)
	bool bDead;
};
