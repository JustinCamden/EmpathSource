// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathPlayerController.h"
#include "EmpathFunctionLibrary.h"
#include "CollisionQueryParams.h"
#include "EmpathPlayerCharacter.h"
#include "EmpathCharacter.h"
#include "Runtime/Engine/Public/EngineUtils.h"

DECLARE_CYCLE_STAT(TEXT("Player Target Selection"), STAT_EMPATH_PlayerTargetSelection, STATGROUP_EMPATH_PlayerCon);

const FName AEmpathPlayerController::PlayerTargetSelectionTraceTag = FName(TEXT("PlayerTargetSelectionTrace"));

AEmpathPlayerController::AEmpathPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EEmpathTeam::Player;
}


EEmpathTeam AEmpathPlayerController::GetTeamNum_Implementation() const
{
	return Team;
}

void AEmpathPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Grab any empath characters that were not already registered with us. Shouldn't ever happen, but just in case.
	for (AEmpathCharacter* CurrEmpathCHar : TActorRange<AEmpathCharacter>(GetWorld()))
	{
		CurrEmpathCHar->RegisterEmpathPlayerCon(this);
	}
}

void AEmpathPlayerController::RemovePlayerAttackTarget(AActor* Target)
{
	for (int32 Idx = 0; Idx < PlayerAttackTargets.Num(); ++Idx)
	{
		FEmpathPlayerAttackTarget& PlayerTarget = PlayerAttackTargets[Idx];
		if (PlayerTarget.TargetActor == Target)
		{
			PlayerAttackTargets.RemoveAtSwap(Idx, 1, true);
			--Idx;
		}
	}
}

void AEmpathPlayerController::AddPlayerAttackTarget(AActor* Target, float TargetPreference)
{
	if (Target)
	{
		// If the target is already in the list, simply update it
		for (int32 Idx = 0; Idx < PlayerAttackTargets.Num(); ++Idx)
		{
			FEmpathPlayerAttackTarget& PlayerTarget = PlayerAttackTargets[Idx];
			if (PlayerTarget.TargetActor == Target)
			{
				PlayerAttackTargets[Idx].TargetPreference = FMath::Max(TargetPreference, 0.1f);
				return;
			}
		}

		PlayerAttackTargets.Add(FEmpathPlayerAttackTarget(Target, TargetPreference));
		return;
	}
}

void AEmpathPlayerController::CleanUpPlayerAttackTargets()
{
	for (int32 Idx = 0; Idx < PlayerAttackTargets.Num(); ++Idx)
	{
		FEmpathPlayerAttackTarget& PlayerTarget = PlayerAttackTargets[Idx];
		if (PlayerTarget.IsValid() == false)
		{
			PlayerAttackTargets.RemoveAtSwap(Idx, 1, true);
			--Idx;
		}
	}
}

