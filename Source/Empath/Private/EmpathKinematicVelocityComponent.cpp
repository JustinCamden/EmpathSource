// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathKinematicVelocityComponent.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "EmpathPlayerCharacter.h"
#include "EmpathFunctionLibrary.h"
#include "EmpathTimeDilator.h"


// Sets default values for this component's properties
UEmpathKinematicVelocityComponent::UEmpathKinematicVelocityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// We don't necessarily want to be active at the start of the game. Most likely we will activate on a delay.
	bAutoActivate = false;

	SampleTime = 0.1f;

}


// Called every frame
void UEmpathKinematicVelocityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CalculateKinematicVelocity();
}

void UEmpathKinematicVelocityComponent::Activate(bool bReset)
{
	Super::Activate(bReset);

	// Initialize last position and rotation to this position and rotation,
	// so that our calculations don't think we suddenly jumped places.
	if (bIsActive)
	{
		LastLocation = GetComponentLocation();
		if (OwningPlayer)
		{
			LastSphericalLocation = (LastLocation - OwningPlayer->GetCenterMassLocation());
			LastSphericalDist = LastSphericalLocation.Size();
			LastVerticalDist = LastSphericalLocation.Z;
			LastRadialDist = LastSphericalLocation.Size2D();
			LastLocation -= OwningPlayer->GetVRLocation();
		}
		LastRotation = GetComponentQuat();
	}
}

void UEmpathKinematicVelocityComponent::Deactivate()
{
	Super::Deactivate();

	// Clear out old variables
	if (!bIsActive)
	{
		KinematicVelocity = FVector::ZeroVector;
		LastVelocity = FVector::ZeroVector;
		FrameVelocity = FVector::ZeroVector;
		LastFrameVelocity = FVector::ZeroVector;
		KinematicAngularVelocity = FVector::ZeroVector;
		LastAngularVelocity = FVector::ZeroVector;
		FrameAngularVelocity = FVector::ZeroVector;
		LastFrameAngularVelocity = FVector::ZeroVector;
		FrameAngVelFixedLocal = FVector::ZeroVector;
		LastFrameAngVelFixedLocal = FVector::ZeroVector;
		FrameAngVelFixedWorld = FVector::ZeroVector;
		LastFrameAngVelFixedWorld = FVector::ZeroVector;
		LastRotation = FQuat::Identity;
		LastLocation = FVector::ZeroVector;
		LastSphericalLocation = FVector::ZeroVector;
		LastSphericalDist = 0.0f;
		LastRadialDist = 0.0f;
		LastVerticalDist = 0.0f;
		DeltaLocation = FVector::ZeroVector;
		DeltaRotation = FQuat::Identity;
		DeltaSphericalDist = 0.0f;
		DeltaRadialDist = 0.0f;
		DeltaVerticalDist = 0.0f;
		SphericalVelocity = 0.0f;
		LastSphericalVelocity = 0.0f;
		FrameSphericalVelocity = 0.0f;
		LastFrameSphericalVelocity = 0.0f;
		RadialVelocity = 0.0f;
		LastRadialVelocity = 0.0f;
		FrameRadialVelocity = 0.0f;
		LastFrameRadialVelocity = 0.0f;
		VerticalVelocity = 0.0f;
		LastRadialDist = 0.0f;
		FrameVerticalVelocity = 0.0f;
		LastFrameVerticalVelocity = 0.0f;
		KinematicAcceleration = FVector::ZeroVector;
		LastAcceleration = FVector::ZeroVector;
		FrameAcceleration = FVector::ZeroVector;
		LastFrameAcceleration = FVector::ZeroVector;
		AngularAccelWorld = FVector::ZeroVector;
		LastAngularAccelWorld = FVector::ZeroVector;
		FrameAngularAccelWorld = FVector::ZeroVector;
		LastFrameAngularAccelWorld = FVector::ZeroVector;
		AngularAccelLocal = FVector::ZeroVector;
		LastAngularAccelLocal = FVector::ZeroVector;
		FrameAngularAccelLocal = FVector::ZeroVector;
		LastFrameAngularAccelLocal = FVector::ZeroVector;
		SphericalAccel = 0.0f;
		LastSphericalAccel = 0.0f;
		FrameSphericalAccel = 0.0f;
		LastFrameSphericalAccel = 0.0f;
		RadialAccel = 0.0f;
		LastRadialAccel = 0.0f;
		FrameRadialAccel = 0.0f;
		LastFrameRadialAccel = 0.0f;
		VerticalAccel = 0.0f;
		LastVerticalAccel = 0.0f;
		FrameVerticalAccel = 0.0f;
		LastFrameVerticalAccel = 0.0f;
		VelocityHistory.Empty();
	}
}

