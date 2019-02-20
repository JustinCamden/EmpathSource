// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "EmpathTypes.h"
#include "EmpathKinematicVelocityComponent.generated.h"

class AEmpathPlayerCharacter; 
class AEmpathTimeDilator;

// This class exists to allow us to track the velocity of kinematic objects (like motion controllers),
// which would normally not have a velocity since they are not simulating physics

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EMPATH_API UEmpathKinematicVelocityComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UEmpathKinematicVelocityComponent();

	/** How much time, in seconds, that we sample and average to get our current kinematic velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathKinematicVelocityComponent)
		float SampleTime;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void Activate(bool bReset) override;
	virtual void Deactivate() override;

	/** Our location on the last frame, used to calculate our kinematic velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastLocation() const { return LastLocation; }

	/** Our rotation on the last frame, used to calculate our kinematic angular velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FRotator GetLastRotation() const { return LastRotation.Rotator(); }

	/** Our spherical location on the last frame, with respect to our owning player's center of mass. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastSphericalLocation() const { return LastSphericalLocation; }

	/** Our spherical dist the last frame, used to calculation our spherical velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastSphericalDist() const { return LastSphericalDist; }

	/** Our Radial dist the last frame, used to calculation our Radial velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastRadialDist() const { return LastRadialDist; }

	/** Our Vertical dist the last frame, used to calculation our Vertical velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastVerticalDist() const { return LastVerticalDist; }

	/** Our change in location since the last frame, used to calculation our kinematic velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetDeltaLocation() const { return DeltaLocation; }

	/** Our change in rotation since the last frame, used to calculation our kinematic angular velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FRotator GetDeltaRotation() const { return DeltaRotation.Rotator(); }

	/** Our change in spherical dist the last frame, used to calculation our spherical velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetDeltaSphericalDist() const { return DeltaSphericalDist; }

	/** Our change in radial dist the last frame, used to calculation our radial velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetDeltaRadialDist() const { return DeltaRadialDist; }

	/** Our change in vertical dist the last frame, used to calculation our Vertical velocity. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetDeltaVerticalDist() const { return DeltaVerticalDist; }

	/** The kinematic velocity of this component, averaged from all the recorded velocities within the same time. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetKinematicVelocity() const { return KinematicVelocity; }

	/** The kinematic velocity of this component on the last frame, averaged from all the recorded velocities within the same time. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastKinematicVelocity() const { return LastVelocity; }

	/** The kinematic velocity of this component, calculated with respect to this frame only. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetFrameVelocity() const { return FrameVelocity; }

	/** The last kinematic velocity of this component, calculated with respect to the last frame only. Expressed in world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastFrameVelocity() const { return LastFrameVelocity; }

	/** The kinematic angular velocity of this component, averaged from all the recorded velocities within the same time. Expressed in Radians and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetKinematicAngularVelocityRads() const { return FVector::DegreesToRadians(KinematicAngularVelocity); }

	/** The kinematic angular velocity of this component on the last frame, averaged from all the recorded velocities within the same time. Expressed in Radians and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastKinematicAngularVelocityRads() const { return FVector::DegreesToRadians(LastAngularVelocity); }

	/** The kinematic angular velocity of this component, calculated with respect to this frame only. Expressed in Radians and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetFrameAngularVelocityRads() const { return FVector::DegreesToRadians(FrameAngularVelocity); }

	/** The last kinematic angular velocity of this component, calculated with respect to the last frame only. Expressed in Radians and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastFrameAngularVelocityRads() const { return FVector::DegreesToRadians(LastFrameAngularVelocity); }

	/** The kinematic angular velocity of this component, averaged from all the recorded velocities within the same time. Expressed in degrees and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetKAngularVelocity() const { return AngVelFixedWorld; }

	/** The kinematic angular velocity of this component, averaged from all the recorded velocities within the same time. Expressed in degrees and local space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetKAngularVelocityLocal() const { return AngVelFixedLocal; }

	/** The kinematic angular velocity of this component on the last frame, averaged from all the recorded velocities within the same time. Expressed in degrees and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastKAngularVelocity() const { return LastAngVelFixedWorld; }

	/** The kinematic angular velocity of this component on the last frame, averaged from all the recorded velocities within the same time. Expressed in degrees and local space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastKAngularVelocityLocal() const { return LastAngVelFixedLocal; }

	/** The kinematic angular velocity of this component, calculated with respect to this frame only. Expressed in degrees and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetFrameAngularVelocity() const { return FrameAngVelFixedWorld; }

	/** The kinematic angular velocity of this component, calculated with respect to this frame only. Expressed in degrees and local space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetFrameAngularVelocityLocal() const { return FrameAngVelFixedLocal; }

	/** The last kinematic angular velocity of this component, calculated with respect to the last frame only. Expressed in degrees and world space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastFrameAngularVelocity() const { return LastFrameAngVelFixedWorld; }

	/** The last kinematic angular velocity of this component, calculated with respect to the last frame only. Expressed in degrees and local space. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const FVector GetLastFrameAngularVelocityLocal() const { return LastFrameAngVelFixedLocal; }

	/** The acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const FVector GetKinematicAcceleration() const { return KinematicAcceleration; }

	/** The world angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const FVector GetAngularAccelWorld() const { return AngularAccelWorld; }

	/** The local angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const FVector GetAngularAccelLocal() const { return AngularAccelLocal; }

	/** The last acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const FVector GetLastAcceleration() const { return LastAcceleration; }

	/** The last world angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const FVector GetLastAngularAccelWorld() const { return LastAngularAccelWorld; }

	/** The last local angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const FVector GetLastAngularAccelLocal() const { return LastAngularAccelLocal; }

	/** The acceleration of this component, calculated with respect to this frame only. */
	const FVector GetFrameAcceleration() const { return FrameAcceleration; }

	/** The world angular acceleration of this component, calculated with respect to this frame only. */
	const FVector GetFrameAngularAccelWorld() const { return FrameAngularAccelWorld; }

	/** The last local angular acceleration of this component, calculated with respect to this frame only. */
	const FVector GetFrameAngularAccelLocal() const { return FrameAngularAccelLocal; }

	/** The last acceleration of this component, calculated with respect to the last frame only. */
	const FVector GetLastFrameAcceleration() const { return LastFrameAcceleration; }

	/** The last world angular acceleration of this component, calculated with respect to the last frame only. */
	const FVector GetLastFrameAngularAccelWorld() const { return LastFrameAngularAccelWorld; }

	/** The last last local angular acceleration of this component, calculated with respect to the last frame only. */
	const FVector GetLastFrameAngularAccelLocal() const { return LastFrameAngularAccelLocal; }

	/*
	* The spherical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetSphericalVelocity() const { return SphericalVelocity; }


	/*
	* The last spherical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetLastSphericalVelocity() const { return LastSphericalVelocity; }

	/*
	* The spherical velocity of this component, calculated with respect to this frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetFrameSphericalVelocity() const { return FrameSphericalVelocity; }

	/*
	* The spherical velocity magnitude of this component, calculated with respect to last frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetLastFrameSphericalVelocity() const { return LastFrameSphericalVelocity; }

	/*
	* The Radial velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetRadialVelocity() const { return RadialVelocity; }


	/*
	* The last Radial velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetLastRadialVelocity() const { return LastRadialVelocity; }

	/*
	* The Radial velocity of this component, calculated with respect to this frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetFrameRadialVelocity() const { return FrameRadialVelocity; }

	/*
	* The Radial velocity magnitude of this component, calculated with respect to last frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetLastFrameRadialVelocity() const { return LastFrameRadialVelocity; }

	/*
	* The Vertical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetVerticalVelocity() const { return VerticalVelocity; }


	/*
	* The last Vertical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastVerticalVelocity() const { return LastVerticalVelocity; }

	/*
	* The Vertical velocity of this component, calculated with respect to this frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetFrameVerticalVelocity() const { return FrameVerticalVelocity; }

	/*
	* The Vertical velocity magnitude of this component, calculated with respect to last frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastFrameVerticalVelocity() const { return LastFrameVerticalVelocity; }

	/** The spherical acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetSphericalAccel() const { return SphericalAccel; }

	/** The radial acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetRadialAccel() const { return RadialAccel; }

	/** The Vertical acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetVerticalAccel() const { return VerticalAccel; }

	/** The last spherical acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
	const float GetLastSphericalAccel() const { return LastSphericalAccel; }

	/** The last radial acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastRadialAccel() const { return LastRadialAccel; }

	/** The last Vertical acceleration of this component, averaged from all the recorded velocities within the same time. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastVerticalAccel() const { return LastVerticalAccel; }

	/** The spherical acceleration of this component, calculated with respect to this frame only. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetFrameSphericalAccel() const { return FrameSphericalAccel; }

	/** The radial acceleration of this component, calculated with respect to this frame only. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetFrameRadialAccel() const { return FrameRadialAccel; }

	/** The Vertical acceleration of this component, calculated with respect to this frame only. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetFrameVerticalAccel() const { return FrameVerticalAccel; }

	/** The last spherical acceleration of this component, calculated with respect to the last frame only. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastFrameSphericalAccel() const { return LastFrameSphericalAccel; }

	/** The last radial acceleration of this component, calculated with respect to the last frame only. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastFrameRadialAccel() const { return LastFrameRadialAccel; }

	/** The last vertical acceleration of this component, calculated with respect to the last frame only. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathKinematicVelocityComponent)
		const float GetLastFrameVerticalAccel() const { return LastFrameVerticalAccel; }

	
	/** The player that owns this kinematic velocity component. */
	UPROPERTY(BlueprintReadWrite, Category = EmpathKinematicVelocityComponent)
	AEmpathPlayerCharacter* OwningPlayer; 

	/** Safe getter for the time dilator. */
	const AEmpathTimeDilator* GetTimeDilator();