bool AEmpathPlayerController::GetBestAttackTarget(AActor*& OutBestTarget, FVector& OutTargetAimLocation, USceneComponent*& OutTargetAimLocationComp, FVector& OutDirectionToTarget, float& OutAngleToTarget, float& OutDistanceToTarget, const FVector& OriginLocation, const FVector& FacingDirection, float MaxRange /*= 10000.0f*/, float MaxAngle /*= 30.0f*/, float DistanceWeight /*= 1.0f*/, float AngleWeight /*= 1.0f*/)
{
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTargetSelection);

	CleanUpPlayerAttackTargets();
	if (PlayerAttackTargets.Num() > 0)
	{
		// Initialize variables
		float GreatestDistToTarget = 0.0f;
		float GreatestAngle = 0.0f;
		TArray<FEmpathTargetingCandidate> TargetCandidates;
		FVector NormalizedFacingDirection = FacingDirection.GetSafeNormal();

		// Get all possible targets
		for (FEmpathPlayerAttackTarget& CurrTarget : PlayerAttackTargets)
		{
			// If the current target is valid
			if (CurrTarget.IsValid())
			{
				// Get the aim location on the target
				FVector TargetAimLocation;
				USceneComponent* TargetAimLocationComponent;
				if (!UEmpathFunctionLibrary::GetAimLocationOnActor(CurrTarget.TargetActor, OriginLocation, FacingDirection, TargetAimLocation, TargetAimLocationComponent))
				{
					continue;
				}
				
				// Check if the target is in range
				FVector DirectionToTarget = TargetAimLocation - OriginLocation;
				float DistToTargetSquared = DirectionToTarget.SizeSquared();
				if (DistToTargetSquared > MaxRange * MaxRange)
				{
					continue;
				}

				// Check if the target is within the maximum angle
				DirectionToTarget = DirectionToTarget.GetSafeNormal();
				float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DirectionToTarget | NormalizedFacingDirection));
				if (AngleToTarget > MaxAngle)
				{
					continue;
				}

				// Check if we have line of sight to the target
				// Setup trace params
				FCollisionQueryParams Params(PlayerTargetSelectionTraceTag);
				AEmpathPlayerCharacter* PlayerCharacter = Cast<AEmpathPlayerCharacter>(GetPawn());
				if (PlayerCharacter)
				{
					Params.AddIgnoredActors(PlayerCharacter->GetControlledActors());
				}
				Params.AddIgnoredActor(GetPawn());
				Params.AddIgnoredActor(CurrTarget.TargetActor);
				FCollisionResponseParams const ResponseParams = FCollisionResponseParams::DefaultResponseParam;
				FHitResult OutHit;

				// Do the actual raycast
				bool bHit = (GetPawn()->GetWorld()->LineTraceSingleByChannel(OutHit, OriginLocation, TargetAimLocation,
					ECC_Visibility, Params, ResponseParams));
				if (bHit && OutHit.Actor != CurrTarget.TargetActor)
				{
					continue;
				}
			
				// If we passed all above checks, add to the list of candidates
				float DistToTarget = FMath::Sqrt(DistToTargetSquared);
				TargetCandidates.Add(FEmpathTargetingCandidate(CurrTarget.TargetActor, 
					TargetAimLocation, 
					DirectionToTarget, 
					TargetAimLocationComponent, 
					DistToTarget, 
					AngleToTarget, 
					CurrTarget.TargetPreference));

				// Update greatest distance and angles if necessary
				if (DistToTarget > GreatestDistToTarget)
				{
					GreatestDistToTarget = DistToTarget;
				}
				if (AngleToTarget > GreatestAngle)
				{
					GreatestAngle = AngleToTarget;
				}
			}
		}

		// Score the targets
		if (TargetCandidates.Num() > 0)
		{
			float BestScore = -99999.0f;
			FEmpathTargetingCandidate* BestTarget = nullptr;

			// Loop through each target to find the one with the best score
			for (FEmpathTargetingCandidate& CurrCandidate : TargetCandidates)
			{
				// Angle
				float TotalScore = (1.0f - (CurrCandidate.Angle / GreatestAngle)) * AngleWeight;

				// Distance
				TotalScore += (1.0f - (CurrCandidate.Distance / GreatestDistToTarget)) * DistanceWeight;

				// Targeting preference
				TotalScore *= CurrCandidate.TargetPreference;

				// Update best targets and score if necessary
				if (TotalScore > BestScore)
				{
					BestScore = TotalScore;
					BestTarget = &CurrCandidate;
				}
			}

			// Return the target with the best total score
			OutBestTarget = BestTarget->Actor;
			OutTargetAimLocation = BestTarget->AimLocation;
			OutTargetAimLocationComp = BestTarget->AimLocationComponent;
			if (!OutTargetAimLocationComp && OutBestTarget)
			{
				OutTargetAimLocationComp = OutBestTarget->GetRootComponent();
			}
			OutDirectionToTarget = BestTarget->DirectionToTarget;
			OutAngleToTarget = BestTarget->Angle;
			OutDistanceToTarget = BestTarget->Distance;

			return true;
		}
	}

	OutBestTarget = nullptr;
	OutTargetAimLocation = FVector::ZeroVector;
	OutTargetAimLocationComp = nullptr;
	OutDirectionToTarget = FVector::ZeroVector;
	OutAngleToTarget = 0.0f;
	OutDistanceToTarget = 0.0f;
	return false;
}

