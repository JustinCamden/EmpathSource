// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathDeflectorInterface.h"
#include "EmpathTeamAgentInterface.h"
#include "EmpathDeflector.generated.h"

UCLASS()
class EMPATH_API AEmpathDeflector : public AActor, public IEmpathDeflectorInterface, public IEmpathTeamAgentInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathDeflector(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** The team of the deflector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathDeflector, meta = (ExposeOnSpawn = "true"))
	EEmpathTeam Team;

	/** Returns the team number of the actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathDeflector)
	EEmpathTeam GetTeamNum() const;

	/*
	* Overridable function for getting the deflection impulse for this actor when using deflect. 
	* By default, returns the forward vector of the actor.
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = EmpathDeflector)
	FVector GetDeflectionImpulse() const;

	/** Sets whether deflect is currently active. */
	UFUNCTION(BlueprintCallable, Category = EmpathDeflector)
	void SetDeflectActive(const bool bNewActive);

	/** Gets whether deflect is currently active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathDeflector)
	const bool IsDeflectActive() const { return bDeflectActive; }

	/** Called when deflect is activated. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathDeflector)
	void OnDeflectActivated();

	/** Called when deflect is deactivated. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathDeflector)
	void OnDeflectDeactivated();

	/** Whether a particular component on this actor is currently deflecting. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathDeflector)
	bool IsDeflecting(const UPrimitiveComponent* DeflectingComponent) const;

	/** Whether we can deflect friendly targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathDeflector)
	bool bEnableDeflectFriendlies;

	/*
	* Checks to see whether we can deflect another actor. 
	* By default, true if deflection is active, we should not ignore collision with the other actor, the other actor is not friendly or deflect friendlies is enabled, and the other actor is deflectable.
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = EmpathDeflector)
	bool ShouldTriggerDeflect(const UPrimitiveComponent* ImpactingComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherActorComponent);

	/*
	* Attempts to trigger a deflect on the other actor.
	* If ShouldTriggerDeflect returns true, will call BeDeflected on the other actor using the result of GetDeflectionImpulse.
	* Returns true if the deflection was successfully triggered.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathDeflector, meta = (AutoCreateRefTerm = "HitResult"))
	const bool AttemptDeflectActor(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, UPARAM(ref) const FHitResult& HitResult);

	/** Called after the deflector successfully triggers a deflect on another actor. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathDeflector, meta = (AutoCreateRefTerm = "HitResult"))
	void OnDeflected(UPrimitiveComponent* DeflectingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FVector& DeflectionImpulse, const FHitResult& HitResult);

	/*
	* Call on activation to attempt to deflect any initial overlapping actors on the inputted component. 
	* Requires implementation of Sweep Deflection Collision.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathDeflector)
	void DeflectInitialOverlappingActors(UPrimitiveComponent* DeflectingComponent);

	/*
	* Sweeps for overlapping actors across a component being used for deflection collision. 
	* Requires implementation.
	*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = EmpathDeflector)
	TArray<FHitResult> SweepDeflectionCollision(const UPrimitiveComponent* DeflectingComponent);

	/** How long to wait before clearing the initial overlapped actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathDeflector)
	float InitialDeflectedActorsClearTime;

	 /** Time for clearing the initially deflected actors. */
	FTimerHandle ClearInitialDeflectedActorsTimerHandle;

	/** Clears the initially deflected actors so that we can deflect them again. */
	void ClearInitialDeflectedActors();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


private:
	/** Whether deflection is currently active. */
	bool bDeflectActive;

	/** Actors that were overlapped and already deflected when we activated. Will be ignored when it comes to deflection. Should be cleared regularly. */
	TSet <AActor*> InitialDeflectedActors;
	
};