const AEmpathTimeDilator* UEmpathKinematicVelocityComponent::GetTimeDilator()
{
	if (TimeDilator)
	{
		return TimeDilator;
	}

	TimeDilator = UEmpathFunctionLibrary::GetTimeDilator(this);
	return TimeDilator;
}

void UEmpathKinematicVelocityComponent::CalculateKinematicVelocity()
{
	// Archive old kinematic velocity
	LastVelocity = KinematicVelocity;
	LastFrameVelocity = FrameVelocity;
	LastAngularVelocity = KinematicAngularVelocity;

	// Archive last ang velocities
	LastFrameAngularVelocity = FrameAngularVelocity;
	LastAngVelFixedWorld = AngVelFixedWorld;
	LastAngVelFixedLocal = AngVelFixedLocal;
	LastFrameAngVelFixedWorld = FrameAngVelFixedWorld;
	LastFrameAngVelFixedLocal = FrameAngVelFixedLocal;

	// Archive old accelerations
	LastAcceleration = KinematicAcceleration;
	LastFrameAcceleration = FrameAcceleration;
	
	// Archive old angular accels
	LastAngularAccelWorld = AngularAccelWorld;
	LastAngularAccelLocal = AngularAccelLocal;
	LastFrameAngularAccelWorld = FrameAngularAccelWorld;
	LastFrameAngularAccelLocal = FrameAngularAccelLocal;

	// Update last radial and spherical velocities
	if (OwningPlayer)
	{
		LastSphericalVelocity = SphericalVelocity;
		LastFrameSphericalVelocity = FrameSphericalVelocity;
		LastRadialVelocity = RadialVelocity;
		LastFrameRadialVelocity = FrameRadialVelocity;
		LastVerticalVelocity = VerticalVelocity;
		LastFrameVerticalVelocity = FrameVerticalVelocity;
		LastSphericalAccel = SphericalAccel;
		LastFrameSphericalAccel = FrameSphericalAccel;
		LastRadialAccel = RadialAccel;
		LastFrameRadialAccel = FrameRadialAccel;
		LastVerticalAccel = VerticalAccel;
		LastFrameVerticalAccel = FrameVerticalAccel;
	}

	// If for some reason no seconds have passed, don't do anything
	const AEmpathTimeDilator* TimeDilatorRef = GetTimeDilator();
	float DeltaSeconds = (TimeDilatorRef ? TimeDilatorRef->GetUndilatedDeltaTime() : 0.0f);
	if (DeltaSeconds <= SMALL_NUMBER)
	{
		return;
	}

	// Get the velocity this frame by the change in location / change in time.
	FVector CurrentLocation = GetComponentLocation();
	
	// Fix the delta location and get the radial distance if we have an owning player
	float SphericalDist = 0.0f;
	float RadialDist = 0.0f;
	float VerticalDist = 0.0f;
	FVector SphericalLocation = FVector::ZeroVector;
	if (OwningPlayer) 
	{
		// Calculate spherical location
		SphericalLocation = (CurrentLocation - OwningPlayer->GetCenterMassLocation());

		// Get the spherical dist from the radial location
		SphericalDist = SphericalLocation.Size();

		// Using this and the last spherical distance, calculate the frame spherical velocity
		DeltaSphericalDist = SphericalDist - LastSphericalDist;
		FrameSphericalVelocity = (DeltaSphericalDist / DeltaSeconds);

		// Repeat for radial distance
		RadialDist = SphericalLocation.Size2D();
		DeltaRadialDist = RadialDist - LastRadialDist;
		FrameRadialVelocity = (DeltaRadialDist / DeltaSeconds);

		// Repeat for vertical distance
		VerticalDist = FMath::Abs(SphericalLocation.Z);
		DeltaVerticalDist = VerticalDist - LastVerticalDist;
		FrameVerticalVelocity = (DeltaVerticalDist / DeltaSeconds);

		// Calculate accelerations
		FrameSphericalAccel = (FrameSphericalVelocity - LastFrameSphericalVelocity) / DeltaSeconds;
		FrameRadialAccel = (FrameRadialVelocity - LastFrameRadialVelocity) / DeltaSeconds;
		FrameVerticalAccel = (FrameVerticalVelocity - LastFrameVerticalVelocity) / DeltaSeconds;

		// Fix the delta location for normal velocity calculators
		CurrentLocation -= OwningPlayer->GetVRLocation();

		// Cache old values
		LastSphericalLocation = SphericalLocation;
		LastSphericalDist = SphericalDist;
		LastRadialDist = RadialDist;
		LastVerticalDist = VerticalDist;
	}

	// Calculate the velocity
	DeltaLocation = CurrentLocation - LastLocation;
	FrameVelocity = (DeltaLocation / DeltaSeconds);

	// Calculate the acceleration
	FrameAcceleration = (FrameVelocity - LastFrameVelocity) / DeltaSeconds;

	// Next get the current angular velocity by the delta rotation
	FQuat CurrentRotation = GetComponentQuat();
	CurrentRotation.Normalize();
	DeltaRotation = CurrentRotation.Inverse() * LastRotation;
	FVector Axis;
	float Angle;
	DeltaRotation.ToAxisAndAngle(Axis, Angle);

	// Convert to degrees since those will be used more often
	FrameAngularVelocity = FVector::RadiansToDegrees(CurrentRotation.RotateVector((Axis * Angle) / DeltaSeconds));

	// Fix angular velocity weirdness at particular angles.
	// At certain, rare angles close to the 'poles' of the object, the rotate vector function produces wildly inaccurate results.
	// To correct for this behavior, we assume that the angular velocity has continued along its current trajectory in such cases.
	if ((FMath::Abs(LastFrameAngularVelocity.X - FrameAngularVelocity.X) > 500.0f
		&& FMath::Abs(FrameAngularVelocity.X) > 500.0f)
		|| (FMath::Abs(LastFrameAngularVelocity.Y - FrameAngularVelocity.Y) > 500.0f
			&& FMath::Abs(FrameAngularVelocity.Y) > 500.0f)
		|| (FMath::Abs(LastFrameAngularVelocity.Z - FrameAngularVelocity.Z) > 500.0f
			&& FMath::Abs(FrameAngularVelocity.Z) > 500.0f))
	{
		// 'Unfix' the world acceleration from the last frame so we can apply it to the frame angular velocity
		FVector LastAngularAccel = LastAngularAccelWorld;
		LastAngularAccel.Z *= -1.0f;
		FrameAngularVelocity = LastFrameAngularVelocity + (DeltaSeconds * LastAngularAccel);
	}

	// Next get the velocity over the sample time if appropriate.
	if (SampleTime > 0.0f)
	{

		// Log the velocity and the timestamp
		UWorld* World = GetWorld();
		float RealTimeSecs = World->GetRealTimeSeconds();
		VelocityHistory.Add(FEmpathVelocityFrame(FrameVelocity, FrameAngularVelocity, FrameAcceleration, FrameSphericalVelocity, FrameRadialVelocity, FrameVerticalVelocity, FrameSphericalAccel, FrameRadialAccel, FrameVerticalAccel, RealTimeSecs));

		// Loop through and average the array to get our new kinematic velocity.
		// Clean up while we're at it. This should take at most one iteration through the array.
		int32 NumToRemove = 0;
		for (int32 Idx = 0; Idx < VelocityHistory.Num(); ++Idx)
		{
			FEmpathVelocityFrame& VF = VelocityHistory[Idx];
			if ((RealTimeSecs - VF.FrameTimeStamp) > SampleTime)
			{
				NumToRemove++;
			}
			else
			{
				break;
			}
		}

		// Remove expired frames
		if (NumToRemove > 0)
		{
			VelocityHistory.RemoveAt(0, NumToRemove);
		}


		// Remaining history array is now guaranteed to be inside the time threshold.
		// Add it all up and average by the remaining frames to get the current kinematic velocity.
		FVector TotalVelocity = FVector::ZeroVector;
		FVector TotalAngularVelocity = FVector::ZeroVector;
		FVector TotalAcceleration = FVector::ZeroVector;
		float TotalSphericalVelcity = 0.0f;
		float TotalRadialVelocity = 0.0f;
		float TotalVerticalVelocity = 0.0f;
		float TotalSphericalAccel = 0.0f;
		float TotalRadialAccel = 0.0f;
		float TotalVerticalAccel = 0.0f;
		for (FEmpathVelocityFrame& VF : VelocityHistory)
		{
			TotalVelocity += VF.Velocity;
			TotalAngularVelocity += VF.AngularVelocity;
			TotalAcceleration += VF.KAcceleration;
			TotalSphericalVelcity += VF.SphericalVelocity;
			TotalRadialVelocity += VF.RadialVelocity;
			TotalVerticalVelocity += VF.VerticalVelocity;
			TotalSphericalAccel += VF.SphericalAccel;
			TotalRadialAccel += VF.RadialAccel;
			TotalVerticalAccel += VF.VerticalAccel;
		}
		float CountDivisor = (float)VelocityHistory.Num();
		KinematicVelocity = TotalVelocity / CountDivisor;
		KinematicAngularVelocity = TotalAngularVelocity / CountDivisor;
		SphericalVelocity = TotalSphericalVelcity / CountDivisor;
		RadialVelocity = TotalRadialVelocity / CountDivisor;
		VerticalVelocity = TotalVerticalVelocity / CountDivisor;
		KinematicAcceleration = TotalAcceleration / CountDivisor;
		SphericalAccel = TotalSphericalAccel / CountDivisor;
		RadialAccel = TotalRadialAccel / CountDivisor;
		VerticalAccel = TotalVerticalAccel / CountDivisor;
	}

	// If our sample time is <= 0 we just use the frame velocity.
	else
	{
		KinematicVelocity = FrameVelocity;
		KinematicAngularVelocity = FrameAngularVelocity;
		SphericalVelocity = FrameSphericalVelocity;
		RadialVelocity = FrameRadialVelocity;
		VerticalVelocity = FrameVerticalVelocity;
		KinematicAcceleration = FrameAcceleration;
		SphericalAccel = FrameSphericalAccel;
		RadialAccel = FrameRadialAccel;
		VerticalAccel = FrameVerticalAccel;
	}

	// Fix angular velocities to account for Z inversion in UE4
	// Curr ang velocity
	AngVelFixedWorld = KinematicAngularVelocity;
	AngVelFixedWorld.Z *= -1.0f;
	AngularAccelWorld = (AngVelFixedWorld - LastAngVelFixedWorld) / DeltaSeconds;
	AngVelFixedLocal = GetComponentTransform().InverseTransformVectorNoScale(KinematicAngularVelocity);
	AngVelFixedLocal.Z *= -1.0f;
	AngularAccelLocal = (AngVelFixedLocal - LastAngVelFixedLocal) / DeltaSeconds;

	// Curr frame ang velocity
	FrameAngVelFixedWorld = FrameAngularVelocity;
	FrameAngVelFixedWorld.Z *= -1.0f;
	FrameAngularAccelWorld = (FrameAngVelFixedWorld - LastFrameAngVelFixedWorld) / DeltaSeconds;
	FrameAngVelFixedLocal = GetComponentTransform().InverseTransformVectorNoScale(FrameAngularVelocity);
	FrameAngVelFixedLocal.Z *= -1.0f;
	FrameAngularAccelLocal = (FrameAngVelFixedLocal - LastFrameAngVelFixedLocal) / DeltaSeconds;

	// Log our current location for the next time we check our velocity.
	LastLocation = CurrentLocation;
	LastRotation = CurrentRotation;
}