bool AEmpathPlayerController::GetBestAttackTargetWithPrediction(AActor*& OutBestTarget,
	FVector& OutTargetAimLocation,
	USceneComponent*& OutTargetAimLocationComp,
	FVector& OutDirectionToTarget,
	float& OutAngleToTarget,
	float& OutDistanceToTarget,
	const FVector& OriginLocation,
	const FVector& FacingDirection,
	float MaxRange /*= 10000.0f*/,
	float MaxAngle /*= 30.0f*/,
	float DistanceWeight /*= 1.0f*/,
	float AngleWeight /*= 1.0f*/,
	float AttackSpeed /*= 1500.0f*/,
	float Gravity /*= 0.0f*/,
	float MaxPredictionTime /*= 3.0f*/,
	float PredictionFailedPenalty /*= -0.5f*/)
{
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTargetSelection);

	CleanUpPlayerAttackTargets();
	if (PlayerAttackTargets.Num() > 0)
	{
		// Initialize variables
		float GreatestDistance = 0.0f;
		float GreatestAngle = 0.0f;
		TArray<FEmpathTargetingCandidatePred> TargetCandidates;
		FVector NormalizedFacingDirection = FacingDirection.GetSafeNormal();

		// Get all possible targets
		for (FEmpathPlayerAttackTarget& CurrTarget : PlayerAttackTargets)
		{
			// If the current target is valid
			if (CurrTarget.IsValid())
			{
				// Get the aim location on the target
				FVector TargetAimLocation;
				USceneComponent* TargetAimLocationComponent;
				if (!UEmpathFunctionLibrary::GetAimLocationOnActor(CurrTarget.TargetActor, OriginLocation, FacingDirection, TargetAimLocation, TargetAimLocationComponent))
				{
					continue;
				}

				// Check if the target is too far away
				float DistToTargetSquared = (TargetAimLocation - OriginLocation).SizeSquared();
				if (DistToTargetSquared > MaxRange * MaxRange)
				{
					continue;
				}

				// Try to predict the best direction to the target
				FVector DirectionToTarget;
				FVector AimLocCopy = TargetAimLocation;
				bool bFailedAimPrediction = (!UEmpathFunctionLibrary::PredictBestAimDirection(TargetAimLocation, DirectionToTarget, OriginLocation, AimLocCopy, CurrTarget.TargetActor->GetVelocity(), AttackSpeed, Gravity, MaxPredictionTime));

				// Check if the target is within the maximum angle
				float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DirectionToTarget | NormalizedFacingDirection));
				if (AngleToTarget > MaxAngle)
				{
					continue;
				}

				// Check if we have line of sight to the target
				// Setup trace params
				FCollisionQueryParams Params(PlayerTargetSelectionTraceTag);
				AEmpathPlayerCharacter* PlayerCharacter = Cast<AEmpathPlayerCharacter>(GetPawn());
				if (PlayerCharacter)
				{
					Params.AddIgnoredActors(PlayerCharacter->GetControlledActors());
				}
				Params.AddIgnoredActor(GetPawn());
				Params.AddIgnoredActor(CurrTarget.TargetActor);
				FCollisionResponseParams const ResponseParams = FCollisionResponseParams::DefaultResponseParam;
				FHitResult OutHit;

				// Do the actual raycast
				bool bHit = (GetPawn()->GetWorld()->LineTraceSingleByChannel(OutHit, OriginLocation, TargetAimLocation,
					ECC_Visibility, Params, ResponseParams));
				if (bHit && OutHit.Actor != CurrTarget.TargetActor)
				{
					continue;
				}

				// If we passed all above checks, add to the list of candidates
				float DistToTarget = FMath::Sqrt(DistToTargetSquared);
				TargetCandidates.Add(FEmpathTargetingCandidatePred(CurrTarget.TargetActor,
					TargetAimLocation,
					DirectionToTarget,
					TargetAimLocationComponent,
					DistToTarget,
					AngleToTarget,
					CurrTarget.TargetPreference,
					bFailedAimPrediction));

				// Update greatest distance and angles if necessary
				if (DistToTarget > GreatestDistance)
				{
					GreatestDistance = DistToTarget;
				}
				if (AngleToTarget > GreatestAngle)
				{
					GreatestAngle = AngleToTarget;
				}
			}
		}

		// Score the targets
		FEmpathTargetingCandidatePred* BestTarget = nullptr;
		if (TargetCandidates.Num() > 0)
		{
			float BestScore = -99999.0f;
			int32 BestTargetIdx = -1;

			// Loop through each target to find the one with the best score
			for (FEmpathTargetingCandidatePred& CurrCandidate : TargetCandidates)
			{
				// Angle
				float TotalScore = (1.0f - (CurrCandidate.Angle / GreatestAngle)) * AngleWeight;

				// Distance
				TotalScore += (1.0f - (CurrCandidate.Distance / GreatestDistance)) * DistanceWeight;
				
				// Aim prediction
				if (CurrCandidate.bFailedAimPrediction)
				{
					TotalScore += PredictionFailedPenalty;
				}

				// Targeting preference
				TotalScore *= CurrCandidate.TargetPreference;

				// Update best targets and score if necessary
				if (TotalScore > BestScore)
				{
					BestScore = TotalScore;
					BestTarget = &CurrCandidate;
				}
			}

			// Return the target with the best total score
			OutBestTarget = BestTarget->Actor;
			OutTargetAimLocation = BestTarget->AimLocation;
			OutTargetAimLocationComp = BestTarget->AimLocationComponent;
			if (!OutTargetAimLocationComp && OutBestTarget)
			{
				OutTargetAimLocationComp = OutBestTarget->GetRootComponent();
			}
			OutDirectionToTarget = BestTarget->DirectionToTarget;
			OutAngleToTarget = BestTarget->Angle;
			OutDistanceToTarget = BestTarget->Distance;

			return true;
		}
	}

	OutBestTarget = nullptr;
	OutTargetAimLocation = FVector::ZeroVector;
	OutTargetAimLocationComp = nullptr;
	OutDirectionToTarget = FVector::ZeroVector;
	OutAngleToTarget = 0.0f;
	OutDistanceToTarget = 0.0f;
	return false;
}

