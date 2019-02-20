
#include "EmpathProjectile.h"
#include "EmpathFunctionLibrary.h"
#include "EmpathIgnoreCollisionInterface.h"
#include "EmpathDeflectorInterface.h"
#include "Classes/GameFramework/ProjectileMovementComponent.h"
#include "EmpathDamageType.h"
#include "TimerManager.h"
#include "Classes/Components/PrimitiveComponent.h"
#include "Classes/Kismet/GameplayStatics.h"

FName AEmpathProjectile::ProjectileMovementComponentName(TEXT("ProjMoveComp"));

AEmpathProjectile::AEmpathProjectile(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
 	// Enabling tick
	PrimaryActorTick.bCanEverTick = true;

	// Spawn and attach components
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(AEmpathProjectile::ProjectileMovementComponentName);
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->Velocity = FVector(1000.0f, 0.0f, 0.0f);

	// Initialize variales
	TimeBeforePostDeathCleanup = 1.0f;
	LifetimeAfterSpawn = 5.0f;
	ImpactDamageAmount = 1.0f;
	MaxHomingAngle = 0.0f;
	DamageTypeClass = UEmpathDamageType::StaticClass();
}

// Called when the game starts or when spawned
void AEmpathProjectile::BeginPlay()
{
	Super::BeginPlay();
	StartDeathTimer(LifetimeAfterSpawn);
}

// Called every frame
void AEmpathProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check whether we have overshot the target
	if (MaxHomingAngle > 0.0f && ProjectileMovement && ProjectileMovement->bIsHomingProjectile && ProjectileMovement->HomingTargetComponent.IsValid() && ProjectileMovement->Velocity.SizeSquared() > 0.0f)
	{
		if (FMath::RadiansToDegrees(FMath::Acos(GetActorForwardVector() | (ProjectileMovement->HomingTargetComponent->GetComponentLocation() - GetActorLocation()).GetSafeNormal())) > MaxHomingAngle)
		{
			ProjectileMovement->bIsHomingProjectile = false;
			ProjectileMovement->HomingTargetComponent = nullptr;
			ProjectileMovement->HomingAccelerationMagnitude = 0.0f;
			ProjectileMovement->SetVelocityInLocalSpace(FVector(ProjectileMovement->Velocity.Size(), 0.0f, 0.0f));
		}
	}
}

EEmpathTeam AEmpathProjectile::GetTeamNum_Implementation() const
{
	return Team;
}

bool AEmpathProjectile::IsDeflectable_Implementation(const UPrimitiveComponent* DeflectedComponent) const
{
	// By default, return true if deflect is enabled and we are not dead.
	if (DeflectedComponent)
	{
		return (!bDead && bDeflectEnabled && DeflectedComponent->GetOwner() == this);
	}
	return false;
}

void AEmpathProjectile::Die()
{
	CancelDeathTimer();
	if (!bDead)
	{
		bDead = true;
		ReceiveDie();
	}
}

void AEmpathProjectile::CancelPostDeathCleanupTimer()
{
	if (PostDeathCleanupTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(PostDeathCleanupTimerHandle);
	}
}

void AEmpathProjectile::StartDeathTimer(const float NewLifetime)
{
	CancelDeathTimer();
	if (NewLifetime >= 0.05f && !bDead)
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &AEmpathProjectile::Die, NewLifetime, false);
	}
}

void AEmpathProjectile::CancelDeathTimer()
{
	if (LifetimeTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	}
}

bool AEmpathProjectile::AttemptImpactActor(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FHitResult& HitResult)
{
	// Check if the conditions for impacting the other component are clear.
	if (ShouldTriggerImpact(ImpactingComponent, OtherActor, OtherActorComponent))
	{
		// If so, call the impact event
		OnImpact(ImpactingComponent, OtherActor, OtherActorComponent, HitResult);
		return true;
	}
	
	// Otherwise return false
	return false;
}

