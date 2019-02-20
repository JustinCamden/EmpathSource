// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathEnemySpawner.h"
#include "EmpathCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "EmpathTrigger.h"

	const FName AEmpathEnemySpawner::RootCompName(TEXT("RootComp"));

AEmpathEnemySpawner::AEmpathEnemySpawner(const FObjectInitializer& ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FName ID_Characters;
		FText NAME_Characters;
		FConstructorStatics()
			: ID_Characters(TEXT("Characters"))
			, NAME_Characters(NSLOCTEXT("SpriteCategory", "Characters", "Characters"))
		{
		}
	};

	static FConstructorStatics ConstructorStatics;

	// Initialize variables
	PrimaryActorTick.bCanEverTick = true;
	MoveToLocation = FVector(150.0f, 0.0f, 0.0f);
	MoveToAcceptanceRadius = -1.0f;
	bMoveToUsePathfinding = true;
	MaxSpawnCount = 1;
	TimeBetweenRespawns = 3.0f;
	bSpawnOnBeginPlay = true;

	// Initialize root component
	RootComp = CreateDefaultSubobject<USceneComponent>(RootCompName);
	RootComponent = RootComp;

	// Initialize visualization components
#if WITH_EDITORONLY_DATA
	// Spawn point
	SpawnPoint = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnPoint"));
	SpawnPoint->InitBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	SpawnPoint->SetupAttachment(RootComponent);
	SpawnPoint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnPoint->SetGenerateOverlapEvents(false);

	// Arrow
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	ArrowComponent->ArrowColor = FColor(150, 200, 255);
	ArrowComponent->bTreatAsASprite = true;
	ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Characters;
	ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Characters;
	ArrowComponent->SetupAttachment(SpawnPoint);
	ArrowComponent->bIsScreenSizeScaled = true;

	// Move to point
	MoveToPoint = CreateDefaultSubobject<UBoxComponent>(TEXT("MoveToPoint"));
	MoveToPoint->InitBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	MoveToPoint->SetupAttachment(RootComponent);
	MoveToPoint->SetRelativeLocation(MoveToLocation);
	MoveToPoint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoveToPoint->SetGenerateOverlapEvents(false);

#endif // WITH_EDITORONLY_DATA

}

#if WITH_EDITORONLY_DATA
void AEmpathEnemySpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (SpawnPoint)
	{
		SpawnPoint->SetWorldLocation(SpawnLocation);
		SpawnPoint->SetWorldRotation(SpawnRotation);
	}
	if (MoveToPoint)
	{
		MoveToPoint->SetWorldLocation(MoveToLocation);
		MoveToPoint->SetVisibility(bUseMoveToLocation);
	}
	SyncDataToComponents();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITORONLY_DATA


#if WITH_EDITORONLY_DATA
void AEmpathEnemySpawner::PostEditMove(bool bFinished)
{	
	SyncDataToComponents();
}
#endif // WITH_EDITORONLY_DATA


#if WITH_EDITORONLY_DATA
void AEmpathEnemySpawner::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (!bRegisteredCallbacks && !IsTemplate())
	{
		// Listen to transform changes on child components.
		bRegisteredCallbacks = true;
		if (SpawnPoint)
		{
			SpawnPoint->TransformUpdated.AddUObject(this, &AEmpathEnemySpawner::OnChildTransformUpdated);
		}
		if (MoveToPoint)
		{
			MoveToPoint->TransformUpdated.AddUObject(this, &AEmpathEnemySpawner::OnChildTransformUpdated);
		}
	}
}
#endif // WITH_EDITORONLY_DATA


#if WITH_EDITORONLY_DATA
void AEmpathEnemySpawner::OnChildTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType TeleportType)
{
	SyncDataToComponents();
}
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITORONLY_DATA
void AEmpathEnemySpawner::SyncDataToComponents()
{

	if (SpawnPoint)
	{
		SpawnLocation = SpawnPoint->GetComponentTransform().GetLocation();
		SpawnRotation = FRotator(SpawnPoint->GetComponentTransform().GetRotation());
		MoveToLocation = MoveToPoint->GetComponentTransform().GetLocation();
	}

	return;
}

void AEmpathEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerVolume)
	{
		TriggerVolume->OnTriggeredDelegate.AddDynamic(this, &AEmpathEnemySpawner::OnTriggerVolumeTriggered);
	}

	if (bSpawnOnBeginPlay)
	{
		SpawnEnemy(nullptr, false);
	}
}

void AEmpathEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TriggerVolume)
	{
		TriggerVolume->OnTriggeredDelegate.RemoveDynamic(this, &AEmpathEnemySpawner::OnTriggerVolumeTriggered);
	}
	if (LastSpawnedEnemy)
	{
		LastSpawnedEnemy->OnDeath.RemoveDynamic(this, &AEmpathEnemySpawner::OnLastSpawnedEnemyDeath);
	}
	if (!bDead)
	{
		OnSpawnerDeath();
		OnSpawnerDeathDelegate.Broadcast(this);
	}
	Super::EndPlay(EndPlayReason);
}

