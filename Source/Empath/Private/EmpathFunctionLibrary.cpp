// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathFunctionLibrary.h"
#include "EmpathPlayerCharacter.h"
#include "EmpathGameModeBase.h"
#include "EmpathAimLocationInterface.h"
#include "AIController.h"
#include "EmpathCharacter.h"
#include "NavigationSystem/Public/NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "EmpathBishopManager.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "EmpathPlayerController.h"
#include "EmpathGameInstance.h"
#include "EmpathGameSettings.h"
#include "EmpathTimeDilator.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "EmpathGenericEventInterface.h"
#include "EmpathDamageType.h"

DECLARE_CYCLE_STAT(TEXT("PredictProjectilePath"), STAT_EMPATH_PredictProjectilePath, STATGROUP_EMPATH_FunctionLibrary);
DECLARE_CYCLE_STAT(TEXT("SuggestProjectileVelocity_CustomArc"), STAT_EMPATH_SuggestProjectileVelocity, STATGROUP_EMPATH_FunctionLibrary);

// Static consts
static const float MaxImpulsePerMass = 5000.f;
static const float MaxPointImpulseMag = 120000.f;

const bool UEmpathFunctionLibrary::GetAimLocationOnActor(const AActor* Actor, FVector LookOrigin, FVector LookDirection, FVector& OutAimLocation, USceneComponent*& OutAimLocationComponent)
{
	if (Actor)
	{
		// First, check for the aim location interface
		if (Actor->GetClass()->ImplementsInterface(UEmpathAimLocationInterface::StaticClass()))
		{
			return IEmpathAimLocationInterface::Execute_GetCustomAimLocationOnActor(Actor, LookOrigin, LookDirection, OutAimLocation, OutAimLocationComponent);

		}

		// Next, check if the object as a controller. If so, call this function in its pawn instead.
		const AController* ControllerTarget = Cast<AController>(Actor);
		if (ControllerTarget && ControllerTarget->GetPawn())
		{
			return GetAimLocationOnActor(ControllerTarget->GetPawn(), LookOrigin, LookDirection, OutAimLocation, OutAimLocationComponent);
		}


		// If all else fails, return the center mass location of the actor and the root component of the actor
		OutAimLocation = GetCenterMassLocationOnActor(Actor);
		OutAimLocationComponent = Actor->GetRootComponent();

		return true;
	}

	// If the actor is invalid, return false
	OutAimLocation = FVector::ZeroVector;
	OutAimLocationComponent = nullptr;

	return false;

}

const FVector UEmpathFunctionLibrary::GetCenterMassLocationOnActor(const AActor* Actor)
{
	// Check if the object is the vr player
	const AEmpathPlayerCharacter* VRChar = Cast<AEmpathPlayerCharacter>(Actor);
	if (VRChar)
	{
		// Aim at the defined center mass of the vr character
		return VRChar->GetVRLocation();
	}

	// Else, get the center of the bounding box
	if (Actor)
	{
		// Aim at bounds center
		FBox BoundingBox = Actor->GetComponentsBoundingBox();

		// Add Child Actor components to bounding box (since we use them)
		// Workaround for GetActorBounds not considering child actor components
		for (const UActorComponent* ActorComponent : Actor->GetComponents())
		{
			UChildActorComponent const* const CAComp = Cast<const UChildActorComponent>(ActorComponent);
			AActor const* const CA = CAComp ? CAComp->GetChildActor() : nullptr;
			if (CA)
			{
				BoundingBox += CA->GetComponentsBoundingBox();
			}
		}

		if (BoundingBox.IsValid)
		{
			FVector BoundsOrigin = BoundingBox.GetCenter();
			return BoundsOrigin;
		}
	}

	// If all else fails, aim at the target's location
	return Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
}

AEmpathAIManager* UEmpathFunctionLibrary::GetAIManager(const UObject* WorldContextObject)
{
	AEmpathGameModeBase* EmpathGMD = WorldContextObject->GetWorld()->GetAuthGameMode<AEmpathGameModeBase>();
	if (EmpathGMD)
	{
		return EmpathGMD->GetAIManager();
	}
	return nullptr;
}

AEmpathSoundManager* UEmpathFunctionLibrary::GetSoundManager(const UObject* WorldContextObject)
{
	AEmpathGameModeBase* EmpathGMD = WorldContextObject->GetWorld()->GetAuthGameMode<AEmpathGameModeBase>();
	if (EmpathGMD)
	{
		return EmpathGMD->GetSoundManager();
	}
	return nullptr;
}

AEmpathBishopManager* UEmpathFunctionLibrary::GetBishopManager(const UObject* WorldContextObject)
{
	AEmpathGameModeBase* EmpathGMD = WorldContextObject->GetWorld()->GetAuthGameMode<AEmpathGameModeBase>();
	if (EmpathGMD)
	{
		return EmpathGMD->GetBishopManager();
	}
	return nullptr;
}

const bool UEmpathFunctionLibrary::IsPlayer(AActor* Actor)
{
	if (Actor)
	{
		AController* PlayerCon = Actor->GetWorld()->GetFirstPlayerController();
		if (PlayerCon)
		{
			if (PlayerCon == Actor)
			{
				return true;
			}
			if (PlayerCon->GetPawn() == Actor)
			{
				return true;
			}
		}
	}
	return false;
}