void AEmpathProjectile::LockOnTarget(USceneComponent* TargetComponent, float HomingAcceleration)
{
	if (!bDead && TargetComponent && HomingAcceleration > 0.0f && ProjectileMovement)
	{
		// Check if the target is angled to far away if appropriate
		if (MaxHomingAngle > 0.0f && FMath::RadiansToDegrees(FMath::Acos(GetActorForwardVector() | (TargetComponent->GetComponentLocation() - GetActorLocation()).GetSafeNormal()) > MaxHomingAngle))
		{
			return;
		}

		// Otherwise, update the projectile movement component and call events as appropriate
		ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
		ProjectileMovement->HomingTargetComponent = TargetComponent;
		ProjectileMovement->bIsHomingProjectile = true;
		OnTargetLocked(TargetComponent, HomingAcceleration);
	}

	return;
}

const float AEmpathProjectile::GetRemainingLifetime() const
{
	return (LifetimeTimerHandle.IsValid() ? GetWorldTimerManager().GetTimerRemaining(LifetimeTimerHandle) : 0.0f);
}

void AEmpathProjectile::OnImpact_Implementation(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FHitResult& HitResult)
{
	// Apply point damage to the hit actor
	if (ImpactDamageAmount > 0.0f)
	{
		FPointDamageEvent DamageEvent(ImpactDamageAmount, HitResult, GetActorForwardVector(), DamageTypeClass);
		OtherActor->TakeDamage(ImpactDamageAmount, DamageEvent, GetInstigatorController(), this);

	}

	// Apply radial damage to the hit actor
	if (RadialDamageAmount > 0.0f && RadialDamageRadius > 0.0f)
	{
		TArray<AActor*> IgnoredActors;
		UGameplayStatics::ApplyRadialDamage(this, 
			RadialDamageAmount, 
			GetActorLocation(), 
			RadialDamageRadius, 
			(RadialDamageTypeClass ? RadialDamageTypeClass : DamageTypeClass), 
			IgnoredActors, 
			this, 
			GetInstigatorController(), 
			!bUseRadialDamageFallOff, 
			ECC_Visibility);
	}

	Die();
	return;
}

void AEmpathProjectile::ReceiveDie_Implementation()
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		if (ProjectileMovement->bIsHomingProjectile)
		{
			ProjectileMovement->bIsHomingProjectile = false;
			ProjectileMovement->HomingTargetComponent = nullptr;
			ProjectileMovement->HomingAccelerationMagnitude = 0.0f;
		}
	}
	if (TimeBeforePostDeathCleanup < 0.05f)
	{
		PostDeathCleanup();
	}
	else
	{
		GetWorldTimerManager().SetTimer(PostDeathCleanupTimerHandle, this, &AEmpathProjectile::PostDeathCleanup, TimeBeforePostDeathCleanup, false);
	}
	return;
}

void AEmpathProjectile::PostDeathCleanup_Implementation()
{
	SetLifeSpan(0.001f);
}

bool AEmpathProjectile::ShouldTriggerImpact_Implementation(const UPrimitiveComponent* ImpactingComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherActorComponent)
{
	// Ensure that we are not dead and that input is valid
	if (bDead || !OtherActor || !OtherActorComponent || !ImpactingComponent)
	{
		return false;
	}
	if (OtherActorComponent->GetOwner() != OtherActor || ImpactingComponent->GetOwner() != this)
	{
		return false;
	}

	// Check if we should ignore collision with the other actor
	if (OtherActor->GetClass()->ImplementsInterface(UEmpathIgnoreCollisionInterface::StaticClass()))
	{
		if (IEmpathIgnoreCollisionInterface::Execute_ShouldOtherActorIgnoreCollision(OtherActor, OtherActorComponent))
		{
			return false;
		}
	}

	// Don't impact friendly actors unless friendly fire is enabled
	if (!bEnableFriendlyFire && UEmpathFunctionLibrary::GetActorTeam(OtherActor) == Team)
	{
		return false;
	}

	// Check if we are deflectable
	if (IEmpathDeflectableInterface::Execute_IsDeflectable(this, ImpactingComponent))
	{
		// If so check if the other actor will deflect us
		if (OtherActor->GetClass()->ImplementsInterface(UEmpathDeflectorInterface::StaticClass()))
		{
			if (IEmpathDeflectorInterface::Execute_IsDeflecting(OtherActor, OtherActorComponent))
			{
				return false;
			}
		}
	}

	// If all checks pass, return true
	return true;
}