#endif // WITH_EDITORONLY_DATA

AEmpathCharacter* AEmpathEnemySpawner::SpawnEnemy(TSubclassOf<AEmpathCharacter> ClassToSpawnOverride, bool bDestroyAfterSpawn)
{
	if (!bDead)
	{
		// Ensure respawn timer is nullified if active
		if (RespawnEnemyTimerHandle.IsValid())
		{
			GetWorldTimerManager().ClearTimer(RespawnEnemyTimerHandle);
			RespawnEnemyTimerHandle.Invalidate();
		}

		// Update number of spawns and whether we should die
		bool bShouldDie = bDestroyAfterSpawn;
		if (MaxSpawnCount > 0)
		{
			NumEnemiesSpawned++;
			if (NumEnemiesSpawned >= MaxSpawnCount)
			{
				bShouldDie = true;
			}
		}

		// Queue death if appropriate
		if (bShouldDie)
		{
			bDead = true;
			OnSpawnerDeath();
			OnSpawnerDeathDelegate.Broadcast(this);
			if (TriggerVolume)
			{
				TriggerVolume->OnTriggeredDelegate.RemoveDynamic(this, &AEmpathEnemySpawner::OnTriggerVolumeTriggered);
				TriggerVolume = nullptr;
			}

			SetLifeSpan(0.0001f);
		}

		AEmpathCharacter* SpawnedEnemy = nullptr;

		// Spawn character if valid
		if (ClassToSpawnOverride || (DefaultClassesToSpawnIdx < DefaultClassesToSpawn.Num() && DefaultClassesToSpawn[DefaultClassesToSpawnIdx]))
		{
			// Setup spawn Variables
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(SpawnLocation);
			SpawnTransform.SetRotation(FQuat(SpawnRotation));
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			// Set the class to spawn
			TSubclassOf<AEmpathCharacter> ClassToSpawn;
			if (ClassToSpawnOverride)
			{
				ClassToSpawn = ClassToSpawnOverride;
			}
			else
			{
				ClassToSpawn = DefaultClassesToSpawn[DefaultClassesToSpawnIdx];
				DefaultClassesToSpawnIdx = (DefaultClassesToSpawnIdx < DefaultClassesToSpawn.Num() - 1 ? DefaultClassesToSpawnIdx + 1 : 0);
			}

			// Spawn the object
			SpawnedEnemy = GetWorld()->SpawnActor<AEmpathCharacter>(ClassToSpawn, SpawnTransform, SpawnParams);

			// Order move if appropriate
			if (bUseMoveToLocation && SpawnedEnemy)
			{
				SpawnedEnemy->RequestMoveTo(MoveToLocation, nullptr, MoveToAcceptanceRadius, MoveToStopOnOverlap, MoveToStopOnOverlap, bMoveToUsePathfinding, bMoveToLockAILogic, false);
			}

			// Broadcast delegates
			OnEnemySpawned(SpawnedEnemy);
			OnEnemySpawnedDelegate.Broadcast(this, SpawnedEnemy);
		}

		// Cache last spawned character
		LastSpawnedEnemy = SpawnedEnemy;

		// Bind events if appropriate
		if (!bDead && LastSpawnedEnemy)
		{
			LastSpawnedEnemy->OnDeath.AddDynamic(this, &AEmpathEnemySpawner::OnLastSpawnedEnemyDeath);
		}

		return SpawnedEnemy;
	}

	return nullptr;
}

void AEmpathEnemySpawner::RespawnEnemy()
{
	SpawnEnemy(nullptr, false);

	return;
}

void AEmpathEnemySpawner::OnLastSpawnedEnemyDeath(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType)
{
	// Fire notifies and unbind events
	OnLastSpawnedEnemyDeathDelegate.Broadcast(this, LastSpawnedEnemy, KillingHitInfo, KillingHitImpulseDir, DeathInstigator, DeathCauser, DeathDamageType);
	ReceiveLastSpawnedEnemyDeath(KillingHitInfo, KillingHitImpulseDir, DeathInstigator, DeathCauser, DeathDamageType);
	if (LastSpawnedEnemy)
	{
		LastSpawnedEnemy->OnDeath.RemoveDynamic(this, &AEmpathEnemySpawner::OnLastSpawnedEnemyDeath);
	}
	LastSpawnedEnemy = nullptr;

	// Respawn if appropriate
	if (bAutoRespawn)
	{
		if (TimeBetweenRespawns > 0.0f)
		{
			GetWorldTimerManager().SetTimer(RespawnEnemyTimerHandle, this, &AEmpathEnemySpawner::RespawnEnemy, TimeBetweenRespawns, false);
		}
		else
		{
			SpawnEnemy(nullptr, false);
		}
	}

	return;
}

void AEmpathEnemySpawner::OnTriggerVolumeTriggered(AEmpathTrigger* Trigger, AActor* TriggeringActor)
{
	SpawnEnemy(nullptr, false);

	return;
}