EEmpathTeam UEmpathFunctionLibrary::GetActorTeam(const AActor* Actor)
{
	if (Actor)
	{
		// First check for the team agent interface on the actor
		{
			if (Actor->GetClass()->ImplementsInterface(UEmpathTeamAgentInterface::StaticClass()))
			{
				return IEmpathTeamAgentInterface::Execute_GetTeamNum(Actor);
			}
		}

		// Next, check on the controller
		{
			APawn const* const TargetPawn = Cast<APawn>(Actor);
			if (TargetPawn)
			{
				AController const* const TargetController = Cast<AController>(TargetPawn);
				if (TargetController && TargetController->GetClass()->ImplementsInterface(UEmpathTeamAgentInterface::StaticClass()))
				{
					return IEmpathTeamAgentInterface::Execute_GetTeamNum(TargetController);
				}
			}
		}

		// Next, check the instigator
		{
			AActor const* TargetInstigator = Actor->Instigator;
			if (TargetInstigator && TargetInstigator->GetClass()->ImplementsInterface(UEmpathTeamAgentInterface::StaticClass()))
			{
				return IEmpathTeamAgentInterface::Execute_GetTeamNum(Actor->Instigator);
			}
		}

		// Finally, check if the instigator controller has a team
		{
			AController* const InstigatorController = Actor->GetInstigatorController();
			if (InstigatorController && InstigatorController->GetClass()->ImplementsInterface(UEmpathTeamAgentInterface::StaticClass()))
			{
				return IEmpathTeamAgentInterface::Execute_GetTeamNum(InstigatorController);
			}
		}
	}

	// By default, return neutral
	return EEmpathTeam::Neutral;
}

const ETeamAttitude::Type UEmpathFunctionLibrary::GetTeamAttitudeBetween(const AActor* A, const AActor* B)
{
	// Get the teams
	EEmpathTeam TeamA = GetActorTeam(A);
	EEmpathTeam TeamB = GetActorTeam(B);

	// If either of them are neutral, we don't care about them
	if (TeamA == EEmpathTeam::Neutral || TeamB == EEmpathTeam::Neutral)
	{
		return ETeamAttitude::Neutral;
	}

	// If they're the same, return frendly
	if (TeamA == TeamB)
	{
		return ETeamAttitude::Friendly;
	}

	// Otherwise, they're hostile
	return ETeamAttitude::Hostile;
}

void UEmpathFunctionLibrary::AddDistributedImpulseAtLocation(USkeletalMeshComponent* SkelMesh, FVector Impulse, FVector Location, FName BoneName, float DistributionPct)
{
	if (Impulse.IsNearlyZero() || (SkelMesh == nullptr))
	{
		return;
	}

	// 
	// overall strategy here: 
	// add some portion of the overall impulse distributed across all bodies in a radius, with linear falloff, but preserving impulse direction
	// the remaining portion is applied as a straight impulse to the given body at the given location
	// Desired outcome is similar output to a simple hard whack at the loc/bone, but more stable.
	//

	// compute total mass, per-bone distance info
	TArray<float> BodyDistances;
	BodyDistances.AddZeroed(SkelMesh->Bodies.Num());
	float MaxDistance = 0.f;

	float TotalMass = 0.0f;
	for (int32 i = 0; i < SkelMesh->Bodies.Num(); ++i)
	{
		FBodyInstance* const BI = SkelMesh->Bodies[i];
		if (BI->IsValidBodyInstance())
		{
			TotalMass += BI->GetBodyMass();

			FVector const BodyLoc = BI->GetUnrealWorldTransform().GetLocation();
			float Dist = (Location - BodyLoc).Size();
			BodyDistances[i] = Dist;
			MaxDistance = FMath::Max(MaxDistance, Dist);
		}
	}

	// sanity, since we divide with these
	TotalMass = FMath::Max(TotalMass, KINDA_SMALL_NUMBER);
	MaxDistance = FMath::Max(MaxDistance, KINDA_SMALL_NUMBER);

	float const OriginalImpulseMag = Impulse.Size();
	FVector const ImpulseDir = Impulse / OriginalImpulseMag;
	float const DistributedImpulseMag = OriginalImpulseMag * DistributionPct;
	float const PointImpulseMag = FMath::Min((OriginalImpulseMag - DistributedImpulseMag), MaxPointImpulseMag);

	const float DistributedImpulseMagPerMass = FMath::Min((DistributedImpulseMag / TotalMass), MaxImpulsePerMass);
	for (int32 i = 0; i < SkelMesh->Bodies.Num(); ++i)
	{
		FBodyInstance* const BI = SkelMesh->Bodies[i];

		if (BI->IsValidBodyInstance())
		{
			// linear falloff
			float const ImpulseFalloffScale = FMath::Max(0.f, 1.f - (BodyDistances[i] / MaxDistance));
			const float ImpulseMagForThisBody = DistributedImpulseMagPerMass * BI->GetBodyMass() * ImpulseFalloffScale;
			if (ImpulseMagForThisBody > 0.f)
			{
				BI->AddImpulse(ImpulseDir*ImpulseMagForThisBody, false);
			}
		}
	}

	// add the rest as a point impulse on the loc
	SkelMesh->AddImpulseAtLocation(ImpulseDir * PointImpulseMag, Location, BoneName);
}

