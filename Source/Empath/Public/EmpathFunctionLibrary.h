// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EmpathTeamAgentInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EmpathFunctionLibrary.generated.h"

// Stat groups for UE Profiler
DECLARE_STATS_GROUP(TEXT("Empath Function Library"), STATGROUP_EMPATH_FunctionLibrary, STATCAT_Advanced);

class AEmpathAIManager;
class AEmpathCharacter;
class AEmpathBishopManager;
class AEmpathPlayerController;
class AEmpathTimeDilator;
class UEmpathGameInstance;
class UEmpathGameSettings;
class AEmpathPlayerCharacter;
class AEmpathGameModeBase;
class AEmpathSoundManager;

/**
 * 
 */
UCLASS()
class EMPATH_API UEmpathFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Gets the aim location of an actor (or its pawn if it is a controller). 
	First checks for Aim Location interface, then calls GetCenterMassLocation. 
	Look direction is used with the interface when multiple aim locations are possible. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility")
	static const bool GetAimLocationOnActor(const AActor* Actor, FVector LookOrigin, FVector LookDirection, FVector& OutAimLocation, USceneComponent*& OutAimLocationComponent);

	/** Gets the center mass location of an Actor, or the VR Location if it is a VR Character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector GetCenterMassLocationOnActor(const AActor* Actor);

	/** Gets the angle between 2 vectors. (Input does not need to be normalized.) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const float AngleBetweenVectors(const FVector& A, const FVector& B);

	/** Gets the angle between 2 vectors on the X and Y plane only. (Input does not need to be normalized.) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const float AngleBetweenVectors2D(const FVector& A, const FVector& B);

	/** Gets the angle between 2 vectors on the Z and radial plane only. (Input does not need to be normalized.) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const float AngleBetweenVectorsZ(const FVector& A, const FVector& B);

	/** Gets the angle and axis between 2 vectors. (Input does not need to be normalized.) */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility")
	static void AngleAndAxisBetweenVectors(const FVector& A, const FVector& B, float &Angle, FVector& Axis);

	/** Gets the world's AI Manager. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|AI", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static AEmpathAIManager* GetAIManager(const UObject* WorldContextObject);

	/** Gets the world's Sound Manager. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|AI", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static AEmpathSoundManager* GetSoundManager(const UObject* WorldContextObject);

	/** Gets the world's bishop Manager. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|AI", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static AEmpathBishopManager* GetBishopManager(const UObject* WorldContextObject);

	/** Returns whether an actor is the player. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|AI")
	static const bool IsPlayer(AActor* Actor);

	/** Gets the team of the target actor. Includes fall-backs to check the targets instigator and controller if no team is found. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Teams")
	static EEmpathTeam GetActorTeam(const AActor* Actor);

	/** Gets the attitude of two actors towards each other. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Teams")
	static const ETeamAttitude::Type GetTeamAttitudeBetween(const AActor* A, const AActor* B);

	/** Add distributed impulse function, (borrowed from Robo Recall so if it causes problems, lets just remove it. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Physics")
	static void AddDistributedImpulseAtLocation(USkeletalMeshComponent* SkelMesh, FVector Impulse, FVector Location, FName BoneName, float DistributionPct = 0.5f);

	/** Transforms a world direction (such as velocity) to be expressed in a component's relative space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector ConvertWorldDirectionToComponentSpace(const USceneComponent* SceneComponent, const FVector Direction);

	/** Transforms a world direction (such as velocity) to be expressed in an actors's relative space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector ConvertWorldDirectionToActorSpace(const AActor* Actor, const FVector Direction);

	/** Transforms a component direction (such as forward) to be expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector ConvertComponentDirectionToWorldSpace(const USceneComponent* SceneComponent, const FVector Direction);

	/** Transforms a component direction (such as forward) to be expressed in an actors's relative space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector ConvertComponentDirectionToActorSpace(const USceneComponent* SceneComponent, const AActor* Actor, const FVector Direction);

	/** Transforms an actor direction (such as forward) to be expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector ConvertActorDirectionToWorldSpace(const AActor* Actor, const FVector Direction);

	/** Transforms an actor direction (such as forward) to be expressed in a component's relative space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector ConvertActorDirectionToComponentSpace(const AActor* Actor, const USceneComponent* SceneComponent, const FVector Direction);

	/** Gets how far one vector (such as velocity) travels along a unit direction vector (such as world up). Automatically normalizes the directional vector. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const float GetMagnitudeInDirection(const FVector Vector, const FVector Direction);

	/**
	* Returns true if it hit something.
	* @param OutPathPositions			Predicted projectile path. Ordered series of positions from StartPos to the end. Includes location at point of impact if it hit something.
	* @param OutHit						Predicted hit result, if the projectile will hit something
	* @param OutLastTraceDestination	Goal position of the final trace it did. Will not be in the path if there is a hit.
	*/
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", AdvancedDisplay = "DrawDebugTime, DrawDebugType", TraceChannel = ECC_WorldDynamic, bTracePath = true))
	static bool PredictProjectilePath(const UObject* WorldContextObject, FHitResult& OutHit, TArray<FVector>& OutPathPositions, FVector& OutLastTraceDestination, FVector StartPos, FVector LaunchVelocity, bool bTracePath, float ProjectileRadius, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, float DrawDebugTime, float SimFrequency = 30.f, float MaxSimTime = 2.f);

	/**
	* Returns the launch velocity needed for a projectile at rest at StartPos to land on EndPos.
	* @param Arc	In range (0..1). 0 is straight up, 1 is straight at target, linear in between. 0.5 would give 45 deg arc if Start and End were at same height.
	* Projectile launch speed is variable and unconstrained.
	* Does no tracing.
	*/
	UFUNCTION(BlueprintCallable, BlueprintCallable, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject"))
	static bool SuggestProjectileVelocity(const UObject* WorldContextObject, FVector& OutLaunchVelocity, FVector StartPos, FVector EndPos, float Arc = 0.5f);

	/** Determines the ascent and descent time of a jump. */
	UFUNCTION(BlueprintCallable, BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static void CalculateJumpTimings(const UObject* WorldContextObject, FVector LaunchVelocity, FVector StartLocation, FVector EndLocation, float& OutAscendingTime, float& OutDescendingTime);

	/** Custom navigation projection that uses the agent, rather than the world, as a fallback for if no nav data or filter class is provided. */
	UFUNCTION(BlueprintCallable, BlueprintCallable, Category = "EmpathFunctionLibrary|AI", meta = (WorldContext = "WorldContextObject"))
	static bool EmpathProjectPointToNavigation(const UObject* WorldContextObject, FVector& ProjectedPoint, FVector Point, ANavigationData* NavData = nullptr, TSubclassOf<UNavigationQueryFilter> FilterClass = nullptr, const FVector QueryExtent = FVector::ZeroVector);

	/** Custom query that uses the agent, rather than the world, as a fallback for if no nav data or filter class is provided. */
	UFUNCTION(BlueprintCallable, BlueprintCallable, Category = "EmpathFunctionLibrary|AI")
	static bool EmpathHasPathToLocation(AEmpathCharacter* EmpathCharacter, FVector Point, ANavigationData* NavData = nullptr, TSubclassOf<UNavigationQueryFilter> FilterClass = nullptr);

	/** Gets delta time in real seconds, unaffected by global time dilation. Does NOT account for game paused. */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static float GetUndilatedDeltaTime(const UObject* WorldContextObject);


	// Wrappers to BreakHitResult that just does individual fields, not copying all members of the struct just to read 1 at a time (which is done in blueprints).
	// Also doesn't litter nativized builds with lots of extra variables.

	/** Indicates if this hit was a result of blocking collision. If false, there was no hit or it was an overlap/touch instead. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_BlockingHit(const struct FHitResult& Hit, bool& bBlockingHit);

	/**
	* Whether the trace started in penetration, i.e. with an initial blocking overlap.
	* In the case of penetration, if PenetrationDepth > 0.f, then it will represent the distance along the Normal vector that will result in
	* minimal contact between the swept shape and the object that was hit. In this case, ImpactNormal will be the normal opposed to movement at that location
	* (ie, Normal may not equal ImpactNormal). ImpactPoint will be the same as Location, since there is no single impact point to report.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_InitialOverlap(const struct FHitResult& Hit, bool& bInitialOverlap);

	/**
	* 'Time' of impact along trace direction (ranging from 0.0 to 1.0) if there is a hit, indicating time between TraceStart and TraceEnd.
	* For swept movement (but not queries) this may be pulled back slightly from the actual time of impact, to prevent precision problems with adjacent geometry.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_Time(const struct FHitResult& Hit, float& Time);

	/** The distance from the TraceStart to the Location in world space. This value is 0 if there was an initial overlap (trace started inside another colliding object). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_Distance(const struct FHitResult& Hit, float& Distance);

	/**
	* The location in world space where the moving shape would end up against the impacted object, if there is a hit. Equal to the point of impact for line tests.
	* Example: for a sphere trace test, this is the point where the center of the sphere would be located when it touched the other object.
	* For swept movement (but not queries) this may not equal the final location of the shape since hits are pulled back slightly to prevent precision issues from overlapping another surface.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_Location(const struct FHitResult& Hit, FVector& Location);

	/**
	* Location in world space of the actual contact of the trace shape (box, sphere, ray, etc) with the impacted object.
	* Example: for a sphere trace test, this is the point where the surface of the sphere touches the other object.
	* @note: In the case of initial overlap (bStartPenetrating=true), ImpactPoint will be the same as Location because there is no meaningful single impact point to report.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_ImpactPoint(const struct FHitResult& Hit, FVector& ImpactPoint);

	/**
	* Normal of the hit in world space, for the object that was swept. Equal to ImpactNormal for line tests.
	* This is computed for capsules and spheres, otherwise it will be the same as ImpactNormal.
	* Example: for a sphere trace test, this is a normalized vector pointing in towards the center of the sphere at the point of impact.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_Normal(const struct FHitResult& Hit, FVector& Normal);

	/**
	* Normal of the hit in world space, for the object that was hit by the sweep, if any.
	* For example if a box hits a flat plane, this is a normalized vector pointing out from the plane.
	* In the case of impact with a corner or edge of a surface, usually the "most opposing" normal (opposed to the query direction) is chosen.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_ImpactNormal(const struct FHitResult& Hit, FVector& ImpactNormal);

	/**
	* Physical material that was hit.
	* @note Must set bReturnPhysicalMaterial on the swept PrimitiveComponent or in the query params for this to be returned.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_PhysMat(const struct FHitResult& Hit, class UPhysicalMaterial*& PhysMat);

	/** Actor hit by the trace. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_Actor(const struct FHitResult& Hit, class AActor*& HitActor);

	/** PrimitiveComponent hit by the trace. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_Component(const struct FHitResult& Hit, class UPrimitiveComponent*& HitComponent);

	/** Name of the _my_ bone which took part in hit event (in case of two skeletal meshes colliding). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_MyBoneName(const struct FHitResult& Hit, FName& MyHitBoneName);

	/** Name of bone we hit (for skeletal meshes). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_BoneName(const struct FHitResult& Hit, FName& HitBoneName);

	/** Extra data about item that was hit (hit primitive specific). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_Item(const struct FHitResult& Hit, int32& HitItem);

	/** Face index we hit (for complex hits with triangle meshes). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_FaceIndex(const struct FHitResult& Hit, int32& FaceIndex);

	/**
	* Start location of the trace.
	* For example if a sphere is swept against the world, this is the starting location of the center of the sphere.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_TraceStart(const struct FHitResult& Hit, FVector& TraceStart);

	/**
	* End location of the trace; this is NOT where the impact occurred (if any), but the furthest point in the attempted sweep.
	* For example if a sphere is swept against the world, this would be the center of the sphere if there was no blocking hit.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakHit_TraceEnd(const struct FHitResult& Hit, FVector& TraceEnd);

	/** Gets the Empath Player Controller of the player character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static AEmpathPlayerController* GetEmpathPlayerCon(const UObject* WorldContextObject);

	/** Returns whether the distance between the two points is greater than the inputted threshold. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const bool DistanceGreaterThan(const FVector A, const FVector B, const float Threshold);

	/** Returns whether the distance between the two points is lass than the inputted threshold. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const bool DistanceLessThan(const FVector A, const FVector B, const float  Threshold);

	/** Returns whether the length of the vector is less than the inputted threshold. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const bool VectorLengthLessThanFloat(const FVector Vector, const float Threshold);

	/** Returns whether the length of the vector is greater than the inputted threshold. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const bool VectorLengthGreaterThanFloat(const FVector Vector, const float Threshold);

	/** Returns whether the length of vector A is less than vector B. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const bool VectorLengthLessThanVector(const FVector A, const FVector B);

	/** Returns whether the length of vector A is greater than vector B. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const bool VectorLengthGreaterThanVector(const FVector A, const FVector B);

	/** Gets the Empath game instance. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static UEmpathGameInstance* GetEmpathGameInstance(const UObject* WorldContextObject);

	/** Gets the Empath game settings. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static UEmpathGameSettings* GetEmpathGameSettings(const UObject* WorldContextObject);

	/** Gets the distance between two locations. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const float DistanceBetweenLocations(const FVector A, const FVector B);

	/** Gets the distance between two locations in X and Y only. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const float DistanceBetweenLocations2D(const FVector A, const FVector B);

	/** Gets the world's AI Manager. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static AEmpathTimeDilator* GetTimeDilator(const UObject* WorldContextObject);

	/** Gets the real time seconds since an event, in undilated time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static float GetRealTimeSince(const UObject* WorldContextObject, float RealTimeStamp);

	/** Saves an array of strings to a file. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Logging", meta = (Keywords = "Save"))
	static bool SaveStringArray(FString SaveDirectory, FString FileName, UPARAM(ref) TArray<FString>& Lines, bool bAllowOverwriting);

	/** Saves an array of strings to an CSV file. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Logging", meta = (Keywords = "Save"))
	static bool SaveStringArrayToCSV(FString SaveDirectory, FString FileName, UPARAM(ref) TArray<FString>& Lines, bool bAllowOverwriting, bool bAlwaysWrite);

	/** Converts a vector to a CSV formated string. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Logging", meta = (Keywords = "String"))
	static const FString VectorToCSV(FVector Input);

	/** Converts gesture condition state into a CSV formatted string. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Gestures")
	static const FString GestureCheckToCSV(UPARAM(ref) FEmpathGestureCheck& Input);

	/** Converts debug gesture condition state into a CSV formatted string. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Gestures")
	static const FString GestureCheckDebugToCSV(UPARAM(ref) FEmpathOneHandGestureConditionCheckDebug& Input);

	/** Flattens a vector on the Z plane. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector FlattenVector(const FVector& Input);

	/** Flattens a vector on the X and Y planes. Radial length is stored in X. Radial sign is preserved */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static const FVector FlattenVectorXY(const FVector& Input);

	/** Gets a ref to the Empath player character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static AEmpathPlayerCharacter* GetEmpathPlayerChar(const UObject* WorldContextObject);

	/** The text to display. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakDialogueBlock_Text(const struct FEmpathDialogueBlock& DialogueBlock, FString& Text);

	/** The AK audio event to play. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakDialogueBlock_AkAudioEvent(const struct FEmpathDialogueBlock& DialogueBlock, UAkAudioEvent*& AkAudioEvent);

	/** The sound to play if there is no inputted AK audio event. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakDialogueBlock_AudioClip(const struct FEmpathDialogueBlock& DialogueBlock, USoundBase*& AudioClip);

	/** The sound to play if there is no inputted AK audio event. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakDialogueBlock_MinDelayAfterEnd(const struct FEmpathDialogueBlock& DialogueBlock, float& MinDelayAfterEnd);

	/** The name of the speaker */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakConversationSpeaker_SpeakerName(const struct FEmpathConversationSpeaker& ConversationSpeaker, FString& SpeakerName);
	
	/** A reference to the speaker component that will show/play the dialogue. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakConversationSpeaker_SpeakerRef(const struct FEmpathConversationSpeaker& ConversationSpeaker, UEmpathSpeakerComponent*& SpeakerRef);

	/*
	* Gets a line from the string, automatically trimming down to the maximum length and avoiding breaking any words.
	* Returns whether the operation completed successfully.
	*/
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility")
	static bool GetLineFromString(UPARAM(ref) const FString& InString, FString& OutLine, FString& OutRemainder, bool& OutWordTruncated, const int32 MaxLength = 1);

	/** Gets the last word from the end of a string. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void GetLastWordFromString(UPARAM(ref) const FString& InString, FString& OutString);

	/** Gets the first word from the beginning of a string. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void GetFirstWordFromString(UPARAM(ref) const FString& InString, FString& OutString);

	/*
	* Converts a string into a series of lines with a maximum character length per line, automatically trimming each line down to the maximum length and avoiding breaking any words. 
	* Returns whether the operation completed successfully.
	*/
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility")
	static bool GetLinesFromString(const FString& InString, TArray<FString>& OutLines, const int32 MaxLength = 1, const bool bAllowTruncationMarks = true, const bool bBridgLines = true);

	/** Gets the Empath Game Mode. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static AEmpathGameModeBase* GetEmpathGameMode(const UObject* WorldContextObject);

	/** Calibrates the world meters scale based on height and arm distance. Returns true if successful. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static bool CalibrateWorldMetersScale(const UObject* WorldContextObject);

	/** Converts a one handed gesture type to a universal gesture type */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static EEmpathGestureType FromOneHandGestureTypeToGestureType(EEmpathOneHandGestureType TypeToConvert);

	/** Converts a one handed gesture type to a universal gesture type */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static EEmpathGestureType FromCastingPoseToGestureType(EEmpathCastingPose PoseToConvert);

	/**	The hand's location when beginning to enter this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastEntryStartLocation(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastEntryStartLocation);

	/**	The hand's rotation when beginning to enter this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastEntryStartRotation(const struct FEmpathGestureTransformCache& TransformCache, FRotator& LastEntryStartRotation);

	/** The motion angle of the hand when beginning to enter this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastEntryStartMotionAngle(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastEntryStartMotionAngle);

	/**	The hand's velocity when beginning to enter this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastEntryStartVelocity(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastEntryStartVelocity);

	/**	The hand's location when beginning to exit this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastExitStartLocation(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastExitStartLocation);

	/**	The hand's rotation when beginning to exit this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastExitStartRotation(const struct FEmpathGestureTransformCache& TransformCache, FRotator& LastExitStartRotation);

	/** The motion angle of the hand when beginning to enter this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastExitStartMotionAngle(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastExitStartMotionAngle);

	/**	The hand's velocity when beginning to enter this gesture. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void BreakGTC_LastExitStartVelocity(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastExitStartVelocity);

	/**	Returns A divided by B, or 0 if B is 0. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static void SafeDivideZero(const float& A, const float& B, float& ReturnValue);

	/** Returns the location of the Actor, accounting for the possibility that it may be a VR Character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static FVector GetActorLocationVR(const AActor* Actor);

	/** Returns the rotation of the Actor, accounting for the possibility that it may be a VR Character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static FRotator GetActorRotationVR(const AActor* Actor);

	/** Returns the rotation of the Actor, accounting for the possibility that it may be a VR Character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathFunctionLibrary|Utility")
	static FString PlayerProgressIdxToSectionName(int progressIdx);

	/*
	* Gets the optimal direction for intersecting the target at its current velocity. 
	* If it fails to find an intersect point within the radius and range, returns the direction to the target.
	* @param ForesightSeconds	Clamped between 0.5 and 3.
	* @param AccuracyRadius		Min of 5
	*/
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility")
	static bool PredictBestAimDirection(FVector& OutPredictedLoc, FVector& OutDirection, const FVector& Origin, const FVector& TargetLocation, const FVector& TargetVelocity, float Speed = 1500.0f, float Gravity = -980.0f, float MaxPredictionTime = 3.0f);

	/** Call to broadcast an event to all actors that implement the Empath Generic Event interface. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static void BroadcastGenericEvent(const UObject* WorldContextObject, const FName EventName);

	/** Generic function for modifying damage to account for friendly fire. */
	UFUNCTION(BlueprintCallable, Category = "EmpathFunctionLibrary|Utility")
	static float ModifyDamageForFriendlyFire(const float DamageAmount, const UDamageType* DamageType, const AController* EventInstigator, const AActor* DamageCauser, const bool bAllowFriendlyFire, const EEmpathTeam FriendlyTeam);
};


// Inline functions for efficiency
FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_BlockingHit(const struct FHitResult& Hit, bool& bBlockingHit)
{
	bBlockingHit = Hit.bBlockingHit;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_InitialOverlap(const struct FHitResult& Hit, bool& bInitialOverlap)
{
	bInitialOverlap = Hit.bStartPenetrating;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_Time(const struct FHitResult& Hit, float& Time)
{
	Time = Hit.Time;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_Distance(const struct FHitResult& Hit, float& Distance)
{
	Distance = Hit.Distance;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_Location(const struct FHitResult& Hit, FVector& Location)
{
	Location = Hit.Location;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_ImpactPoint(const struct FHitResult& Hit, FVector& ImpactPoint)
{
	ImpactPoint = Hit.ImpactPoint;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_Normal(const struct FHitResult& Hit, FVector& Normal)
{
	Normal = Hit.Normal;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_ImpactNormal(const struct FHitResult& Hit, FVector& ImpactNormal)
{
	ImpactNormal = Hit.ImpactNormal;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_PhysMat(const struct FHitResult& Hit, class UPhysicalMaterial*& PhysMat)
{
	PhysMat = Hit.PhysMaterial.Get();
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_Actor(const struct FHitResult& Hit, class AActor*& HitActor)
{
	HitActor = Hit.Actor.Get();
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_Component(const struct FHitResult& Hit, class UPrimitiveComponent*& HitComponent)
{
	HitComponent = Hit.Component.Get();
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_BoneName(const struct FHitResult& Hit, FName& HitBoneName)
{
	HitBoneName = Hit.BoneName;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_MyBoneName(const struct FHitResult& Hit, FName& MyHitBoneName)
{
	MyHitBoneName = Hit.MyBoneName;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_Item(const struct FHitResult& Hit, int32& HitItem)
{
	HitItem = Hit.Item;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_FaceIndex(const struct FHitResult& Hit, int32& FaceIndex)
{
	FaceIndex = Hit.FaceIndex;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_TraceStart(const struct FHitResult& Hit, FVector& TraceStart)
{
	TraceStart = Hit.TraceStart;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakHit_TraceEnd(const struct FHitResult& Hit, FVector& TraceEnd)
{
	TraceEnd = Hit.TraceEnd;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakDialogueBlock_Text(const struct FEmpathDialogueBlock& DialogueBlock, FString& Text)
{
	Text = DialogueBlock.Text;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakDialogueBlock_AkAudioEvent(const struct FEmpathDialogueBlock& DialogueBlock, UAkAudioEvent*& AkAudioEvent)
{
	AkAudioEvent = DialogueBlock.AkAudioEvent;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakDialogueBlock_AudioClip(const struct FEmpathDialogueBlock& DialogueBlock, USoundBase*& AudioClip)
{
	AudioClip = DialogueBlock.AudioClip;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakConversationSpeaker_SpeakerName(const struct FEmpathConversationSpeaker& ConversationSpeaker, FString& SpeakerName)
{
	SpeakerName = ConversationSpeaker.SpeakerName;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakConversationSpeaker_SpeakerRef(const struct FEmpathConversationSpeaker& ConversationSpeaker, UEmpathSpeakerComponent*& SpeakerRef)
{
	SpeakerRef = ConversationSpeaker.SpeakerRef;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakDialogueBlock_MinDelayAfterEnd(const struct FEmpathDialogueBlock& DialogueBlock, float& MinDelayAfterEnd)
{
	MinDelayAfterEnd = DialogueBlock.MinDelayAfterEnd;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastEntryStartLocation(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastEntryStartLocation)
{
	LastEntryStartLocation = TransformCache.LastEntryStartLocation;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastEntryStartRotation(const struct FEmpathGestureTransformCache& TransformCache, FRotator& LastEntryStartRotation)
{
	LastEntryStartRotation = TransformCache.LastEntryStartRotation;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastEntryStartMotionAngle(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastEntryStartMotionAngle)
{
	LastEntryStartMotionAngle = TransformCache.LastEntryStartMotionAngle;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastEntryStartVelocity(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastEntryStartVelocity)
{
	LastEntryStartVelocity = TransformCache.LastEntryStartVelocity;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastExitStartLocation(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastExitStartLocation)
{
	LastExitStartLocation = TransformCache.LastExitStartLocation;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastExitStartRotation(const struct FEmpathGestureTransformCache& TransformCache, FRotator& LastExitStartRotation)
{
	LastExitStartRotation = TransformCache.LastExitStartRotation;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastExitStartMotionAngle(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastExitStartMotionAngle)
{
	LastExitStartMotionAngle = TransformCache.LastExitStartMotionAngle;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::BreakGTC_LastExitStartVelocity(const struct FEmpathGestureTransformCache& TransformCache, FVector& LastExitStartVelocity)
{
	LastExitStartVelocity = TransformCache.LastExitStartVelocity;
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::SafeDivideZero(const float& A, const float& B, float& ReturnValue)
{
	ReturnValue = (B!=0.0f? A / B: 0.0f);
}

FORCEINLINE_DEBUGGABLE const FVector UEmpathFunctionLibrary::FlattenVector(const FVector& Input)
{
	return FVector(Input.X, Input.Y, 0.0f);
}

FORCEINLINE_DEBUGGABLE const FVector UEmpathFunctionLibrary::FlattenVectorXY(const FVector& Input)
{
	// Get the magnitude
	float RadialMagnitude = Input.Size2D();

	// Restore the sign
	if (Input.X < 0.0f)
	{
		RadialMagnitude *= -1.0f;
	}

	return FVector(RadialMagnitude, 0.0f, Input.Z);
}

FORCEINLINE_DEBUGGABLE const float UEmpathFunctionLibrary::AngleBetweenVectors(const FVector& A, const FVector& B)
{
	return FMath::RadiansToDegrees(FMath::Acos(A.GetSafeNormal() | B.GetSafeNormal()));
}

FORCEINLINE_DEBUGGABLE const float UEmpathFunctionLibrary::AngleBetweenVectors2D(const FVector& A, const FVector& B)
{
	return FMath::RadiansToDegrees(FMath::Acos(A.GetSafeNormal2D() | B.GetSafeNormal2D()));
}

FORCEINLINE_DEBUGGABLE const float UEmpathFunctionLibrary::AngleBetweenVectorsZ(const FVector& A, const FVector& B)
{
	return FMath::RadiansToDegrees(FMath::Acos(FlattenVectorXY(A).GetSafeNormal() | FlattenVectorXY(B).GetSafeNormal()));
}

FORCEINLINE_DEBUGGABLE void UEmpathFunctionLibrary::AngleAndAxisBetweenVectors(const FVector& A, const FVector& B, float& Angle, FVector& Axis)
{
	FVector ANormalized = A.GetSafeNormal();
	FVector BNormalized = B.GetSafeNormal();
	Angle = FMath::RadiansToDegrees(FMath::Acos(ANormalized | BNormalized));
	Axis = FVector::CrossProduct(ANormalized, BNormalized);
	return;
}