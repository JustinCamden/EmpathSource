// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EmpathTypes.h"
#include "EmpathTeamAgentInterface.h"
#include "EmpathPlayerController.generated.h"

// Stat groups for UE Profiler
DECLARE_STATS_GROUP(TEXT("EmpathPlayerCon"), STATGROUP_EMPATH_PlayerCon, STATCAT_Advanced);

class AEmpathPlayerCharacter;

/**
 * 
 */
UCLASS()
class EMPATH_API AEmpathPlayerController : public APlayerController, public IEmpathTeamAgentInterface
{
	GENERATED_BODY()
	
public:

	static const FName PlayerTargetSelectionTraceTag;

	/** Constructor like behavior. */
	AEmpathPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// ---------------------------------------------------------
	//	TeamAgent Interface

	/** Returns the team number of the actor. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Game")
	EEmpathTeam GetTeamNum() const;

	/** Called when the controlled VR character becomes stunned. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerController, meta = (DisplayName = "OnCharacterStunned"))
	void ReceiveCharacterStunned(const AController* StunInstigator, const AActor* StunCauser, const float StunDuration);

	/** Called when the controlled VR character is no longer stunned. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerController, meta = (DisplayName = "OnCharacterStunEnd"))
	void ReceiveCharacterStunEnd();
	
	/** Called when the controlled VR character dies. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerController, meta = (DisplayName = "OnCharacterStunEnd"))
	void ReceiveCharacterDeath(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType);

	// Returns the possible player attack targets in the scene
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
	TArray<FEmpathPlayerAttackTarget> const& GetPlayerAttackTargets() const { return PlayerAttackTargets; }

	/** Adds a player attack target to the list. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
	void AddPlayerAttackTarget(AActor* NewTarget, float TargetPreference = 1.0f);

	/** Removes a player attack target from the list. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
	void RemovePlayerAttackTarget(AActor* TargetToRemove);

	/*
	* Gets the best target for aim assist, depending on the origin location and targeting direction. Returns true if a target was found.
	* @param DistanceWeight How heavily to weight distance in target selection. Higher scores will emphasize nearer targets.
	* @param AngleWeight How heavily to weight angle in target selection. Higher scores will emphasize smaller angles to the target.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
		bool GetBestAttackTarget(AActor*& OutBestTarget,
			FVector& OutTargetAimLocation,
			USceneComponent*& OutTargetAimLocationComp,
			FVector& OutDirectionToTarget,
			float& OutAngleToTarget,
			float& OutDistanceToTarget,
			const FVector& OriginLocation,
			const FVector& FacingDirection,
			float MaxRange = 10000.0f,
			float MaxAngle = 30.0f,
			float DistanceWeight = 1.0f,
			float AngleWeight = 1.0f);

	/*
	* Gets the best target for aim assist, depending on the origin location and targeting direction. Returns true if a target was found.
	* @param DistanceWeight How heavily to weight distance in target selection. Higher scores will emphasize nearer targets.
	* @param AngleWeight How heavily to weight angle in target selection. Higher scores will emphasize smaller angles to the target.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
		bool GetBestAttackTargetWithPrediction(AActor*& OutBestTarget,
			FVector& OutTargetAimLocation,
			USceneComponent*& OutTargetAimLocationComp,
			FVector& OutDirectionToTarget,
			float& OutAngleToTarget,
			float& OutDistanceToTarget,
			const FVector& OriginLocation,
			const FVector& FacingDirection,
			float MaxRange = 10000.0f,
			float MaxAngle = 30.0f,
			float DistanceWeight = 1.0f,
			float AngleWeight = 1.0f,
			float AttackSpeed = 1500.0f,
			float Gravity = 0.0f,
			float MaxPredictionTime = 3.0f,
			float PredictionFailedPenalty = -0.5f);


	/*
	* Gets the best target for aim assist, depending on the origin location and targeting direction. Returns true if a target was found.
	* @param DistanceWeight How heavily to weight distance in target selection. Higher scores will emphasize nearer targets.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
	bool GetBestAttackTargetInDirections(AActor*& OutBestTarget,
		FVector& OutTargetAimLocation,
		USceneComponent*& OutTargetAimLocationComp,
		FVector& OutBestSearchDirection,
		FVector& OutDirectionToTarget,
		float& OutBestAngleToTarget,
		float& OutDistanceToTarget,
		const FVector& OriginLocation, 
		const TArray<FEmpathTargetingDirection>& SearchDirections,
		float MaxRange = 10000.0f,
		float DistanceWeight = 1.0f,
		float AngleWeight = 1.0f);


	/*
	* Gets the best target for aim assist, depending on the origin location and targeting direction. Returns true if a target was found.
	* @param DistanceWeight How heavily to weight distance in target selection. Higher scores will emphasize nearer targets.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
	bool GetBestAttackTargetInDirectionsWithPrediction(AActor*& OutBestTarget,
		FVector& OutTargetAimLocation,
		USceneComponent*& OutTargetAimLocationComp,
		FVector& OutBestSearchDirection,
		FVector& OutDirectionToTarget,
		float& OutBestAngleToTarget,
		float& OutDistanceToTarget,
		const FVector& OriginLocation,
		const TArray<FEmpathTargetingDirection>& SearchDirections,
		float MaxRange = 10000.0f,
		float DistanceWeight = 1.0f,
		float AngleWeight = 1.0f,
		float AttackSpeed = 1500.0f,
		float Gravity = 0.0f,
		float MaxPredictionTime = 3.0f,
		float PredictionFailedPenalty = -0.5f);

	/*
	* Gets the best target for aim assist, depending on the origin location and targeting direction. Returns true if a target was found.
	* Uses the right edge and left edge variables for the horizontal edges of the cone. Angle between these vectors will be added to the maximum horizontal angle.
	* Input need not be normalized;
	* @param DistanceWeight How heavily to weight distance in target selection. Higher scores will emphasize nearer targets.
	* @param AngleWeight How heavily to weight angle in target selection. Higher scores will emphasize smaller angles to the target.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
		bool GetBestAttackTargetInConeSegmentWithPrediction(AActor*& OutBestTarget,
			FVector& OutTargetAimLocation,
			USceneComponent*& OutTargetAimLocationComp,
			FVector& OutDirectionToTarget,
			float& OutAngleToTarget,
			float& OutDistanceToTarget,
			const FVector& OriginLocation,
			const FVector& RightEdge,
			const FVector& LeftEdge,
			float MaxRange = 10000.0f,
			float MaxConeAngle = 15.0f,
			float DistanceWeight = 1.0f,
			float AngleWeight = 1.0f,
			float AttackSpeed = 1500.0f,
			float Gravity = 0.0f,
			float MaxPredictionTime = 3.0f,
			float PredictionFailedPenalty = -0.5f);

	/*
	* Gets the best target for aim assist, depending on the origin location and targeting direction. Returns true if a target was found.
	* Uses the right edge and left edge variables for the horizontal edges of the cone. Angle between these vectors will be added to the maximum horizontal angle.
	* Input need not be normalized;
	* Predicts target path for enhanced accuracy.
	* @param DistanceWeight How heavily to weight distance in target selection. Higher scores will emphasize nearer targets.
	* @param AngleWeight How heavily to weight angle in target selection. Higher scores will emphasize smaller angles to the target.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerController)
		bool GetBestAttackTargetInConeSegment(AActor*& OutBestTarget,
			FVector& OutTargetAimLocation,
			USceneComponent*& OutTargetAimLocationComp,
			FVector& OutDirectionToTarget,
			float& OutAngleToTarget,
			float& OutDistanceToTarget,
			const FVector& OriginLocation,
			const FVector& RightEdge,
			const FVector& LeftEdgeEdge,
			float MaxRange = 10000.0f,
			float MaxConeAngle = 15.0f,
			float DistanceWeight = 1.0f,
			float AngleWeight = 1.0f);

	/** Gets the controlled player character, if any. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerController)
	AEmpathPlayerCharacter* GetControlledEmpathPlayerChar() const { return EmpathPlayerCharacter; }

protected:
	virtual void BeginPlay() override;

	virtual void SetPawn(APawn* InPawn) override;

	/** The list of possible player targets. */
	UPROPERTY(Category = EmpathPlayerController, EditAnywhere, BlueprintReadWrite)
	TArray<FEmpathPlayerAttackTarget> PlayerAttackTargets;


private:
	/** Removes invalid targets from the list of targets. */
	void CleanUpPlayerAttackTargets();

	/** The team of this actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathPlayerController, meta = (AllowPrivateAccess = true))
	EEmpathTeam Team;

	/** Reference to the controlled Empath Character. */
	AEmpathPlayerCharacter* EmpathPlayerCharacter;
};