bool AEmpathPlayerController::GetBestAttackTargetInDirections(AActor*& OutBestTarget, FVector& OutTargetAimLocation, USceneComponent*& OutTargetAimLocationComp, FVector& OutBestSearchDirection, FVector& OutDirectionToTarget, float& OutBestAngleToTarget, float& OutDistanceToTarget, const FVector& OriginLocation, const TArray<FEmpathTargetingDirection>& SearchDirections, float MaxRange /*= 10000.0f*/, float DistanceWeight /*= 1.0f*/, float AngleWeight /*= 1.0f*/)
{
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTargetSelection);

	CleanUpPlayerAttackTargets();
	if (PlayerAttackTargets.Num() > 0 && SearchDirections.Num() > 0.0f)
	{
		// Initialize variables
		float GreatestDistance = -1.0f;
		float GreatestWeightedAngles = -1.0f;
		TMap<FVector, FEmpathTargetingCandidateAdv> TargetingCandidates;

		// Get each possible location to score
		for (FEmpathPlayerAttackTarget& CurrTarget : PlayerAttackTargets)
		{
			// If the current target is valid
			if (CurrTarget.IsValid())
			{
				// Get the best aim location on the target for each search dir
				for (const FEmpathTargetingDirection& CurrTargetingDirection : SearchDirections)
				{
					// Check if this is a new location
					FVector TargetAimLocation;
					USceneComponent* TargetAimLocationComp;
					if (!UEmpathFunctionLibrary::GetAimLocationOnActor(CurrTarget.TargetActor, OriginLocation, CurrTargetingDirection.SearchDirection, TargetAimLocation, TargetAimLocationComp)
						|| TargetingCandidates.Contains(TargetAimLocation))
					{
						continue;
					}

					// Check if the distance to the target is within range
					FVector DirToTarget = TargetAimLocation - OriginLocation;
					float DistSquared = DirToTarget.SizeSquared();
					if (DistSquared > MaxRange * MaxRange)
					{
						continue;
					}

					// Check if the target is within angle range of any of the search directions max angles
					DirToTarget = DirToTarget.GetSafeNormal();
					FVector BestSearchDir;
					float BestAngle = 1000.0f;
					float TotalWeightedAngles = 0.0f;
					for (const FEmpathTargetingDirection& ScoringDir : SearchDirections)
					{
						FVector SearchDirNormalized = ScoringDir.SearchDirection.GetSafeNormal();
						float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DirToTarget | SearchDirNormalized));
						if (AngleToTarget <= ScoringDir.MaxAngle)
						{
							float ScoringWeight = ScoringDir.AngleWeight;
							if (ScoringDir.AngleScoreCurve)
							{
								ScoringWeight *= ScoringDir.AngleScoreCurve->GetFloatValue(AngleToTarget);
							}
							TotalWeightedAngles += ((180.0f - AngleToTarget) * ScoringWeight);
							if (BestAngle > AngleToTarget)
							{
								BestAngle = AngleToTarget;
								BestSearchDir = SearchDirNormalized;
							}
						}
					}
					if (BestAngle > 180.0f)
					{
						continue;
					}

					// Check if we have line of sight to the target
					// Setup trace params
					FCollisionQueryParams Params(PlayerTargetSelectionTraceTag);
					AEmpathPlayerCharacter* PlayerCharacter = Cast<AEmpathPlayerCharacter>(GetPawn());
					if (PlayerCharacter)
					{
						Params.AddIgnoredActors(PlayerCharacter->GetControlledActors());
					}
					Params.AddIgnoredActor(GetPawn());
					Params.AddIgnoredActor(CurrTarget.TargetActor);
					FCollisionResponseParams const ResponseParams = FCollisionResponseParams::DefaultResponseParam;
					FHitResult OutHit;

					// Do the actual raycast
					bool bHit = (GetPawn()->GetWorld()->LineTraceSingleByChannel(OutHit, OriginLocation, TargetAimLocation,
						ECC_Visibility, Params, ResponseParams));
					if (bHit && OutHit.Actor != CurrTarget.TargetActor)
					{
						continue;
					}

					// If we passed all checks, add the candidate target
					float DistToTarget = FMath::Sqrt(DistSquared);
					TargetingCandidates.Add(TargetAimLocation, FEmpathTargetingCandidateAdv(CurrTarget.TargetActor,
						TargetAimLocationComp,
						DistToTarget,
						TotalWeightedAngles,
						BestAngle,
						CurrTarget.TargetPreference,
						BestSearchDir,
						DirToTarget));

					// Update greatest dist squared and greatest sum angles if necessary
					if (GreatestDistance < DistToTarget)
					{
						GreatestDistance = DistToTarget;
					}
					if (GreatestWeightedAngles < TotalWeightedAngles)
					{
						GreatestWeightedAngles = TotalWeightedAngles;
					}
				}
			}
		}

		// Score each candidate location
		if (TargetingCandidates.Num() > 0)
		{
			float BestScore = -9999.0f;
			for (TPair<FVector, FEmpathTargetingCandidateAdv>& CurrCandidate : TargetingCandidates)
			{
				// Angle
				float TotalScore = 0.0f;
				UE_LOG(LogTemp, Warning, TEXT("Weighted Angles are %f"), CurrCandidate.Value.TotalWeightedAngles);
				TotalScore += (CurrCandidate.Value.TotalWeightedAngles / GreatestWeightedAngles) * AngleWeight;
				UE_LOG(LogTemp, Warning, TEXT("Angle Score is %f"), TotalScore);
				float AngleScore = TotalScore;

				// Distance
				UE_LOG(LogTemp, Warning, TEXT("Distance is %f"), CurrCandidate.Value.Distance);
				TotalScore += (1.0f - (CurrCandidate.Value.Distance / GreatestDistance)) * DistanceWeight;
				UE_LOG(LogTemp, Warning, TEXT("Distance Score is %f"), TotalScore - AngleScore);

				UE_LOG(LogTemp, Warning, TEXT("Total Score is %f"), TotalScore);
				// Targeting preference
				TotalScore *= CurrCandidate.Value.TargetPreference;

				// Update best targets and score if necessary
				if (TotalScore > BestScore)
				{
					BestScore = TotalScore;
					OutTargetAimLocation = CurrCandidate.Key;
				}
			}
			UE_LOG(LogTemp, Warning, TEXT("Best score is %f"), BestScore);
			// Return the target with the best score
			FEmpathTargetingCandidateAdv& BestTarget = TargetingCandidates[OutTargetAimLocation];
			UE_LOG(LogTemp, Warning, TEXT("Waited angles are %f"), BestTarget.TotalWeightedAngles);
			OutBestTarget = BestTarget.Actor;
			OutTargetAimLocationComp = BestTarget.AimLocationComponent;
			if (!OutTargetAimLocationComp && OutBestTarget)
			{
				OutTargetAimLocationComp = OutBestTarget->GetRootComponent();
			}
			OutBestSearchDirection = BestTarget.BestSearchDirection;
			OutDirectionToTarget = BestTarget.DirectionToTarget;
			OutBestAngleToTarget = BestTarget.BestAngle;
			OutDistanceToTarget = BestTarget.Distance;

			return true;
		}
	}

	// Return nothing if we found no appropriate targets
	OutBestTarget = nullptr;
	OutTargetAimLocation = FVector::ZeroVector;
	OutTargetAimLocationComp = nullptr;
	OutBestSearchDirection = FVector::ZeroVector;
	OutDirectionToTarget = FVector::ZeroVector;
	OutBestAngleToTarget = 0.0f;
	OutDistanceToTarget = 0.0f;
	return false;
}

