// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AITypes.h"
#include "EmpathEnemySpawner.generated.h"

class AEmpathCharacter;
class UBoxComponent;
class UArrowComponent;
class UShapeComponent;
class AEmpathTrigger;

// Notify delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemySpawnedDelegate, AEmpathEnemySpawner*, Spawner, AEmpathCharacter*, SpawnedEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(FOnLastSpawnedEnemyDeathDelegate, AEmpathEnemySpawner*, Spawner, AEmpathCharacter*, LastSpawnedCharacter, FHitResult const&, KillingHitInfo, FVector, KillingHitImpulseDir, const AController*, DeathInstigator, const AActor*, DeathCauser, const UDamageType*, DeathDamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnerDeathDelegate, AEmpathEnemySpawner*, Spawner);

UCLASS()
class EMPATH_API AEmpathEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	

	static const FName RootCompName;

	// Sets default values for this actor's properties
	AEmpathEnemySpawner(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Determines whether the spawner should auto activate on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
		bool bSpawnOnBeginPlay;

	/** Class of enemy to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	TArray<TSubclassOf<AEmpathCharacter>> DefaultClassesToSpawn;

	/** The current spawn idx of classes to spawn. */
	int32 DefaultClassesToSpawnIdx;

	/** The location to spawn the enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	FVector SpawnLocation;

	/** The rotation to spawn the enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	FRotator SpawnRotation;

	/** Spawns the default enemy class, or the inputted override type if set. */
	UFUNCTION(BlueprintCallable, Category = EmpathEnemySpawner)
	AEmpathCharacter* SpawnEnemy(TSubclassOf<AEmpathCharacter> ClassToSpawnOverride = nullptr, bool bDestroyAfterSpawn = false);

	/** Called when an enemy is spawned. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathEnemySpawner)
	void OnEnemySpawned(AEmpathCharacter* SpawnedEnemy);

	/** Called when an enemy is spawned. */
	UPROPERTY(BlueprintAssignable, Category = EmpathEnemySpawner)
	FOnEnemySpawnedDelegate OnEnemySpawnedDelegate;

	/** The maximum number of enemies to spawn. Unlimited if <= 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	int32 MaxSpawnCount;

	/** Whether we should automatically respawn an enemy after the last spawned enemy dies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	bool bAutoRespawn;

	/** How long to wait after an enemy death to respawn the next enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	float TimeBetweenRespawns;

	/** Timer for respawn. */
	FTimerHandle RespawnEnemyTimerHandle;

	/** Function for re-spawning an enemy. */
	void RespawnEnemy();

	/** The last enemy spawned. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathEnemySpawner)
	AEmpathCharacter* LastSpawnedEnemy;

	/** Called when the last spawned character dies. */
	void OnLastSpawnedEnemyDeath(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType);

	/** Called when the last spawned character dies. */
	UPROPERTY(BlueprintAssignable, Category = EmpathEnemySpawner)
	FOnLastSpawnedEnemyDeathDelegate OnLastSpawnedEnemyDeathDelegate;

	/** Called when the last spawned character dies. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathEnemySpawner)
	void ReceiveLastSpawnedEnemyDeath(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType);

	/** Called when the spawner dies. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathEnemySpawner)
	void OnSpawnerDeath();

	/** Called when the spawner dies. */
	UPROPERTY(BlueprintAssignable, Category = EmpathEnemySpawner)
	FOnSpawnerDeathDelegate OnSpawnerDeathDelegate;

	/** Optional trigger for spawning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	AEmpathTrigger* TriggerVolume;

	/** Called when the trigger volume is triggered. */
	void OnTriggerVolumeTriggered(AEmpathTrigger* Trigger, AActor* TriggeringActor);

	/** Whether we should move to the move to location after spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	bool bUseMoveToLocation;

	/** The location for the spawned enemy to move to after being spawned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	FVector MoveToLocation;

	/** The acceptance radius for our move to location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	float MoveToAcceptanceRadius = -1.0f;

	/** Whether to stop on overlap when using the move to location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	TEnumAsByte<EAIOptionFlag::Type> MoveToStopOnOverlap;

	/** Whether to accept a partial path when using the move to location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	TEnumAsByte<EAIOptionFlag::Type> MoveToAcceptPartialPath;
	
	/** Whether to use pathfinding when using the move to location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	bool bMoveToUsePathfinding = true;

	/** Whether to block all AI logic when using the move to location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathEnemySpawner)
	bool bMoveToLockAILogic = true;

	/** Returns whether the spawner is currently dead. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathEnemySpawner)
	const bool IsDead() const { return bDead; }

	// Functions for updating the visualization
#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostRegisterAllComponents() override;
	void OnChildTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType TeleportType);
	void SyncDataToComponents();
	bool bRegisteredCallbacks;

#endif // WITH_EDITOR

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Whether this spawner is currently dead. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathEnemySpawner)
	bool bDead;

	/** The number of enemies spawned so far */
	UPROPERTY(BlueprintReadOnly, Category = EmpathEnemySpawner)
	int32 NumEnemiesSpawned;

private:
	/** The default root for the actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathEnemySpawner, meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootComp;

	// Visualization components
#if WITH_EDITORONLY_DATA

	/** Editor-only component to visualize spawn point. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathEnemySpawner, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* SpawnPoint;

	/** Editor-only component to visualize point spawned enemy to move to after spawn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathEnemySpawner, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* MoveToPoint;

	/** Editor-only component to visualize spawn rotation. */
	UPROPERTY()
	UArrowComponent* ArrowComponent;

#endif
};
