// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathDeflectableInterface.h"
#include "EmpathTeamAgentInterface.h"
#include "EmpathMeleeWeapon.generated.h"

class UEmpathDamageType;

UCLASS()
class EMPATH_API AEmpathMeleeWeapon : public AActor, public IEmpathDeflectableInterface, public IEmpathTeamAgentInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathMeleeWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** The team of the weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathMeleeWeapon, meta = (ExposeOnSpawn = "true"))
	EEmpathTeam Team;

	/** Returns the team number of the actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathMeleeWeapon)
	EEmpathTeam GetTeamNum() const;

	/** Whether the weapon can be deflected in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	bool bDeflectEnabled;

	/** Whether a particular component of the weapon can be deflected. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathMeleeWeapon)
	bool IsDeflectable(const UPrimitiveComponent* DeflectedComponent) const;

	/*
	* Overridable function for getting the impact impulse for this actor when impacting another actor.
	* By default, returns the forward vector of the actor.
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = EmpathMeleeWeapon)
	FVector GetImpactImpulse() const;

	/** Sets whether the weapon is currently active. */
	UFUNCTION(BlueprintCallable, Category = EmpathMeleeWeapon)
	void SetWeaponActive(const bool bNewActive);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Returns whether the weapon is currently active. */
	UFUNCTION(BlueprintCallable, Category = EmpathMeleeWeapon)
	const bool IsWeaponActive() const { return bWeaponActive; }

	/** Called when the weapon is activated. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathMeleeWeapon)
		void OnWeaponActivated();

	/** Called when the weapon is deactivated. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathMeleeWeapon)
		void OnWeaponDeactivated();

	/** Whether we can damage friendly targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathMeleeWeapon)
	bool bEnableFriendlyFire;

	/*
	* Returns whether we should trigger can impact event.
	* By default, true if the other actor is not friendly or friendly fire is enabled,
	* the impacting component is not deflectable while the impacted component is deflecting, 
	* and we should not ignore the collision event.
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = EmpathMeleeWeapon)
	bool ShouldTriggerImpact(const UPrimitiveComponent* ImpactingComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherActorComponent);

	/** Attempts to impact the inputted actor. If ShouldTriggerImpact is successful, it will call the OnImpact event. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = EmpathMeleeWeapon, meta = (AutoCreateRefTerm = "HitResult"))
	bool AttemptImpactActor(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, UPARAM(ref) const FHitResult& HitResult);

	/*
	* Event that is called when we successfully impact another actor.
	* By default, applies point damage to that actor, and triggers the die event.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = EmpathMeleeWeapon)
	void OnImpact(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FHitResult& HitResult);

	/*
	* Call on activation to attempt to impact any initial overlapping actors on the inputted component. 
	* Requires implementation of Sweep Impact Collision.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathMeleeWeapon)
	void ImpactInitialOverlappingActors(UPrimitiveComponent* ImpactingComponent);

	/*
	* Sweeps for overlapping actors across a component being used for impact collision.
	* Requires implementation. 
	*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = EmpathMeleeWeapon)
	TArray<FHitResult> SweepImpactCollision(const UPrimitiveComponent* ImpactingComponent);

	/** How long to wait before clearing the initial overlapped actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathMeleeWeapon)
	float InitialImpactedActorsClearTime;

	/** Time for clearing the initially impacted actors. */
	FTimerHandle ClearInitialImpactedActorsTimerHandle;

	/** Clears the initially impacted actors so that we can impact them again. */
	void ClearInitialImpactedActors();

	/** The amount of damage to inflict. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile, meta = (ExposeOnSpawn = "true"))
	float DamageAmount;

	/** The damage type of the projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	TSubclassOf<UEmpathDamageType> DamageTypeClass;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	/** Whether the weapon is currently active. */
	bool bWeaponActive;
	
	/** Actors that were overlapped and already impacted when we activated. Will be ignored when it comes to future impacts Should be cleared regularly. */
	TSet <AActor*> InitialImpactedActors;
};
