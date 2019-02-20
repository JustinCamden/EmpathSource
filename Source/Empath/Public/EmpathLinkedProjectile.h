// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EmpathTypes.h"
#include "EmpathProjectile.h"
#include "EmpathLinkedProjectile.generated.h"

class UCapsuleComponent;
class UBoxComponent;
class AEmpathGameModeBase;

/**
 * 
 */
UCLASS()
class EMPATH_API AEmpathLinkedProjectile : public AEmpathProjectile
{
	GENERATED_BODY()

public:

	/** Name of the head collision component. */
	static FName HeadComponentName;

	/** Name of the air tail collision component. */
	static FName AirTailComponentName;

	/** Name of the ground tail collision component. */
	static FName GroundTailComponentName;

	// Sets default variables for the class
	AEmpathLinkedProjectile(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/*
	* The target for the projectile's tail.
	* The tail will always position between this component and the head, scaling and rotating accordingly.
	* Usually the head of the previous link.
	*/
	UPROPERTY(BlueprintReadWrite, Category = EmpathLinkedProjectile, meta = (ExposeOnSpawn = "true"))
	USceneComponent* TailTarget;

	/** The minimum lifetime for spawned links. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile)
	float MinLifetime;

	/** The lifetime of the projectile after becoming grounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile)
	float GroundedLifetime;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Called every time the world pulses. */
	UFUNCTION()
	void OnPulse(float PulseDeltaTime);

	/** Called every time the world pulses. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile, meta = (DisplayName = OnPulse))
	void ReceivePulse(float PulseDeltaTime);

	/** The maximum length of the tail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile)
	float MaxTailLength;

	/** Inserts a link into the chain between this link and the tail target. Returns the link that was created if successful. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	AEmpathLinkedProjectile* InsertLink();

	/** Reference to the previous link. */
	UPROPERTY(BlueprintReadWrite, Category = EmpathLinkedProjectile, meta = (ExposeOnSpawn = "true"))
	AEmpathLinkedProjectile* PreviousLink;

	/** Reference to the next link. */
	UPROPERTY(BlueprintReadWrite, Category = EmpathLinkedProjectile, meta = (ExposeOnSpawn = "true"))
	AEmpathLinkedProjectile* NextLink;

	/** Detaches from the next link in the chain. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void DetachFromNext();

	/** Detaches from the previous link in the chain. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void DetachFromPrevious();

	/** Removes the link from the chain. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void RemoveLink();

	/** Override of die to remove the link. */
	virtual void Die() override;

	/** Returns whether this projectile is the last link in a chain. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	const bool IsLastLink() const;

	/** Returns whether this projectile is the first link in a chain. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	const bool IsFirstLink() const;

	/*
	* Called when this link becomes the oldest link in a chain. 
	* Call when the first projectile spawns, or when detaching from the next link. 
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void OnBecomeFirstLink();

	/*
	* Called when this link becomes the oldest link in a chain.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile, meta = (DisplayName = "On Become First Link"))
	void ReceiveBecomeFirstLink();

	/** Gets the first link in the chain. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	AEmpathLinkedProjectile* GetFirstLink();

	/** Grounds the projectile and disables movement. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void BecomeGrounded();

	/** Called when we become grounded. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile)
	void OnBecomeGrounded();

	/** Called when we activate the ground tail. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile)
	void OnGroundTailActivated();

	/** Called just before the air tail is destroyed. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile)
	void OnBeforeAirTailDestroyed();

	/*
	* Gets the best location for the head when becoming grounded. 
	* Returns false if no valid location was found.
	*/
	bool GetBestGroundedLocation(FVector& OutGroundedLocation);

	/** The width of the grounded tail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile)
	float GroundTailWidth;

	/** The minimum height of the grounded tail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile)
	float GroundTailMinHeight;

	/** The damage type inflicted when grounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile)
	TSubclassOf<UEmpathDamageType> GroundedDamageTypeClass;

	/** The amount of damage inflicted on impact when grounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile, meta = (ExposeOnSpawn = "true"))
	float GroundedDamageAmountPerImpact;

	/** The amount of damage inflicted per second on overlapping actors when grounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathLinkedProjectile, meta = (ExposeOnSpawn = "true"))
	float GroundedDamageAmountPerSecond;

	virtual void Tick(float DeltaTime) override;

	virtual void OnImpact_Implementation(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FHitResult& HitResult) override;

	/** Applies damage to all valid targets overlapping the ground tail. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void ApplyGroundedDamageOverTime(const float DeltaTime);

	/** Gets previous links and link points inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void GetPreviousChainLinksAndPoints(TArray<AEmpathLinkedProjectile*>& OutLinks, TArray<FVector>& OutLinkPoints, USceneComponent*& OutFinalTailTarget);

	/** Gets previous links inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	TArray<AEmpathLinkedProjectile*> GetPreviousChainLinks();

	/** Gets all the head and tail target points in a chain, starting from this link downwards. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	TArray<FVector> GetPreviousChainPoints();

	/** Gets the first link in a chain with an active air tail. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	AEmpathLinkedProjectile* GetFirstAirTailLink();

	/** Gets previous links and link points with active air tails, inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void GetPreviousAirTailLinksAndPoints(TArray<AEmpathLinkedProjectile*>& OutLinks, TArray<FVector>& OutLinkPoints, USceneComponent*& OutFinalTailTarget);

	/** Gets previous links with active air tails, inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	TArray<AEmpathLinkedProjectile*> GetPreviousAirTailLinks();

	/** Gets all previous points in a chain with active air tails, inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	TArray<FVector> GetPreviousAirTailPoints();

	/** Called when this link becomes the first link with an air tail. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile)
	void OnBecomeFirstAirTailLink();

	/** Gets the first link in a chain with an active ground tail. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	AEmpathLinkedProjectile* GetFirstGroundTailLink();

	/** Gets previous links and link points with active ground tail, inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	void GetPreviousGroundTailLinksAndPoints(TArray<AEmpathLinkedProjectile*>& OutLinks, TArray<FVector>& OutLinkPoints, USceneComponent*& OutFinalTailTarget);

	/** Gets previous links with active ground tails, inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	TArray<AEmpathLinkedProjectile*> GetPreviousGroundTailLinks();

	/** Gets all previous points in a chain with active ground tails, inclusive of this link. */
	UFUNCTION(BlueprintCallable, Category = EmpathLinkedProjectile)
	TArray<FVector> GetPreviousGroundTailPoints();

	/** Called when this link becomes the first grounded link. */
	void OnBecomeFirstGroundTailLink();

	/** Called when this link becomes the first grounded link. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile, meta = (DisplayName = "On Become First Ground Tail Link"))
	void ReceiveBecomeFirstGroundTailLink();

	/** Called when this link is no longer the first grounded link. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathLinkedProjectile)
	void OnStopBeingFirstGroundTailLink();


protected:
	/*
	* The projectile's root or head component.
	* The center of the projectile's movement.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathLinkedProjectile, meta = (AllowPrivateAccess = "true"))
	USceneComponent* LinkHead;

	/*
	* The projectile's tail collision component.
	* Always points towards the tail target component and the head component.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathLinkedProjectile, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* LinkAirTail;

	/** The projectile's tail collision after becoming grounded. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathLinkedProjectile, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LinkGroundTail;

	/** Reference to the empath game mode. */
	AEmpathGameModeBase* EmpathGameMode;

	/** The current grounded state of the projectile. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathLinkedProjectile, meta = (AllowPrivateAccess = "true"))
	EEmpathSlashGroundedState GroundedState;

	/** Updates the air tail location and rotation to position itself between the head and target, and extend itself to meet them. */
	void UpdateAirTailTransform();

	/** Destroys the capsule tail and creates a ground version. */
	void ActivateGroundTail();

	/** Class of linked projectile to spawn when inserting links. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = EmpathLinkedProjectile)
	TSubclassOf<AEmpathLinkedProjectile> InsertedLinkClass;
};