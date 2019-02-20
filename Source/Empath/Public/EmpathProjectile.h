// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathDeflectableInterface.h"
#include "EmpathTeamAgentInterface.h"
#include "EmpathProjectile.generated.h"

class UProjectileMovementComponent;
class UEmpathDamageType;

UCLASS()
class EMPATH_API AEmpathProjectile : public AActor, public IEmpathDeflectableInterface, public IEmpathTeamAgentInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathProjectile(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** The team of the projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile, meta = (ExposeOnSpawn = "true"))
	EEmpathTeam Team;

	/** Returns the team number of the actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathProjectile)
	EEmpathTeam GetTeamNum() const;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Name of the ProjectileMovementMovement component. Use this name if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass). */
	static FName ProjectileMovementComponentName;

	/** Whether the projectile can be deflected in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	bool bDeflectEnabled;

	/** Whether a particular component of the projectile can be deflected. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathProjectile)
	bool IsDeflectable(const UPrimitiveComponent* DeflectedComponent) const;

	/** Whether friendly fire is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	bool bEnableFriendlyFire;

	/** The lifetime of the projectile after spawning, before the Die function is automatically called. Will be ignored if < 0.05. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile, meta = (ExposeOnSpawn = "true"))
	float LifetimeAfterSpawn;

	/** The handle for timing out and dying. */
	FTimerHandle LifetimeTimerHandle;

	/*
	* Called to start the lifetime timer. 
	* When finished, the actor will call the death function. 
	* Will be ignored if < 0.05. 
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathProjectile)
	void StartDeathTimer(const float Lifetime);

	/** Cancels the lifetime timer handle. */
	UFUNCTION(BlueprintCallable, Category = EmpathProjectile)
	void CancelDeathTimer();

	/** Returns Whether this projectile is currently dead and awaiting cleanup. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathProjectile)
	const bool IsDead() const { return bDead; }

	/** Instructs the projectile to die and begin cleanup. */
	UFUNCTION(BlueprintCallable, Category = EmpathProjectile)
	virtual void Die();

	/** Called when the projectile begins to die. */
	UFUNCTION(BlueprintNativeEvent, Category = EmpathProjectile, meta = (DisplayName = "Die"))
	void ReceiveDie();

	/*
	*  The amount of take after death we wait to perform cleanup operations after the projectile is dead. 
	*  Will be ignored if less than 0.05.
	*  Resets the lifetime timer handle if necessary.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeBeforePostDeathCleanup;

	/** The timer handle for post death cleanup. */
	FTimerHandle PostDeathCleanupTimerHandle;

	/** Stops the post death cleanup timer. */
	void CancelPostDeathCleanupTimer();

	/** Called to cleanup the projectile after it dies. Usually deletes the projectile. */
	UFUNCTION(BlueprintNativeEvent, Category = EmpathProjectile)
	void PostDeathCleanup();

	/** The amount of damage to inflict. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile, meta = (ExposeOnSpawn = "true"))
	float ImpactDamageAmount;

	/** The damage type of the projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	TSubclassOf<UEmpathDamageType> DamageTypeClass;

	/*
	* The amount of radial damage to inflict.
	* Ignored if <= 0.0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile, meta = (ExposeOnSpawn = "true"))
	float RadialDamageAmount;

	/*
	* The radius of radial damage.
	* Ignored if <= 0.0.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile, meta = (ExposeOnSpawn = "true"))
	float RadialDamageRadius;

	/*
	* Override damage type for radial damage.
	* Will use DamageTypeClass if not set.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	TSubclassOf<UEmpathDamageType> RadialDamageTypeClass;

	/*
	* Whether we should use fall-off damage for radial damage, applying less damage to targets that are further away.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	bool bUseRadialDamageFallOff;

	/*
	* Returns whether we should trigger can impact event. 
	* By default, true if we are not dead,  the other actor is not friendly or friendly fire is enabled,
	* the impacting component is not deflectable while the impacted component is deflecting,
	* and we should not ignore the collision event.
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = EmpathProjectile)
	bool ShouldTriggerImpact(const UPrimitiveComponent* ImpactingComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherActorComponent);

	/** Attempts to impact the inputted actor. If ShouldTriggerImpact is successful, it will call the OnImpact event. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = EmpathProjectile, meta = (AutoCreateRefTerm = "HitResult"))
	bool AttemptImpactActor(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, UPARAM(ref) const FHitResult& HitResult);

	/*
	* Event that is called when we successfully impact another actor. 
	* By default, applies point damage to that actor, and triggers the die event. 
	*/
	UFUNCTION(BlueprintNativeEvent, Category = EmpathProjectile)
	void OnImpact(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FHitResult& HitResult);

	/** Gets the remaining lifetime of the projectile, provided that it was set. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathProjectile)
	const float GetRemainingLifetime() const;

	/*
	* The maximum turning angle when homing. 
	* If the homing target is more than this number of degree away the forward angle of the projectile, will abort homing.
	* Useful for when you want a homing projectile to be able to miss the target and should not turn back around.
	* Ignored if 0 or lower.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathProjectile)
	float MaxHomingAngle;

	/** Makes this projectile a homing projectile, calling events and setting parameters as appropriate. */
	UFUNCTION(BlueprintCallable, Category = EmpathProjectile)
	void LockOnTarget(USceneComponent* TargetComponent, float HomingAcceleration = 150.0f);

	/** Called when this projectile locks on to a target. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathProjectile)
	void OnTargetLocked(USceneComponent* TargetComponet, float HomingAcceleration);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** The projectile's movement component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathProjectile, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

	/** Whether this projectile is currently dead and awaiting cleanup. */
	bool bDead;

private:

};