private:
	/** Our location on the last frame, used to calculation our kinematic velocity. */
	FVector LastLocation;

	/** Our rotation on the last frame, used to calculation our kinematic angular velocity. */
	FQuat LastRotation;

	/** Our spherical distance on the last frame, used to calculation our spherical velocity. */
	float LastSphericalDist;;

	/** Our radial distance on the last frame, used to calculation our radial velocity. */
	float LastRadialDist;

	/** Our vertical distance on the last frame, used to calculation our vertical velocity. */
	float LastVerticalDist;

	/** Our spherical location on the last frame, used to calculation our spherical velocity. */
	FVector LastSphericalLocation;

	/** Our change in location since the last frame, used to calculation our kinematic velocity. */
	FVector DeltaLocation;

	/** Our change in rotation since the last frame, used to calculation our kinematic angular velocity. */
	FQuat DeltaRotation;

	/** Our change in spherical distance on the last frame, used to calculation our spherical velocity. */
	float DeltaSphericalDist;

	/** Our change in radial distance on the last frame, used to calculation our radial velocity. */
	float DeltaRadialDist;

	/** Our change in vertical distance on the last frame, used to calculation our vertical velocity. */
	float DeltaVerticalDist;

	/** Per-frame record of kinematic velocity. */
	TArray<FEmpathVelocityFrame> VelocityHistory;

	/** The kinematic velocity of this component, averaged from all the recorded velocities within the same time. */
	FVector KinematicVelocity;

	/** The kinematic velocity of this component on the last frame, averaged from all the recorded velocities within the same time. */
	FVector LastVelocity;

	/** The kinematic velocity of this component, calculated with respect to this frame only. */
	FVector FrameVelocity;

	/** The last kinematic velocity of this component, calculated with respect to last frame only. */
	FVector LastFrameVelocity;

	/** The kinematic angular velocity of this component, averaged from all the recorded velocities within the same time. Expressed in degrees. */
	FVector KinematicAngularVelocity;

	/*
	* The kinematic angular velocity of this component, averaged from all the recorded velocities within the same time. Expressed in degrees. 
	* Has the Z fixed for world space.
	*/
	FVector AngVelFixedWorld;

	/*
	* The kinematic angular velocity of this component, averaged from all the recorded velocities within the same time. Expressed in degrees.
	* Has the Z fixed for local space.
	*/
	FVector AngVelFixedLocal;

	/*
	* The spherical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float SphericalVelocity;

	/*
	* The radial velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float RadialVelocity;

	/*
	* The vertical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float VerticalVelocity;

	/** The acceleration of this component, averaged from all the recorded velocities within the same time. */
	FVector KinematicAcceleration;

	/** The world angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	FVector AngularAccelWorld;

	/** The local angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	FVector AngularAccelLocal;

	/** The spherical acceleration of this component, averaged from all the recorded velocities within the same time. */
	float SphericalAccel;

	/** The radial acceleration of this component, averaged from all the recorded velocities within the same time. */
	float RadialAccel;

	/** The Vertical acceleration of this component, averaged from all the recorded velocities within the same time. */
	float VerticalAccel;

	/** The kinematic angular velocity of this component on the last frame, averaged from all the recorded velocities within the same time. Expressed in degrees. */
	FVector LastAngularVelocity;

	/*
	* The kinematic angular velocity of this component on the last frame, averaged from all the recorded velocities within the same time. Expressed in degrees. 
	* Has the Z fixed for world space.
	*/
	FVector LastAngVelFixedWorld;

	/*
	* The kinematic angular velocity of this component on the last frame, averaged from all the recorded velocities within the same time. Expressed in degrees.
	* Has the Z fixed for local space.
	*/
	FVector LastAngVelFixedLocal;

	/*
	* The last spherical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float LastSphericalVelocity;

	/*
	* The last radial velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float LastRadialVelocity;

	/*
	* The last vertical velocity magnitude of this component, averaged from all the recorded velocities within the same time.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float LastVerticalVelocity;

	/** The last acceleration of this component, averaged from all the recorded velocities within the same time. */
	FVector LastAcceleration;

	/** The last world angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	FVector LastAngularAccelWorld;

	/** The last local angular acceleration of this component, averaged from all the recorded velocities within the same time. */
	FVector LastAngularAccelLocal;

	/** The last spherical acceleration of this component, averaged from all the recorded velocities within the same time. */
	float LastSphericalAccel;

	/** The last radial acceleration of this component, averaged from all the recorded velocities within the same time. */
	float LastRadialAccel;

	/** The last Vertical acceleration of this component, averaged from all the recorded velocities within the same time. */
	float LastVerticalAccel;

	/** The kinematic angular velocity of this component, calculated with respect to this frame only. Expressed in degrees. */
	FVector FrameAngularVelocity;

	/*
	* The kinematic angular velocity of this component, calculated with respect to this frame only. Expressed in degrees. 
	* Has the Z fixed for world space.
	*/
	FVector FrameAngVelFixedWorld;

	/*
	* The kinematic angular velocity of this component, calculated with respect to this frame only. Expressed in degrees.
	* Has the Z fixed for local space.
	*/
	FVector FrameAngVelFixedLocal;

	/*
	* The spherical velocity magnitude of this component, calculated with respect to this frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away. 
	*/
	float FrameSphericalVelocity;

	/*
	* The radial velocity magnitude of this component, calculated with respect to this frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float FrameRadialVelocity;

	/*
	* The radial velocity magnitude of this component, calculated with respect to this frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float FrameVerticalVelocity;

	/** The acceleration of this component, calculated with respect to this frame only. */
	FVector FrameAcceleration;

	/** The world angular acceleration of this component, calculated with respect to this frame only. */
	FVector FrameAngularAccelWorld;

	/** The last local angular acceleration of this component, calculated with respect to this frame only. */
	FVector FrameAngularAccelLocal;

	/** The spherical acceleration of this component, calculated with respect to this frame only. */
	float FrameSphericalAccel;

	/** The radial acceleration of this component, calculated with respect to this frame only. */
	float FrameRadialAccel;

	/** The Vertical acceleration of this component, calculated with respect to this frame only. */
	float FrameVerticalAccel;

	/** The last angular kinematic velocity of this component, calculated with respect to the last frame only. Expressed in degrees. */
	FVector LastFrameAngularVelocity;

	/*
	* The last angular kinematic velocity of this component, calculated with respect to the last frame only. Expressed in degrees. 
	* Has the Z fixed for world space
	*/
	FVector LastFrameAngVelFixedWorld;

	/*
	* The last angular kinematic velocity of this component, calculated with respect to the last frame only. Expressed in degrees.
	* Has the Z fixed for local space
	*/
	FVector LastFrameAngVelFixedLocal;

	/*
	* The spherical velocity magnitude of this component, calculated with respect to the last frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float LastFrameSphericalVelocity;

	/*
	* The Radial velocity magnitude of this component, calculated with respect to the last frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float LastFrameRadialVelocity;

	/*
	* The Radial velocity magnitude of this component, calculated with respect to the last frame only.
	* Negative if towards owning player's center of mass.
	* Positive if moving away.
	*/
	float LastFrameVerticalVelocity;

	/** The last acceleration of this component, calculated with respect to the last frame only. */
	FVector LastFrameAcceleration;

	/** The last world angular acceleration of this component, calculated with respect to the last frame only. */
	FVector LastFrameAngularAccelWorld;

	/** The last last local angular acceleration of this component, calculated with respect to the last frame only. */
	FVector LastFrameAngularAccelLocal;

	/** The last spherical acceleration of this component, calculated with respect to the last frame only. */
	float LastFrameSphericalAccel;

	/** The last radial acceleration of this component, calculated with respect to the last frame only. */
	float LastFrameRadialAccel;

	/** The last vertical acceleration of this component, calculated with respect to the last frame only. */
	float LastFrameVerticalAccel;

	/** Uses our last position to calculate the kinematic velocity for this frame. */
	void CalculateKinematicVelocity();

	/** Reference to the time dilator for optimization. */
	AEmpathTimeDilator* TimeDilator;
};