const FVector UEmpathFunctionLibrary::ConvertWorldDirectionToComponentSpace(const USceneComponent* SceneComponent, const FVector Direction)
{
	if (SceneComponent)
	{
		return SceneComponent->GetComponentTransform().InverseTransformVectorNoScale(Direction);
	}
	return FVector::ZeroVector;
}

const FVector UEmpathFunctionLibrary::ConvertWorldDirectionToActorSpace(const AActor* Actor, const FVector Direction)
{
	if (Actor)
	{
		return Actor->GetActorTransform().InverseTransformVectorNoScale(Direction);
	}
	return FVector::ZeroVector;
}

const FVector UEmpathFunctionLibrary::ConvertComponentDirectionToWorldSpace(const USceneComponent* SceneComponent, const FVector Direction)
{
	if (SceneComponent)
	{
		return SceneComponent->GetComponentTransform().TransformVectorNoScale(Direction);
	}
	return FVector::ZeroVector;
}

const FVector UEmpathFunctionLibrary::ConvertComponentDirectionToActorSpace(const USceneComponent* SceneComponent, const AActor* Actor, const FVector Direction)
{
	if (Actor && SceneComponent)
	{
		return Actor->GetActorTransform().InverseTransformVectorNoScale(SceneComponent->GetComponentTransform().TransformVectorNoScale(Direction));
	}
	return FVector::ZeroVector;
}

const FVector UEmpathFunctionLibrary::ConvertActorDirectionToWorldSpace(const AActor* Actor, const FVector Direction)
{
	if (Actor)
	{
		return Actor->GetActorTransform().TransformVectorNoScale(Direction);
	}
	return FVector::ZeroVector;
}


const FVector UEmpathFunctionLibrary::ConvertActorDirectionToComponentSpace(const AActor* Actor, const USceneComponent* SceneComponent, const FVector Direction)
{
	if (Actor && SceneComponent)
	{
		return SceneComponent->GetComponentTransform().InverseTransformVectorNoScale(Actor->GetActorTransform().TransformVectorNoScale(Direction));
	}
	return FVector::ZeroVector;
}

const float UEmpathFunctionLibrary::GetMagnitudeInDirection(const FVector Vector, const FVector Direction)
{
	return FVector::DotProduct(Vector, Direction.GetSafeNormal());
}

bool UEmpathFunctionLibrary::PredictProjectilePath(const UObject* WorldContextObject, FHitResult& OutHit, TArray<FVector>& OutPathPositions, FVector& OutLastTraceDestination, FVector StartPos, FVector LaunchVelocity, bool bTracePath, float ProjectileRadius, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, float DrawDebugTime, float SimFrequency, float MaxSimTime)
{
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PredictProjectilePath);
	return UGameplayStatics::Blueprint_PredictProjectilePath_ByObjectType(WorldContextObject, OutHit, OutPathPositions, OutLastTraceDestination, StartPos, LaunchVelocity, bTracePath, ProjectileRadius, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, DrawDebugTime, SimFrequency, MaxSimTime);
}

bool UEmpathFunctionLibrary::SuggestProjectileVelocity(const UObject* WorldContextObject, FVector& OutLaunchVelocity, FVector StartPos, FVector EndPos, float Arc)
{
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_SuggestProjectileVelocity);
	return UGameplayStatics::SuggestProjectileVelocity_CustomArc(WorldContextObject, OutLaunchVelocity, StartPos, EndPos, /*OverrideCustomGravityZ=*/ 0.f, Arc);
}


void UEmpathFunctionLibrary::CalculateJumpTimings(const UObject* WorldContextObject, FVector LaunchVelocity, FVector StartLocation, FVector EndLocation, float& OutAscendingTime, float& OutDescendingTime)
{
	// Cache variables
	UWorld* World = WorldContextObject->GetWorld();
	float const Gravity = World->GetGravityZ();
	float const XYSpeed = LaunchVelocity.Size2D();
	float const XYDistance = FVector::DistXY(StartLocation, EndLocation);
	float const TotalTime = XYDistance / XYSpeed;

	// If we are ascending for part of the jump
	if (LaunchVelocity.Z > 0.0f)
	{
		// Calculate how long until we hit the apex of the jump
		float TimeToApex = -LaunchVelocity.Z / Gravity;

		// We hit the destination while still ascending
		if (TimeToApex > TotalTime)
		{
			OutAscendingTime = TotalTime;
			OutDescendingTime = 0.0f;
		}

		// Descent time is the remaining time after ascending
		else
		{
			OutAscendingTime = TimeToApex;
			OutDescendingTime = TotalTime - OutAscendingTime;
		}
	}

	// We are descending for the entire jump
	else
	{
		OutAscendingTime = 0.0f;
		OutDescendingTime = TotalTime;
	}
}

