// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathMeleeWeapon.h"
#include "EmpathFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "EmpathDamageType.h"
#include "TimerManager.h"
#include "EmpathIgnoreCollisionInterface.h"
#include "EmpathDeflectorInterface.h"


// Sets default values
AEmpathMeleeWeapon::AEmpathMeleeWeapon(const FObjectInitializer& ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DamageAmount = 1.0f;
	DamageTypeClass = UEmpathDamageType::StaticClass();
	InitialImpactedActorsClearTime = 0.2f;
	Team = EEmpathTeam::Enemy;
}

// Called when the game starts or when spawned
void AEmpathMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AEmpathMeleeWeapon::SetWeaponActive(const bool bNewActive)
{
	if (bNewActive != bWeaponActive)
	{
		bWeaponActive = bNewActive;
		if (bWeaponActive)
		{
			OnWeaponActivated();
		}
		else
		{
			InitialImpactedActors.Empty();
			OnWeaponDeactivated();
		}
	}
	return;
}

// Called every frame
void AEmpathMeleeWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AEmpathMeleeWeapon::AttemptImpactActor(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, UPARAM(ref) const FHitResult& HitResult)
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

void AEmpathMeleeWeapon::ImpactInitialOverlappingActors(UPrimitiveComponent* ImpactingComponent)
{
	if (ImpactingComponent)
	{
		// Get each overlapping component
		TArray<FHitResult> InitialOverlappingHits = SweepImpactCollision(ImpactingComponent);
		for (FHitResult& CurrHit : InitialOverlappingHits)
		{
			// Get the owning actor of the hit and check if we have already impacted it
			AActor* CurrOwner = CurrHit.GetActor();
			if (CurrOwner && CurrOwner != this && !InitialImpactedActors.Contains(CurrOwner))
			{
				// Attempt to impact the hit
				UPrimitiveComponent* CurrComp = CurrHit.GetComponent();
				if (CurrComp)
				{
					// If successful, add the component to the list of those initially overlapped
					if (AttemptImpactActor(ImpactingComponent, CurrOwner, CurrComp, CurrHit))
					{
						InitialImpactedActors.Emplace(CurrOwner);
					}
				}
			}
		}
	}

	// Queue clearing initial impacted actors
	if (InitialImpactedActors.Num() > 0)
	{
		if (InitialImpactedActorsClearTime > 0.0f)
		{
			if (!ClearInitialImpactedActorsTimerHandle.IsValid())
			{
				GetWorldTimerManager().SetTimer(ClearInitialImpactedActorsTimerHandle, this, &AEmpathMeleeWeapon::ClearInitialImpactedActors, InitialImpactedActorsClearTime, false);
			}
		}
		else
		{
			InitialImpactedActors.Empty();
		}
	}

	return;
}

void AEmpathMeleeWeapon::ClearInitialImpactedActors()
{
	InitialImpactedActors.Empty();
}

EEmpathTeam AEmpathMeleeWeapon::GetTeamNum_Implementation() const
{
	return Team;
}

FVector AEmpathMeleeWeapon::GetImpactImpulse_Implementation() const
{
	return GetActorForwardVector();
}

bool AEmpathMeleeWeapon::IsDeflectable_Implementation(const UPrimitiveComponent* DeflectedComponent) const
{
	// By default, return true if deflect is enabled and we are not dead.
	if (DeflectedComponent)
	{
		return (bDeflectEnabled && DeflectedComponent->GetOwner() == this);
	}
	return false;
}

bool AEmpathMeleeWeapon::ShouldTriggerImpact_Implementation(const UPrimitiveComponent* ImpactingComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherActorComponent)
{
	// Ensure that input is valid
	if (!OtherActor || !OtherActorComponent || !ImpactingComponent)
	{
		return false;
	}
	if (OtherActorComponent->GetOwner() != OtherActor || ImpactingComponent->GetOwner() != this)
	{
		return false;
	}
	if (InitialImpactedActors.Num() > 0)
	{
		AActor* const MutableOther = const_cast<AActor*>(OtherActor);
		if (InitialImpactedActors.Contains(MutableOther))
		{
			return false;
		}
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

void AEmpathMeleeWeapon::OnImpact_Implementation(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FHitResult& HitResult)
{
	// By default, apply point damage to the other actor and trigger the die event
	FPointDamageEvent DamageEvent(DamageAmount, HitResult, GetImpactImpulse(), DamageTypeClass);
	OtherActor->TakeDamage(DamageAmount, DamageEvent, GetInstigatorController(), this);

	return;
}