bool AEmpathPlayerController::GetBestAttackTargetInDirectionsWithPrediction(AActor*& OutBestTarget,
	FVector& OutTargetAimLocation,
	USceneComponent*& OutTargetAimLocationComp,
	FVector& OutBestSearchDirection,
	FVector& OutDirectionToTarget,
	float& OutBestAngleToTarget,
	float& OutDistanceToTarget,
	const FVector& OriginLocation,
	const TArray<FEmpathTargetingDirection>& SearchDirections,
	float MaxRange /*= 10000.0f*/,
	float DistanceWeight /*= 1.0f*/,
	float AngleWeight /*= 1.0f*/,
	float AttackSpeed /*= 1500.0f*/,
	float Gravity /*= 0.0f*/,
	float MaxPredictionTime /*= 3.0f*/,
	float PredictionFailedPenalty /*= -0.5f*/)
{
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTargetSelection);

	CleanUpPlayerAttackTargets();
	if (PlayerAttackTargets.Num() > 0 && SearchDirections.Num() > 0.0f)
	{
		// Initialize variables
		float GreatestDistance = -1.0f;
		float GreatestWeightedAngles = -1.0f;
		TMap<FVector, FEmpathTargetingCandidateAdvPred> TargetingCandidates;

		// Get each possible location to score
		for (FEmpathPlayerAttackTarget& CurrTarget : PlayerAttackTargets)
		{
			// If the current target is valid
			if (CurrTarget.IsValid())
			{
				// Get the best aim location on the target for each search dir
				for (const FEmpathTargetingDirection& CurrTargetingDirection : SearchDirections)
				{
					// Check if this is a new location
					FVector TargetAimLocation;
					USceneComponent* TargetAimLocationComp;
					if (!UEmpathFunctionLibrary::GetAimLocationOnActor(CurrTarget.TargetActor, OriginLocation, CurrTargetingDirection.SearchDirection, TargetAimLocation, TargetAimLocationComp)
						|| TargetingCandidates.Contains(TargetAimLocation))
					{
						continue;
					}

					// Check if the distance to the target is within range
					float DistSquared = (TargetAimLocation - OriginLocation).SizeSquared();
					if (DistSquared > MaxRange * MaxRange)
					{
						continue;
					}

					// Try to predict the direction to the target
					FVector DirToTarget;
					FVector AimLocCopy = TargetAimLocation;
					bool bAimPredictionFailed = !(UEmpathFunctionLibrary::PredictBestAimDirection(TargetAimLocation, DirToTarget, OriginLocation, AimLocCopy, CurrTarget.TargetActor->GetVelocity(), AttackSpeed, Gravity, MaxPredictionTime));

					// Check if the target is within angle range of any of the search directions max angles
					DirToTarget = DirToTarget.GetSafeNormal();
					FVector BestSearchDir;
					float BestAngle = 1000.0f;
					float TotalWeightedAngles = 0.0f;
					for (const FEmpathTargetingDirection& ScoringDir : SearchDirections)
					{
						FVector SearchDirNormalized = ScoringDir.SearchDirection.GetSafeNormal();
						float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DirToTarget | SearchDirNormalized));
						if (AngleToTarget <= ScoringDir.MaxAngle)
						{
							TotalWeightedAngles += ((180.0f - AngleToTarget) * ScoringDir.AngleWeight);
							if (BestAngle > AngleToTarget)
							{
								BestAngle = AngleToTarget;
								BestSearchDir = SearchDirNormalized;
							}
						}
					}
					if (BestAngle > 180.0f)
					{
						continue;
					}

					// Check if we have line of sight to the target
					// Setup trace params
					FCollisionQueryParams Params(PlayerTargetSelectionTraceTag);
					AEmpathPlayerCharacter* PlayerCharacter = Cast<AEmpathPlayerCharacter>(GetPawn());
					if (PlayerCharacter)
					{
						Params.AddIgnoredActors(PlayerCharacter->GetControlledActors());
					}
					Params.AddIgnoredActor(GetPawn());
					Params.AddIgnoredActor(CurrTarget.TargetActor);
					FCollisionResponseParams const ResponseParams = FCollisionResponseParams::DefaultResponseParam;
					FHitResult OutHit;

					// Do the actual raycast
					bool bHit = (GetPawn()->GetWorld()->LineTraceSingleByChannel(OutHit, OriginLocation, TargetAimLocation,
						ECC_Visibility, Params, ResponseParams));
					if (bHit && OutHit.Actor != CurrTarget.TargetActor)
					{
						continue;
					}

					// If we passed all checks, add the candidate target
					float DistToTarget = FMath::Sqrt(DistSquared);
					TargetingCandidates.Add(TargetAimLocation, FEmpathTargetingCandidateAdvPred(CurrTarget.TargetActor,
						TargetAimLocationComp,
						DistToTarget,
						TotalWeightedAngles,
						BestAngle,
						CurrTarget.TargetPreference,
						BestSearchDir,
						DirToTarget,
						bAimPredictionFailed));

					// Update greatest dist squared and greatest sum angles if necessary
					if (GreatestDistance < DistToTarget)
					{
						GreatestDistance = DistToTarget;
					}
					if (GreatestWeightedAngles < TotalWeightedAngles)
					{
						GreatestWeightedAngles = TotalWeightedAngles;
					}
				}
			}
		}

		// Score each candidate location
		if (TargetingCandidates.Num() > 0)
		{
			float BestScore = -9999.0f;
			for (TPair<FVector, FEmpathTargetingCandidateAdvPred>& CurrCandidate : TargetingCandidates)
			{
				// Angle
				float TotalScore = 0.0f;
				TotalScore += (CurrCandidate.Value.TotalWeightedAngles / GreatestWeightedAngles) * AngleWeight;

				// Distance
				TotalScore += (1.0f - (CurrCandidate.Value.Distance / GreatestDistance)) * DistanceWeight;

				// Aim prediction
				if (CurrCandidate.Value.bFailedAimPrediction)
				{
					TotalScore += PredictionFailedPenalty;
				}

				// Targeting preference
				TotalScore *= CurrCandidate.Value.TargetPreference;

				// Update best targets and score if necessary
				if (TotalScore > BestScore)
				{
					BestScore = TotalScore;
					OutTargetAimLocation = CurrCandidate.Key;
				}
			}

			// Return the target with the best score
			FEmpathTargetingCandidateAdvPred& BestTarget = TargetingCandidates[OutTargetAimLocation];
			OutBestTarget = BestTarget.Actor;
			OutTargetAimLocationComp = BestTarget.AimLocationComponent;
			if (!OutTargetAimLocationComp && OutBestTarget)
			{
				OutTargetAimLocationComp = OutBestTarget->GetRootComponent();
			}
			OutBestSearchDirection = BestTarget.BestSearchDirection;
			OutDirectionToTarget = BestTarget.DirectionToTarget;
			OutBestAngleToTarget = BestTarget.BestAngle;
			OutDistanceToTarget = BestTarget.Distance;

			return true;
		}
	}

	// Return nothing if we found no appropriate targets
	OutBestTarget = nullptr;
	OutTargetAimLocation = FVector::ZeroVector;
	OutTargetAimLocationComp = nullptr;
	OutBestSearchDirection = FVector::ZeroVector;
	OutDirectionToTarget = FVector::ZeroVector;
	OutBestAngleToTarget = 0.0f;
	OutDistanceToTarget = 0.0f;
	return false;
}