bool UEmpathFunctionLibrary::EmpathProjectPointToNavigation(const UObject* WorldContextObject, FVector& ProjectedPoint, FVector Point, ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass, const FVector QueryExtent)
{
	UWorld* const World = WorldContextObject->GetWorld();
	UNavigationSystemV1* const NavSys = UNavigationSystemV1::GetCurrent(World);
	if (NavSys)
	{
		// If no nav data is provided, try and find one from the world context object.
		// Should work if it is a Character or AController.
		if (NavData == nullptr)
		{
			const FNavAgentProperties* AgentProps = nullptr;
			if (const INavAgentInterface* AgentContext = Cast<INavAgentInterface>(WorldContextObject))
			{
				AgentProps = &AgentContext->GetNavAgentPropertiesRef();
			}

			NavData = (AgentProps ? NavSys->GetNavDataForProps(*AgentProps) : NavSys->GetDefaultNavDataInstance());
		}

		// If no nav filter is provided, try and find one from the world context object.
		// Should work if it is an AController.
		if (FilterClass == nullptr)
		{
			const AAIController* AI = Cast<AAIController>(WorldContextObject);
			if (AI == nullptr)
			{
				if (const APawn* Pawn = Cast<APawn>(WorldContextObject))
				{
					AI = Cast<AAIController>(Pawn->GetController());
				}
			}

			if (AI)
			{
				FilterClass = AI->GetDefaultNavigationFilterClass();
			}
		}

		// Finally, do the actually projection
		if (NavData)
		{
			FNavLocation ProjectedNavLoc(Point);
			bool const bRet = NavSys->ProjectPointToNavigation(Point, ProjectedNavLoc, (QueryExtent.IsNearlyZero() ? INVALID_NAVEXTENT : QueryExtent), NavData,
				UNavigationQueryFilter::GetQueryFilter(*NavData, WorldContextObject, FilterClass));
			ProjectedPoint = ProjectedNavLoc.Location;
			return bRet;
		}
	}

	return false;
}

bool UEmpathFunctionLibrary::EmpathHasPathToLocation(AEmpathCharacter* EmpathCharacter, FVector Destination, ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass)
{
	UWorld* const World = EmpathCharacter->GetWorld();
	UNavigationSystemV1* const NavSys = UNavigationSystemV1::GetCurrent(World);
	if (NavSys)
	{
		// Use correct nav mesh
		if (NavData == nullptr)
		{
			// If WorldContextObject is a Character or AIController, get agent properties from them.
			const FNavAgentProperties* AgentProps = &EmpathCharacter->GetNavAgentPropertiesRef();
			NavData = (AgentProps ? NavSys->GetNavDataForProps(*AgentProps) : NavSys->GetDefaultNavDataInstance());
		}

		// Use correct filter class
		if (FilterClass == nullptr)
		{
			if (AAIController* AI = Cast<AAIController>(EmpathCharacter->GetController()))
			{
				FilterClass = AI->GetDefaultNavigationFilterClass();
			}
		}

		// Now do the path query
		if (NavData)
		{
			const FVector SourceLocation = EmpathCharacter->GetPathingSourceLocation();
			FSharedConstNavQueryFilter NavFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, EmpathCharacter, FilterClass);
			FPathFindingQuery Query(*EmpathCharacter, *NavData, SourceLocation, Destination, NavFilter);
			Query.SetAllowPartialPaths(false);

			const bool bPathExists = NavSys->TestPathSync(Query, EPathFindingMode::Regular);
			return bPathExists;
		}
	}

	return false;
}

float UEmpathFunctionLibrary::GetUndilatedDeltaTime(const UObject* WorldContextObject)
{
	AEmpathGameModeBase* EmpathGMD = WorldContextObject->GetWorld()->GetAuthGameMode<AEmpathGameModeBase>();
	if (EmpathGMD)
	{
		AEmpathTimeDilator* TimeDilator = EmpathGMD->GetTimeDilator();
		if (TimeDilator)
		{
			return TimeDilator->GetUndilatedDeltaTime();
		}
	}

	return 0.0f;
}

AEmpathPlayerController* UEmpathFunctionLibrary::GetEmpathPlayerCon(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject->GetWorld();
	if (World)
	{
		return Cast<AEmpathPlayerController>(World->GetFirstPlayerController());
	}
	return nullptr;
}

const bool UEmpathFunctionLibrary::DistanceGreaterThan(const FVector A, const FVector B, const float Threshold)
{
	return FVector::DistSquared(A, B) > FMath::Pow(Threshold, 2);
}

const bool UEmpathFunctionLibrary::DistanceLessThan(const FVector A, const FVector B, const float Threshold)
{
	return FVector::DistSquared(A, B) < FMath::Pow(Threshold, 2);
}

const bool UEmpathFunctionLibrary::VectorLengthLessThanFloat(const FVector A, const float Threshold)
{
	return A.SizeSquared() < FMath::Pow(Threshold, 2);
}

const bool UEmpathFunctionLibrary::VectorLengthGreaterThanFloat(const FVector A, const float Threshold)
{
	return A.SizeSquared() > FMath::Pow(Threshold, 2);
}

const bool UEmpathFunctionLibrary::VectorLengthLessThanVector(const FVector A, const FVector B)
{
	return A.SizeSquared() < B.SizeSquared();
}

const bool UEmpathFunctionLibrary::VectorLengthGreaterThanVector(const FVector A, const FVector B)
{
	return A.SizeSquared() > B.SizeSquared();
}

UEmpathGameInstance* UEmpathFunctionLibrary::GetEmpathGameInstance(const UObject* WorldContextObject)
{
	return Cast<UEmpathGameInstance>(WorldContextObject->GetWorld()->GetGameInstance());
}

UEmpathGameSettings* UEmpathFunctionLibrary::GetEmpathGameSettings(const UObject* WorldContextObject)
{
	if (UEmpathGameInstance* EGI = Cast<UEmpathGameInstance>(WorldContextObject->GetWorld()->GetGameInstance()))
	{
		return EGI->LoadedSettings;
	}
	return nullptr;
}

const float UEmpathFunctionLibrary::DistanceBetweenLocations(const FVector A, const FVector B)
{
	return (A - B).Size();
}

const float UEmpathFunctionLibrary::DistanceBetweenLocations2D(const FVector A, const FVector B)
{
	return (A - B).Size2D();
}

AEmpathTimeDilator* UEmpathFunctionLibrary::GetTimeDilator(const UObject* WorldContextObject)
{
	AEmpathGameModeBase* EmpathGMD = WorldContextObject->GetWorld()->GetAuthGameMode<AEmpathGameModeBase>();
	if (EmpathGMD)
	{
		return EmpathGMD->GetTimeDilator();
	}
	return nullptr;
}

float UEmpathFunctionLibrary::GetRealTimeSince(const UObject* WorldContextObject, float RealTimeStamp)
{
	return WorldContextObject->GetWorld()->GetRealTimeSeconds() - RealTimeStamp;
}

bool UEmpathFunctionLibrary::SaveStringArray(FString SaveDirectory, FString FileName, TArray<FString>& Lines, bool bAllowOverwriting)
{
	// Set complete file path
	SaveDirectory = FPaths::ProfilingDir() + SaveDirectory;
	SaveDirectory += "\\";
	SaveDirectory += FileName;

	// Check if file already exists
	if (!bAllowOverwriting)
	{
		// If so, don't overwrite unless allowed
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*SaveDirectory))
		{
			return false;
		}
	}

	// Write each string to the file.
	FString FileString = "";
	for (FString& CurrString : Lines)
	{
		FileString += CurrString;
		FileString += LINE_TERMINATOR;
	}

	// Finally save the string
	return FFileHelper::SaveStringToFile(FileString, *SaveDirectory);
}

bool UEmpathFunctionLibrary::SaveStringArrayToCSV(FString SaveDirectory, FString FileName, TArray<FString>& Lines, bool bAllowOverwriting, bool bAlwaysWrite)
{

	// Remove an extension suffixes from the string
	int32 ExtensionStartIndex = 0;
	if (FileName.FindChar('.', ExtensionStartIndex))
	{
		FileName.RemoveAt(ExtensionStartIndex, FileName.Len() - ExtensionStartIndex, true);
	}

	// Add the date to the file name
	FileName += "_";
	FileName += FDateTime::Now().ToString();

	// Set complete file path
	SaveDirectory = FPaths::ProjectDir() + SaveDirectory;
	SaveDirectory += "\\";
	SaveDirectory += FileName;

	// Check if file already exists
	if (!bAllowOverwriting)
	{
		// If so, don't overwrite unless allowed
		FString SuggestedSaveDirectory = SaveDirectory;
		SuggestedSaveDirectory += ".csv";
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*SuggestedSaveDirectory) && !bAlwaysWrite)
		{
			return false;
		}

		// If always write, add to the file name
		else while (FPlatformFileManager::Get().GetPlatformFile().FileExists(*SuggestedSaveDirectory))
		{
			SaveDirectory += "_1";
			SuggestedSaveDirectory = SaveDirectory;
			SuggestedSaveDirectory += ".csv";
		}
	}

	// Finally, add the CSV suffix
	SaveDirectory += ".csv";

	// Write each string to the file.
	FString FileString = "";
	for (FString& CurrString : Lines)
	{
		FileString += CurrString;
		FileString += LINE_TERMINATOR;
	}

	// Finally save the string
	return FFileHelper::SaveStringToFile(FileString, *SaveDirectory, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
}

const FString UEmpathFunctionLibrary::VectorToCSV(FVector Input)
{
	FString ReturnValue = "";
	ReturnValue += FString::SanitizeFloat(Input.X);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.Y);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.Z);
	return ReturnValue;
}

const FString UEmpathFunctionLibrary::GestureCheckToCSV(FEmpathGestureCheck& Input)
{
	FString ReturnValue = "";
	ReturnValue += VectorToCSV(Input.CheckVelocity);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.VelocityMagnitude);
	ReturnValue += ",";
	ReturnValue += VectorToCSV(Input.AngularVelocity);
	ReturnValue += ",";
	ReturnValue += VectorToCSV(Input.ScaledAngularVelocity);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.SphericalVelocity);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.RadialVelocity);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.VerticalVelocity);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.SphericalDist);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.RadialDist);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.VerticalDist);
	ReturnValue += ",";
	ReturnValue += VectorToCSV(Input.MotionAngle);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.AccelMagnitude);
	ReturnValue += ",";
	ReturnValue += VectorToCSV(Input.CheckAcceleration);
	ReturnValue += ",";
	ReturnValue += VectorToCSV(Input.AngularAcceleration);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.SphericalAccel);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.RadialAccel);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.VerticalAccel);

	return ReturnValue;
}