bool AEmpathPlayerController::GetBestAttackTargetInConeSegmentWithPrediction(AActor*& OutBestTarget, 
	FVector& OutTargetAimLocation, 
	USceneComponent*& OutTargetAimLocationComp, 
	FVector& OutDirectionToTarget, 
	float& OutAngleToTarget, 
	float& OutDistanceToTarget, 
	const FVector& OriginLocation, 
	const FVector& RightEdge, 
	const FVector& LeftEdge, 
	float MaxRange /*= 10000.0f*/, 
	float MaxConeAngle /*= 15.0f*/, 
	float DistanceWeight /*= 1.0f*/, 
	float AngleWeight /*= 1.0f*/, 
	float AttackSpeed /*= 1500.0f*/, 
	float Gravity /*= 0.0f*/, 
	float MaxPredictionTime /*= 3.0f*/, 
	float PredictionFailedPenalty /*= -0.5f*/)
{
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTargetSelection);

	CleanUpPlayerAttackTargets();
	if (PlayerAttackTargets.Num() > 0)
	{
		// Initialize variables
		float GreatestDistance = 0.0f;
		float GreatestAngle = 0.0f;
		TArray<FEmpathTargetingCandidatePred> TargetCandidates;
		FVector RightEdgeNormalized = RightEdge.GetSafeNormal();
		FVector LeftEdgeNormalized = LeftEdge.GetSafeNormal();
		FVector SearchDirection = RightEdgeNormalized;
		FVector CrossNormalized = FVector::ZeroVector;
		float MaxHorizontalAngle = MaxConeAngle;
		if (RightEdgeNormalized != LeftEdgeNormalized)
		{
			MaxHorizontalAngle += (0.5f * FMath::RadiansToDegrees(FMath::Acos(RightEdgeNormalized | LeftEdgeNormalized)));
			SearchDirection = ((RightEdgeNormalized + LeftEdgeNormalized) * 0.5f).GetSafeNormal();
			CrossNormalized = FVector::CrossProduct(RightEdgeNormalized, LeftEdgeNormalized).GetSafeNormal();
		}

		// Get all possible targets
		for (FEmpathPlayerAttackTarget& CurrTarget : PlayerAttackTargets)
		{
			// If the current target is valid
			if (CurrTarget.IsValid())
			{
				// Get the aim location on the target
				FVector TargetAimLocation;
				USceneComponent* TargetAimLocationComponent;
				if (!UEmpathFunctionLibrary::GetAimLocationOnActor(CurrTarget.TargetActor, OriginLocation, SearchDirection, TargetAimLocation, TargetAimLocationComponent))
				{
					continue;
				}

				// Check if the target is too far away
				float DistToTargetSquared = (TargetAimLocation - OriginLocation).SizeSquared();
				if (DistToTargetSquared > MaxRange * MaxRange)
				{
					continue;
				}

				// Try to predict the best direction to the target
				FVector DirectionToTarget;
				FVector AimLocCopy = TargetAimLocation;
				bool bFailedAimPrediction = (!UEmpathFunctionLibrary::PredictBestAimDirection(TargetAimLocation, DirectionToTarget, OriginLocation, AimLocCopy, CurrTarget.TargetActor->GetVelocity(), AttackSpeed, Gravity, MaxPredictionTime));

				// Check if the target is within the maximum angle
				float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DirectionToTarget | SearchDirection));
				if (AngleToTarget > MaxHorizontalAngle)
				{
					continue;
				}
				if (MaxHorizontalAngle > MaxConeAngle)
				{
					float VerticalAngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(DirectionToTarget | CrossNormalized)));
					if (VerticalAngleToTarget < 90.0f - MaxConeAngle)
					{
						continue;
					}
				}

				// Check if we have line of sight to the target
				// Setup trace params
				FCollisionQueryParams Params(PlayerTargetSelectionTraceTag);
				AEmpathPlayerCharacter* PlayerCharacter = Cast<AEmpathPlayerCharacter>(GetPawn());
				if (PlayerCharacter)
				{
					Params.AddIgnoredActors(PlayerCharacter->GetControlledActors());
				}
				Params.AddIgnoredActor(GetPawn());
				Params.AddIgnoredActor(CurrTarget.TargetActor);
				FCollisionResponseParams const ResponseParams = FCollisionResponseParams::DefaultResponseParam;
				FHitResult OutHit;

				// Do the actual raycast
				bool bHit = (GetPawn()->GetWorld()->LineTraceSingleByChannel(OutHit, OriginLocation, TargetAimLocation,
					ECC_Visibility, Params, ResponseParams));
				if (bHit && OutHit.Actor != CurrTarget.TargetActor)
				{
					continue;
				}

				// If we passed all above checks, add to the list of candidates
				float DistToTarget = FMath::Sqrt(DistToTargetSquared);
				TargetCandidates.Add(FEmpathTargetingCandidatePred(CurrTarget.TargetActor,
					TargetAimLocation,
					DirectionToTarget,
					TargetAimLocationComponent,
					DistToTarget,
					AngleToTarget,
					CurrTarget.TargetPreference,
					bFailedAimPrediction));

				// Update greatest distance and angles if necessary
				if (DistToTarget > GreatestDistance)
				{
					GreatestDistance = DistToTarget;
				}
				if (AngleToTarget > GreatestAngle)
				{
					GreatestAngle = AngleToTarget;
				}
			}
		}

		// Score the targets
		FEmpathTargetingCandidatePred* BestTarget = nullptr;
		if (TargetCandidates.Num() > 0)
		{
			float BestScore = -99999.0f;
			int32 BestTargetIdx = -1;

			// Loop through each target to find the one with the best score
			for (FEmpathTargetingCandidatePred& CurrCandidate : TargetCandidates)
			{
				// Angle
				float TotalScore = (1.0f - (CurrCandidate.Angle / GreatestAngle)) * AngleWeight;

				// Distance
				TotalScore += (1.0f - (CurrCandidate.Distance / GreatestDistance)) * DistanceWeight;

				// Aim prediction
				if (CurrCandidate.bFailedAimPrediction)
				{
					TotalScore += PredictionFailedPenalty;
				}

				// Targeting preference
				TotalScore *= CurrCandidate.TargetPreference;

				// Update best targets and score if necessary
				if (TotalScore > BestScore)
				{
					BestScore = TotalScore;
					BestTarget = &CurrCandidate;
				}
			}

			// Return the target with the best total score
			OutBestTarget = BestTarget->Actor;
			OutTargetAimLocation = BestTarget->AimLocation;
			OutTargetAimLocationComp = BestTarget->AimLocationComponent;
			if (!OutTargetAimLocationComp && OutBestTarget)
			{
				OutTargetAimLocationComp = OutBestTarget->GetRootComponent();
			}
			OutDirectionToTarget = BestTarget->DirectionToTarget;
			OutAngleToTarget = BestTarget->Angle;
			OutDistanceToTarget = BestTarget->Distance;
			return true;
		}
	}

	OutBestTarget = nullptr;
	OutTargetAimLocation = FVector::ZeroVector;
	OutTargetAimLocationComp = nullptr;
	OutDirectionToTarget = FVector::ZeroVector;
	OutAngleToTarget = 0.0f;
	OutDistanceToTarget = 0.0f;
	return false;
}