const FString UEmpathFunctionLibrary::GestureCheckDebugToCSV(FEmpathOneHandGestureConditionCheckDebug& Input)
{
	FString ReturnValue = "";
	ReturnValue += GestureCheckToCSV(Input.GestureConditionState);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.MovementSinceStart);
	ReturnValue += ",";
	ReturnValue += FString::SanitizeFloat(Input.TimeSinceStart);

	return ReturnValue;
}

AEmpathPlayerCharacter* UEmpathFunctionLibrary::GetEmpathPlayerChar(const UObject* WorldContextObject)
{
	if (AEmpathPlayerController* PlayerCon = GetEmpathPlayerCon(WorldContextObject))
	{
		return PlayerCon->GetControlledEmpathPlayerChar();
	}
	return nullptr;
}

bool UEmpathFunctionLibrary::GetLineFromString(UPARAM(ref) const FString& InString, FString& OutLine, FString& OutRemainder, bool& OutWordTruncated, const int32 MaxLength)
{
	// Protect against invalid input
	if (MaxLength < 1 || InString.Len() < 2)
	{
		OutLine = "";
		OutRemainder = "";
		OutWordTruncated = false;
		return false;
	}

	// Check if the string is already small enough
	if (InString.Len() <= MaxLength)
	{
		OutLine = InString;
		OutRemainder = "";
		OutWordTruncated = false;
		return true;
	}

	// Loop through the string to find the nearest whitespace
	int32 CurrIdx = MaxLength;
	int32 MinIdx = 0;

	while (CurrIdx >= MinIdx && InString[CurrIdx] != ' ')
	{
		CurrIdx--;
	}

	// If we found a whitespace, truncate the string there
	if (CurrIdx >= 0)
	{
		OutLine = InString.Mid(0, CurrIdx);
		OutRemainder = InString.Mid(CurrIdx + 1, InString.Len() - (CurrIdx));
		OutWordTruncated = false;
		return true;
	}

	// Otherwise, split the string at the end
	else
	{
		// Truncate the string
		OutLine = InString.Mid(0, MaxLength);
		FString Holder = InString.Mid(MaxLength, InString.Len() - (MaxLength));

		// Add symbols to show a word was split if appropriate
		OutRemainder = "";
		OutRemainder += Holder;
		OutWordTruncated = true;
		return true;
	}
}

void UEmpathFunctionLibrary::GetLastWordFromString(const FString& InString, FString& OutString)
{
	int32 CurrIdx = InString.Len() - 1;
	while (CurrIdx > 0 && InString[CurrIdx] != ' ')
	{
		CurrIdx--;
	}
	if (CurrIdx > 0)
	{
		OutString = InString.Mid(CurrIdx + 1, InString.Len() - (CurrIdx));
	}
	else
	{
		OutString = InString;
	}
	return;
}

void UEmpathFunctionLibrary::GetFirstWordFromString(const FString& InString, FString& OutString)
{
	int32 CurrIdx = 0;
	while (CurrIdx < InString.Len() && InString[CurrIdx] != ' ')
	{
		CurrIdx++;
	}
	if (CurrIdx < InString.Len())
	{
		OutString = InString.Mid(0, CurrIdx);
	}
	else
	{
		OutString = InString;
	}
	return;
}

bool UEmpathFunctionLibrary::GetLinesFromString(const FString& InString, TArray<FString>& OutLines, const int32 MaxLength, const bool bAllowTruncationMarks, const bool bBridgeLines)
{
	// Empty array to start
	OutLines.Empty();

	// Initialize variables
	FString LineString = "";
	FString Remainder = InString;
	FString RemainderHolder = "";
	bool bWordTruncated;

	// Loop through the remainder of the string, adding lines as necessary
	do
	{
		// Attempt to get the next string
		if (GetLineFromString(Remainder, LineString, RemainderHolder, bWordTruncated, MaxLength))
		{
			// Add truncation marks if necessary
			if (bWordTruncated && bAllowTruncationMarks)
			{
				LineString += "-";
				Remainder = "-";
				Remainder += RemainderHolder;
			}
			else
			{
				// Check if we should bridge
				if (bBridgeLines && !bWordTruncated)
				{
					FString LastWord;
					GetLastWordFromString(LineString, LastWord);
					FString FirstWord;
					GetFirstWordFromString(RemainderHolder, FirstWord);
					if (LastWord.Len() + FirstWord.Len() < MaxLength)
					{
						Remainder = LastWord;
						Remainder += " ";
						Remainder += RemainderHolder;
					}
					else
					{
						Remainder = RemainderHolder;
					}
				}
				else
				{
					Remainder = RemainderHolder;
				}
			}
			OutLines.Add(LineString);
		}
		// Otherwise, abort the loop
		else
		{
			return false;
		}

	} while (Remainder.Len() > MaxLength);

	// Add the last line
	if (Remainder.Len() > 0)
	{
		OutLines.Add(Remainder);
	}

	return true;
}

AEmpathGameModeBase* UEmpathFunctionLibrary::GetEmpathGameMode(const UObject* WorldContextObject)
{
	return Cast<AEmpathGameModeBase>(WorldContextObject->GetWorld()->GetAuthGameMode());
}

bool UEmpathFunctionLibrary::CalibrateWorldMetersScale(const UObject* WorldContextObject)
{
	// Grab the player, the game instance, and he settings
	AEmpathPlayerCharacter* PlayerChar = GetEmpathPlayerChar(WorldContextObject);
	if (PlayerChar && PlayerChar->LeftMotionController && PlayerChar->RightMotionController)
	{
		UEmpathGameInstance* GameInstance = GetEmpathGameInstance(WorldContextObject);
		if (GameInstance)
		{
			UEmpathGameSettings* GameSettings = GameInstance->LoadedSettings;
			if (GameSettings)
			{
				// Get the average of their arm distance and eye height, adjusted to account for total head height
				float NewWorldMetersScale = (PlayerChar->GetVREyeHeight() + 7.5f +
					UEmpathFunctionLibrary::DistanceBetweenLocations(
						PlayerChar->RightMotionController->GetComponentLocation(),
						PlayerChar->LeftMotionController->GetComponentLocation())) / 2.0f;

				// Fix the size to account for the current world meters scale
				NewWorldMetersScale /= (UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(PlayerChar) / 100.0f);

				// Get the fixed height as a fraction of the base height and multiply it by 100 to get the new world meters scale
				NewWorldMetersScale = (GameInstance->PlayerBaseHeight / NewWorldMetersScale);
				NewWorldMetersScale *= 100.0f;

				// Clamp the final value and update the world meters scale
				NewWorldMetersScale = FMath::Clamp(NewWorldMetersScale, GameInstance->MinWorldMetersScale, GameInstance->MaxWorldMetersScale);
				UHeadMountedDisplayFunctionLibrary::SetWorldToMetersScale(PlayerChar, NewWorldMetersScale);

				// Update the game settings
				GameSettings->CalibratedWorldMetersScale = NewWorldMetersScale;
				GameInstance->SaveSettings();

				return true;
			}
		}
	}
	return false;

}

EEmpathGestureType UEmpathFunctionLibrary::FromOneHandGestureTypeToGestureType(EEmpathOneHandGestureType TypeToConvert)
{
	switch (TypeToConvert)
	{
	case EEmpathOneHandGestureType::Punching:
	{
		return EEmpathGestureType::Punching;
		break;
	}
	case EEmpathOneHandGestureType::Slashing:
	{
		return EEmpathGestureType::Slashing;
		break;
	}
	default:
	{
		return EEmpathGestureType::NoGesture;
		break;
	}
	}
}

EEmpathGestureType UEmpathFunctionLibrary::FromCastingPoseToGestureType(EEmpathCastingPose PoseToConvert)
{
	switch (PoseToConvert)
	{
	case EEmpathCastingPose::CannonShotDynamic:
	{
		return EEmpathGestureType::CannonShotDynamic;
		break;
	}
	case EEmpathCastingPose::CannonShotStatic:
	{
		return EEmpathGestureType::CannonShotStatic;
		break;
	}
	default:
	{
		return EEmpathGestureType::NoGesture;
		break;
	}
	}
}

FVector UEmpathFunctionLibrary::GetActorLocationVR(const AActor* Actor)
{
	if (const AVRCharacter* VRChar = Cast<AVRCharacter>(Actor))
	{
		return VRChar->GetVRLocation();
	}
	return Actor->GetActorLocation();
}

FRotator UEmpathFunctionLibrary::GetActorRotationVR(const AActor* Actor)
{
	if (const AVRCharacter* VRChar = Cast<AVRCharacter>(Actor))
	{
		return VRChar->GetVRRotation();
	}
	return Actor->GetActorRotation();
}

FString UEmpathFunctionLibrary::PlayerProgressIdxToSectionName(int progressIdx) {
	switch (progressIdx) {
	case 0:
		return "Alleyway";
		break;
	case 1:
		return "Alleyway";
		break;
	case 2:
		return "Marketplace";
		break;
	case 3:
		return "Rooftops";
		break;
	case 4:
		return "ClockTower";
	default:
		return "";
	}
}