bool AEmpathPlayerController::GetBestAttackTargetInConeSegment(AActor*& OutBestTarget, 
	FVector& OutTargetAimLocation, 
	USceneComponent*& OutTargetAimLocationComp, 
	FVector& OutDirectionToTarget, 
	float& OutAngleToTarget, 
	float& OutDistanceToTarget, 
	const FVector& OriginLocation, 
	const FVector& RightEdge, 
	const FVector& LeftEdge, 
	float MaxRange /*= 10000.0f*/,  
	float MaxConeAngle /*= 15.0f*/, 
	float DistanceWeight /*= 1.0f*/, 
	float AngleWeight /*= 1.0f*/)
{

	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTargetSelection);

	CleanUpPlayerAttackTargets();
	if (PlayerAttackTargets.Num() > 0)
	{
		// Initialize variables
		float GreatestDistToTarget = 0.0f;
		float GreatestAngle = 0.0f;
		TArray<FEmpathTargetingCandidate> TargetCandidates;
		FVector RightEdgeNormalized = RightEdge.GetSafeNormal();
		FVector LeftEdgeNormalized = LeftEdge.GetSafeNormal();
		float MaxHorizontalAngle = MaxConeAngle;
		FVector SearchDirection = RightEdgeNormalized;
		FVector CrossNormalized = FVector::ZeroVector;
		if (RightEdgeNormalized != LeftEdgeNormalized)
		{
			MaxHorizontalAngle += (0.5f * FMath::RadiansToDegrees(FMath::Acos(RightEdgeNormalized | LeftEdgeNormalized)));
			SearchDirection = ((RightEdgeNormalized + LeftEdgeNormalized) * 0.5f).GetSafeNormal();
			CrossNormalized = FVector::CrossProduct(RightEdgeNormalized, LeftEdgeNormalized).GetSafeNormal();
		}

		// Get all possible targets
		for (FEmpathPlayerAttackTarget& CurrTarget : PlayerAttackTargets)
		{
			// If the current target is valid
			if (CurrTarget.IsValid())
			{
				// Get the aim location on the target
				FVector TargetAimLocation;
				USceneComponent* TargetAimLocationComponent;
				if (!UEmpathFunctionLibrary::GetAimLocationOnActor(CurrTarget.TargetActor, OriginLocation, SearchDirection, TargetAimLocation, TargetAimLocationComponent))
				{
					continue;
				}

				// Check if the target is in range
				FVector DirectionToTarget = TargetAimLocation - OriginLocation;
				float DistToTargetSquared = DirectionToTarget.SizeSquared();
				if (DistToTargetSquared > MaxRange * MaxRange)
				{
					continue;
				}

				// Check if the target is within the maximum angles
				DirectionToTarget = DirectionToTarget.GetSafeNormal();
				float AngleToTarget = FMath::RadiansToDegrees(FMath::Acos(DirectionToTarget | SearchDirection));
				if (AngleToTarget > MaxHorizontalAngle)
				{
					continue;
				}
				if (MaxHorizontalAngle > MaxConeAngle)
				{
					float VerticalAngleToTarget = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(DirectionToTarget | CrossNormalized)));
					if (VerticalAngleToTarget < 90.0f - MaxConeAngle)
					{
						continue;
					}
				}

				// Check if we have line of sight to the target
				// Setup trace params
				FCollisionQueryParams Params(PlayerTargetSelectionTraceTag);
				AEmpathPlayerCharacter* PlayerCharacter = Cast<AEmpathPlayerCharacter>(GetPawn());
				if (PlayerCharacter)
				{
					Params.AddIgnoredActors(PlayerCharacter->GetControlledActors());
				}
				Params.AddIgnoredActor(GetPawn());
				Params.AddIgnoredActor(CurrTarget.TargetActor);
				FCollisionResponseParams const ResponseParams = FCollisionResponseParams::DefaultResponseParam;
				FHitResult OutHit;

				// Do the actual raycast
				bool bHit = (GetPawn()->GetWorld()->LineTraceSingleByChannel(OutHit, OriginLocation, TargetAimLocation,
					ECC_Visibility, Params, ResponseParams));
				if (bHit && OutHit.Actor != CurrTarget.TargetActor)
				{
					continue;
				}

				// If we passed all above checks, add to the list of candidates
				float DistToTarget = FMath::Sqrt(DistToTargetSquared);
				TargetCandidates.Add(FEmpathTargetingCandidate(CurrTarget.TargetActor,
					TargetAimLocation,
					DirectionToTarget,
					TargetAimLocationComponent,
					DistToTarget,
					AngleToTarget,
					CurrTarget.TargetPreference));

				// Update greatest distance and angles if necessary
				if (DistToTarget > GreatestDistToTarget)
				{
					GreatestDistToTarget = DistToTarget;
				}
				if (AngleToTarget > GreatestAngle)
				{
					GreatestAngle = AngleToTarget;
				}
			}
		}

		// Score the targets
		if (TargetCandidates.Num() > 0)
		{
			float BestScore = -99999.0f;
			FEmpathTargetingCandidate* BestTarget = nullptr;

			// Loop through each target to find the one with the best score
			for (FEmpathTargetingCandidate& CurrCandidate : TargetCandidates)
			{
				// Angle
				float TotalScore = (1.0f - (CurrCandidate.Angle / GreatestAngle)) * AngleWeight;

				// Distance
				TotalScore += (1.0f - (CurrCandidate.Distance / GreatestDistToTarget)) * DistanceWeight;

				// Targeting preference
				TotalScore *= CurrCandidate.TargetPreference;

				// Update best targets and score if necessary
				if (TotalScore > BestScore)
				{
					BestScore = TotalScore;
					BestTarget = &CurrCandidate;
				}
			}

			// Return the target with the best total score
			OutBestTarget = BestTarget->Actor;
			OutTargetAimLocation = BestTarget->AimLocation;
			OutTargetAimLocationComp = BestTarget->AimLocationComponent;
			if (!OutTargetAimLocationComp && OutBestTarget)
			{
				OutTargetAimLocationComp = OutBestTarget->GetRootComponent();
			}
			OutDirectionToTarget = BestTarget->DirectionToTarget;
			OutAngleToTarget = BestTarget->Angle;
			OutDistanceToTarget = BestTarget->Distance;
			return true;
		}
	}

	OutBestTarget = nullptr;
	OutTargetAimLocation = FVector::ZeroVector;
	OutTargetAimLocationComp = nullptr;
	OutDirectionToTarget = FVector::ZeroVector;
	OutAngleToTarget = 0.0f;
	OutDistanceToTarget = 0.0f;
	return false;
}

void AEmpathPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	EmpathPlayerCharacter = Cast<AEmpathPlayerCharacter>(GetPawn());
}