bool UEmpathFunctionLibrary::PredictBestAimDirection(FVector& OutPredictedLoc, FVector& OutDirection, const FVector& Origin, const FVector& TargetLocation, const FVector& TargetVelocity, float Speed /*= 1500.0f*/, float Gravity /*= -980.0f*/, float MaxPredictionTime /*= 3.0f*/)
{

	// Get the delta location to the target
	FVector LocationDelta = TargetLocation - Origin;

	// If the target is not moving or the speed is <= 0, simply return the direction to the target
	if (TargetVelocity.SizeSquared() <= 1.0f || Speed <= 0.0f || LocationDelta.SizeSquared() < 1.0f)
	{
		OutDirection = LocationDelta.GetSafeNormal();
		OutPredictedLoc = TargetLocation;
		return true;
	}
	float SpeedSquared = Speed * Speed;
	float TargetSpeedSquared = TargetVelocity.SizeSquared();
	float TargetSpeed = FMath::Sqrt(TargetSpeedSquared);
	FVector TargetVelocityDir = TargetVelocity.GetSafeNormal();
	FVector TargetToOrigin = Origin - TargetLocation;
	float TargetToOriginDistSquared = TargetToOrigin.SizeSquared();
	float TargetToOriginDist = FMath::Sqrt(TargetToOriginDistSquared);
	FVector TargetToOriginDir = TargetToOrigin.GetSafeNormal();

	float CosTheta = (TargetToOriginDir | TargetVelocityDir);

	bool bFoundValidLocation = true;
	float SimTime = 1.0f;
	if (FMath::IsNearlyEqual(SpeedSquared, TargetSpeedSquared))
	{
		if (CosTheta > 0)
		{
			SimTime = 0.5f * TargetToOriginDist / (TargetSpeed * CosTheta);
		}
		else
		{
			bFoundValidLocation = false;
		}
	}
	else
	{
		float A = SpeedSquared - TargetSpeedSquared;
		float B = 2.0f * TargetToOriginDist * TargetSpeed * CosTheta;
		float C = -TargetToOriginDistSquared;
		float Discriminant = B * B - 4.0f * A * C;

		if (Discriminant < 0)
		{
			// Square root of a negative number is an imaginary number (NaN)
			bFoundValidLocation = false;
		}
		else
		{
			// We know from the above check that the root will never be 0
			float DiscriminantRoot = FMath::Sqrt(Discriminant);
			float ImpactTimeOne = 0.5f * (-B + DiscriminantRoot) / A;
			float ImpactTimeTwo = 0.5f * (-B - DiscriminantRoot) / A;

			// Assign the lowest positive time to t to aim at the earliest hit
			SimTime = FMath::Min(ImpactTimeOne, ImpactTimeTwo);
			if (SimTime < KINDA_SMALL_NUMBER)
			{
				SimTime = FMath::Max(ImpactTimeOne, ImpactTimeTwo);
				if (SimTime < KINDA_SMALL_NUMBER)
				{
					// Invalid time
					bFoundValidLocation = false;
				}
			}

			// If the predicted impact time is beyond our prediction time, this is a failure
			if (MaxPredictionTime > 0.0f && SimTime > MaxPredictionTime)
			{
				bFoundValidLocation = false;
			}
		}
	}
	if (bFoundValidLocation)
	{
		OutDirection = TargetVelocity + (-TargetToOrigin / SimTime);
		OutPredictedLoc = TargetLocation + (TargetVelocity * SimTime);
		if (Gravity > -0.01f)
		{
			OutDirection = OutDirection.GetSafeNormal();
		}
		else
		{
			FVector GravityComponsation = (Gravity * FVector(0.0f, 0.0f, 1.0f)) * 0.5f * SimTime;

			// Cap gravityCompensation to avoid AIs that shoot infinitely high
			float GravityCompensationCap = 0.9f * Speed;
			if (GravityComponsation.SizeSquared() > GravityCompensationCap * GravityCompensationCap)
			{
				GravityComponsation = GravityCompensationCap * GravityComponsation.GetSafeNormal();
			}
			OutDirection -= GravityComponsation;
		}

	}
	else
	{
		OutDirection = LocationDelta.GetSafeNormal();
		OutPredictedLoc = TargetLocation;
	}

	return bFoundValidLocation;
}

void UEmpathFunctionLibrary::BroadcastGenericEvent(const UObject* WorldContextObject, const FName EventName)
{
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithInterface(WorldContextObject, UEmpathGenericEventInterface::StaticClass(), OutActors);
	for (AActor* CurrActor : OutActors)
	{
		IEmpathGenericEventInterface::Execute_ReceiveGenericEvent(CurrActor, EventName);
	}
	return;
}

float UEmpathFunctionLibrary::ModifyDamageForFriendlyFire(const float DamageAmount, const UDamageType* DamageType, const AController* EventInstigator, const AActor* DamageCauser, const bool bAllowFriendlyFire, const EEmpathTeam FriendlyTeam)
{
	if (EventInstigator || DamageCauser)
	{
		EEmpathTeam const InstigatorTeam = GetActorTeam((EventInstigator ? EventInstigator : DamageCauser));
		if (InstigatorTeam == FriendlyTeam)
		{
			// If this damage came from our team and we can't take friendly fire, do nothing
			if (!bAllowFriendlyFire)
			{
				return 0.0f;
			}

			// Otherwise, scale the damage by the friendly fire damage multiplier
			else
			{
				UEmpathDamageType const* EmpathDamageTypeCDO = Cast<UEmpathDamageType>(DamageType);
				if (EmpathDamageTypeCDO)
				{
					float AdjustedDamage = DamageAmount * EmpathDamageTypeCDO->FriendlyFireDamageMultiplier;

					// If our damage was negated, return
					if (AdjustedDamage <= 0.0f)
					{
						return 0.0f;
					}
					else
					{
						return AdjustedDamage;
					}
				}
			}
		}
	}

	return DamageAmount;
}
