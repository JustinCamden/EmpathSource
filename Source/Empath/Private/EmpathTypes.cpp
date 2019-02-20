// // Copyright 2018 Team Empath All Rights Reserved

#include "EmpathTypes.h"
#include "EmpathHandActor.h"
#include "EmpathKinematicVelocityComponent.h"

const FName FEmpathBBKeys::AttackTarget(TEXT("AttackTarget"));
const FName FEmpathBBKeys::bCanSeeTarget(TEXT("bCanSeeTarget"));
const FName FEmpathBBKeys::bAlert(TEXT("bAlert"));
const FName FEmpathBBKeys::GoalLocation(TEXT("GoalLocation"));
const FName FEmpathBBKeys::BehaviorMode(TEXT("BehaviorMode"));
const FName FEmpathBBKeys::bIsPassive(TEXT("bIsPassive"));
const FName FEmpathBBKeys::DefendTarget(TEXT("DefendTarget"));
const FName FEmpathBBKeys::DefendGuardRadius(TEXT("DefendGuardRadius"));
const FName FEmpathBBKeys::DefendPursuitRadius(TEXT("DefendPursuitRadius"));
const FName FEmpathBBKeys::FleeTarget(TEXT("FleeTarget"));
const FName FEmpathBBKeys::FleeTargetRadius(TEXT("FleeTargetRadius"));
const FName FEmpathBBKeys::NavRecoveryDestination(TEXT("NavRecoveryDestination"));
const FName FEmpathBBKeys::NavRecoverySearchInnerRadius(TEXT("NavRecoverySearchInnerRadius"));
const FName FEmpathBBKeys::NavRecoverySearchOuterRadius(TEXT("NavRecoverySearchOuterRadius"));

const FName FEmpathCollisionProfiles::Ragdoll(TEXT("Ragdoll"));
const FName FEmpathCollisionProfiles::PawnIgnoreAll(TEXT("PawnIgnoreAll"));
const FName FEmpathCollisionProfiles::DamageCollision(TEXT("DamageCollision"));
const FName FEmpathCollisionProfiles::HandCollision(TEXT("HandCollision"));
const FName FEmpathCollisionProfiles::NoCollision(TEXT("NoCollision"));
const FName FEmpathCollisionProfiles::GripCollision(TEXT("GripCollision"));
const FName FEmpathCollisionProfiles::OverlapAllDynamic(TEXT("OverlapAllDynamic"));
const FName FEmpathCollisionProfiles::PawnOverlapOnlyTrigger(TEXT("PawnOverlapOnlyTrigger"));
const FName FEmpathCollisionProfiles::PlayerRoot(TEXT("PlayerRoot"));
const FName FEmpathCollisionProfiles::HandCollisionClimb(TEXT("HandCollisionClimb"));
const FName FEmpathCollisionProfiles::Projectile(TEXT("Projectile"));
const FName FEmpathCollisionProfiles::ProjectileSensor(TEXT("ProjectileSensor"));
const FName FEmpathCollisionProfiles::EmpathTrigger(TEXT("EmpathTrigger"));


bool FSecondaryAttackTarget::IsValid() const
{
	return (TargetActor != nullptr) && !TargetActor->IsPendingKill();
}

bool FEmpathPlayerAttackTarget::IsValid() const
{
	return (TargetActor != nullptr) && !TargetActor->IsPendingKill();
}


const bool FEmpathOneHandGestureData::AreEntryConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck)
{
	return (EntryConditions.AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, LastEntryFailureReason));
}


const bool FEmpathOneHandGestureData::AreSustainConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck)
{
	return (SustainConditions.AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, LastSustainFailureReason));
}

void FEmpathOneHandGestureData::InvertHand()
{
	EntryConditions.InvertHand();
	EntryConditions.InvertSubConditions();
	SustainConditions.InvertHand();
	SustainConditions.InvertSubConditions();

	return;
}

const bool FEmpathGestureCondition::EvaluateBinary(float Value, FString& OutFailureMessage)
{

	switch (CheckOperationType)
	{
	case EEmpathBinaryOperation::GreaterThan:
	{
		if (Value > ThresholdValue)
		{
			return true;
		}
		else
		{
			OutFailureMessage = "Too Low";
			return false;
		}
		break;
	}
	case EEmpathBinaryOperation::GreaterThanEqual:
	{
		if (Value >= ThresholdValue)
		{
			return true;
		}
		else
		{
			OutFailureMessage = "Too Low";
			return false;
		}
		break;
	}
	case EEmpathBinaryOperation::LessThan:
	{
		if (Value < ThresholdValue)
		{
			return true;
		}
		else
		{
			OutFailureMessage = "Too High";
			return false;
		}
		break;
	}
	case EEmpathBinaryOperation::LessThanEqual:
	{
		if (Value <= ThresholdValue)
		{
			return true;
		}
		else
		{
			OutFailureMessage = "Too High";
			return false;
		}
		break;
	}
	case EEmpathBinaryOperation::Equal:
	{
		if (FMath::IsNearlyEqual(Value, ThresholdValue))
		{
			return true;
		}
		else
		{
			OutFailureMessage = "Not Equal";
			return false;
		}
		break;
	}
	default:
	{
		OutFailureMessage = "Invalid Case";
		return false;
		break;
	}
	}
	return false;
}

const bool FEmpathGestureCondition::AreBaseConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailureMessage)
{
	switch (ConditionCheckType)
	{
	case EEmpathGestureConditionCheckType::AutoSuccess:
	{
		return true;
		break;
	}
	case EEmpathGestureConditionCheckType::VelocityMagnitude:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.VelocityMagnitude : FrameConditionCheck.VelocityMagnitude);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Velocity Magnitude";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::VelocityX:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.CheckVelocity.X : FrameConditionCheck.CheckVelocity.X);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Velocity X";
			return false;
		}
		break;
	}

	case EEmpathGestureConditionCheckType::VelocityY:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.CheckVelocity.Y : FrameConditionCheck.CheckVelocity.Y);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Velocity Y";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::VelocityZ:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.CheckVelocity.Z : FrameConditionCheck.CheckVelocity.Y);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Velocity Z";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AngularVelocityX:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.AngularVelocity.X : FrameConditionCheck.AngularVelocity.X);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Angular Velocity X";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AngularVelocityY:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.AngularVelocity.Y : FrameConditionCheck.AngularVelocity.Y);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Angular Velocity Y";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AngularVelocityZ:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.AngularVelocity.Z : FrameConditionCheck.AngularVelocity.Z);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Angular Velocity Z";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::ScaledAngularVelocityX:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.ScaledAngularVelocity.X : FrameConditionCheck.ScaledAngularVelocity.X);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Scaled Angular Velocity X";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::ScaledAngularVelocityY:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.ScaledAngularVelocity.Y : FrameConditionCheck.ScaledAngularVelocity.Y);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Scaled Angular Velocity Y";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::ScaledAngularVelocityZ:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.ScaledAngularVelocity.Z : FrameConditionCheck.ScaledAngularVelocity.Z);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Scaled Angular Velocity Z";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::SphericalVelocity:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.SphericalVelocity : FrameConditionCheck.SphericalVelocity);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Spherical Velocity";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::RadialVelocity:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.RadialVelocity : FrameConditionCheck.RadialVelocity);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Radial Velocity";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::VerticalVelocity:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.VerticalVelocity : FrameConditionCheck.VerticalVelocity);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Vertical Velocity";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AccelerationMagnitude:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.AccelMagnitude : FrameConditionCheck.AccelMagnitude);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Acceleration Magnitude";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AccelerationX:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.CheckAcceleration.X : FrameConditionCheck.CheckAcceleration.X);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Acceleration X";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AccelerationY:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.CheckAcceleration.Y : FrameConditionCheck.CheckAcceleration.Y);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Acceleration Y";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AccelerationZ:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.CheckAcceleration.Z : FrameConditionCheck.CheckAcceleration.Z);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Acceleration Z";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AngularAccelerationX:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.AngularAcceleration.X : FrameConditionCheck.AngularAcceleration.X);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Angular Acceleration X";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AngularAccelerationY:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.AngularAcceleration.Y : FrameConditionCheck.AngularAcceleration.Y);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Angular Acceleration Y";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::AngularAccelerationZ:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.AngularAcceleration.Z : FrameConditionCheck.AngularAcceleration.Z);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Angular Acceleration Z";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::SphericalAcceleration:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.SphericalAccel : FrameConditionCheck.SphericalAccel);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Spherical Acceleration";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::RadialAcceleration:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.RadialAccel : FrameConditionCheck.RadialAccel);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Radial Acceleration";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::VerticalAcceleration:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.VerticalAccel : FrameConditionCheck.VerticalAccel);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Vertical Acceleration";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::MotionAngleX:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.MotionAngle.X : FrameConditionCheck.MotionAngle.X);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Motion Angle X";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::MotionAngleY:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.MotionAngle.Y : FrameConditionCheck.MotionAngle.Y);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Motion Angle Y";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::MotionAngleZ:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.MotionAngle.Z : FrameConditionCheck.MotionAngle.Z);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Motion Angle Z";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::SphericalDistance:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.SphericalDist : FrameConditionCheck.SphericalDist);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Spherical Distance";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::RadialDistance:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.RadialDist: FrameConditionCheck.RadialDist);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Radial Distance";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::VerticalDistance:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.VerticalDist : FrameConditionCheck.VerticalDist);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Vertical Distance";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::DistanceBetweenHands:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.DistBetweenHands : FrameConditionCheck.DistBetweenHands);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Distance Between Hands";
			return false;
		}
		break;
	}
	case EEmpathGestureConditionCheckType::InteriorAngleToOtherHand:
	{
		float CheckValue = (VelocityCheckType == EEmpathVelocityCheckType::BufferedAverage ? GestureConditionCheck.InteriorAngleToOtherHand : FrameConditionCheck.InteriorAngleToOtherHand);
		if (EvaluateBinary(CheckValue, OutFailureMessage))
		{
			return true;
		}
		else
		{
			OutFailureMessage += " Interior Angle to Other Hand";
			return false;
		}
		break;
	}
	default:
	{
		OutFailureMessage = "Invalid Case";
		return false;
		break;
	}
	}

	return false;
}


void FEmpathGestureCondition::InvertHand()
{
	switch (ConditionCheckType)
	{
	// Y Velocity
	case EEmpathGestureConditionCheckType::VelocityY:
	{

		switch (CheckOperationType)
		{
		// Max to min
		case EEmpathBinaryOperation::GreaterThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::GreaterThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThanEqual;
			return;
			break;
		}

		// Min to max
		case EEmpathBinaryOperation::LessThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::LessThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThanEqual;
			return;
			break;
		}

		// If equal, then simply invert
		case EEmpathBinaryOperation::Equal:
		{
			if (ThresholdValue != 0.0f)
			{
				ThresholdValue *= -1.0f;
			}
			return;
			break;
		}

		default:
		{
			return;
			break;
		}
		}
	}

	// Y Angles
	case EEmpathGestureConditionCheckType::MotionAngleY:
	{
		switch (CheckOperationType)
		{
		// Max to min
		case EEmpathBinaryOperation::GreaterThan:
		{
			ThresholdValue = 180.0f - ThresholdValue;
			CheckOperationType = EEmpathBinaryOperation::LessThan;
			return;
			break;
		}

		case EEmpathBinaryOperation::GreaterThanEqual:
		{
			ThresholdValue = 180.0f - ThresholdValue;
			CheckOperationType = EEmpathBinaryOperation::LessThanEqual;
			return;
			break;
		}

		// Min to max
		case EEmpathBinaryOperation::LessThan:
		{
			ThresholdValue = 180.0f - ThresholdValue;
			CheckOperationType = EEmpathBinaryOperation::GreaterThan;
			return;
			break;
		}

		case EEmpathBinaryOperation::LessThanEqual:
		{
			ThresholdValue = 180.0f - ThresholdValue;
			CheckOperationType = EEmpathBinaryOperation::GreaterThanEqual;
			return;
			break;
		}

		// If equal, only invert the angle
		case EEmpathBinaryOperation::Equal:
		{
			ThresholdValue = 180.0f - ThresholdValue;
			return;
			break;
		}
		default:
		{
			return;
			break;
		}
		}
	}

	// Angular velocity Z
	case EEmpathGestureConditionCheckType::AngularVelocityZ:
	{
		switch (CheckOperationType)
		{
		// Max to min
		case EEmpathBinaryOperation::GreaterThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::GreaterThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThanEqual;
			return;
			break;
		}

		// Min to max
		case EEmpathBinaryOperation::LessThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::LessThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThanEqual;
			return;
			break;
		}

		// Invert if equalize
		case EEmpathBinaryOperation::Equal:
		{
			ThresholdValue *= -1.0f;
			return;
			break;
		}
		default:
		{
			return;
			break;
		}
		}
	}

	// Scaled angular velocity Z
	case EEmpathGestureConditionCheckType::ScaledAngularVelocityZ:
	{
		switch (CheckOperationType)
		{
			// Max to min
		case EEmpathBinaryOperation::GreaterThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::GreaterThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThanEqual;
			return;
			break;
		}

		// Min to max
		case EEmpathBinaryOperation::LessThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::LessThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThanEqual;
			return;
			break;
		}

		// Invert if equalize
		case EEmpathBinaryOperation::Equal:
		{
			ThresholdValue *= -1.0f;
			return;
			break;
		}
		default:
		{
			return;
			break;
		}
		}
	}

	// Y Acceleration
	case EEmpathGestureConditionCheckType::AccelerationY:
	{

		switch (CheckOperationType)
		{
			// Max to min
		case EEmpathBinaryOperation::GreaterThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::GreaterThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThanEqual;
			return;
			break;
		}

		// Min to max
		case EEmpathBinaryOperation::LessThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::LessThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThanEqual;
			return;
			break;
		}

		// If equal, then simply invert
		case EEmpathBinaryOperation::Equal:
		{
			if (ThresholdValue != 0.0f)
			{
				ThresholdValue *= -1.0f;
			}
			return;
			break;
		}

		default:
		{
			return;
			break;
		}
		}
	}

	// Z angular acceleration
	case EEmpathGestureConditionCheckType::AngularAccelerationZ:
	{

		switch (CheckOperationType)
		{
			// Max to min
		case EEmpathBinaryOperation::GreaterThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::GreaterThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::LessThanEqual;
			return;
			break;
		}

		// Min to max
		case EEmpathBinaryOperation::LessThan:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThan;
			return;
			break;
		}
		case EEmpathBinaryOperation::LessThanEqual:
		{
			ThresholdValue *= -1.0f;
			CheckOperationType = EEmpathBinaryOperation::GreaterThanEqual;
			return;
			break;
		}

		// If equal, then simply invert
		case EEmpathBinaryOperation::Equal:
		{
			if (ThresholdValue != 0.0f)
			{
				ThresholdValue *= -1.0f;
			}
			return;
			break;
		}

		default:
		{
			return;
			break;
		}
		}
	}
	default:
	{
		return;
		break;
	}
	}

	return;
}

const bool FEmpathTwoHandGestureData::AreEntryConditionsMet(FEmpathGestureCheck& RightHandGestureCheck, FEmpathGestureCheck& RightHandFrameGestureCheck, FEmpathGestureCheck& LeftHandGestureCheck, FEmpathGestureCheck& LeftHandFrameGestureCheck)
{
	return (RightHandEntryConditions.AreAllConditionsMet(RightHandGestureCheck, RightHandFrameGestureCheck, LastRightHandEntryFailureReason)
		&& LeftHandEntryConditions.AreAllConditionsMet(LeftHandGestureCheck, LeftHandFrameGestureCheck, LastLeftHandEntryFailureReason));
}

const bool FEmpathTwoHandGestureData::AreSustainConditionsMet(FEmpathGestureCheck& RightHandGestureCheck, FEmpathGestureCheck& RightHandFrameGestureCheck, FEmpathGestureCheck& LeftHandGestureCheck, FEmpathGestureCheck& LeftHandFrameGestureCheck)
{
	return (RightHandSustainConditions.AreAllConditionsMet(RightHandGestureCheck, RightHandFrameGestureCheck, LastRightHandSustainFailureReason)
		&& LeftHandSustainConditions.AreAllConditionsMet(LeftHandGestureCheck, LeftHandFrameGestureCheck, LastLeftHandSustainFailureReason));
}

void FEmpathTwoHandGestureData::CalculateLeftHandConditions()
{
	LeftHandEntryConditions = RightHandEntryConditions;
	LeftHandEntryConditions.InvertHand();
	LeftHandEntryConditions.InvertSubConditions();
	LeftHandSustainConditions = RightHandSustainConditions;
	LeftHandSustainConditions.InvertHand();
	LeftHandSustainConditions.InvertSubConditions();
}

const bool FEmpathGestureCondition04::AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailureMessage)
{
	// Check base conditions
	if (AreBaseConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
	{
		// Check strong sub conditions
		for (int32 Idx = 0; Idx < StrongSubConditions.Num(); Idx++)
		{
			// If any strong sub condition fails, fail
			if (!StrongSubConditions[Idx].AreBaseConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
			{
				OutFailureMessage += " Strong Sub Condition ";
				OutFailureMessage += FString::FromInt(Idx);
				return false;
			}
		}

		// Check weak sub conditions
		if (WeakSubConditions.Num() > 0)
		{
			// Initialize our own failure message for better logging
			FString WeakFailureMessage = "";
			FString TotalFailures = "";

			// We need at last one weak sub condition to succeed for us to pass
			for (int32 Idx = 0; Idx < WeakSubConditions.Num(); Idx++)
			{
				if (WeakSubConditions[Idx].AreBaseConditionsMet(GestureConditionCheck, FrameConditionCheck, WeakFailureMessage))
				{
					return true;
				}
				else
				{
					TotalFailures += " ";
					TotalFailures += WeakFailureMessage;
				}
			}

			// If we found no successes, fail
			OutFailureMessage += TotalFailures;
			OutFailureMessage += " Weak Sub Conditions";
			return false;
		}

		return true;
	}
	return false;
}

void FEmpathGestureCondition04::InvertSubConditions()
{
	for (FEmpathGestureCondition& CurrCondition : StrongSubConditions)
	{
		CurrCondition.InvertHand();
	}
	for (FEmpathGestureCondition& CurrCondition : WeakSubConditions)
	{
		CurrCondition.InvertHand();
	}
	return;
}

const bool FEmpathGestureCondition03::AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailureMessage)
{
	if (AreBaseConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
	{
		// Check strong sub conditions
		for (int32 Idx = 0; Idx < StrongSubConditions.Num(); Idx++)
		{
			// If any strong sub condition fails, fail
			if (!StrongSubConditions[Idx].AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
			{
				OutFailureMessage += " Strong Sub Condition ";
				OutFailureMessage += FString::FromInt(Idx);
				return false;
			}
		}

		// Check weak sub conditions
		if (WeakSubConditions.Num() > 0)
		{
			// Initialize our own failure message for better logging
			FString WeakFailureMessage = "";
			FString TotalFailures = "";

			// We need at last one weak sub condition to succeed for us to pass
			for (int32 Idx = 0; Idx < WeakSubConditions.Num(); Idx++)
			{
				if (WeakSubConditions[Idx].AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, WeakFailureMessage))
				{
					return true;
				}
				else
				{
					TotalFailures += " ";
					TotalFailures += WeakFailureMessage;
				}
			}

			// If we found no successes, fail
			OutFailureMessage += TotalFailures;
			OutFailureMessage += " Weak Sub Conditions";
			return false;
		}

		return true;
	}
	return false;
}

void FEmpathGestureCondition03::InvertSubConditions()
{
	for (FEmpathGestureCondition04& CurrCondition : StrongSubConditions)
	{
		CurrCondition.InvertHand();
		CurrCondition.InvertSubConditions();
	}
	for (FEmpathGestureCondition04& CurrCondition : WeakSubConditions)
	{
		CurrCondition.InvertHand();
		CurrCondition.InvertSubConditions();
	}
	return;
}

const bool FEmpathGestureCondition02::AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailureMessage)
{
	if (AreBaseConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
	{
		// Check strong sub conditions
		for (int32 Idx = 0; Idx < StrongSubConditions.Num(); Idx++)
		{
			// If any strong sub condition fails, fail
			if (!StrongSubConditions[Idx].AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
			{
				OutFailureMessage += " Strong Sub Condition ";
				OutFailureMessage += FString::FromInt(Idx);
				return false;
			}
		}

		// Check weak sub conditions
		if (WeakSubConditions.Num() > 0)
		{
			// Initialize our own failure message for better logging
			FString WeakFailureMessage = "";
			FString TotalFailures = "";

			// We need at last one weak sub condition to succeed for us to pass
			for (int32 Idx = 0; Idx < WeakSubConditions.Num(); Idx++)
			{
				if (WeakSubConditions[Idx].AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, WeakFailureMessage))
				{
					return true;
				}
				else
				{
					TotalFailures += " ";
					TotalFailures += WeakFailureMessage;
				}
			}

			// If we found no successes, fail
			OutFailureMessage += TotalFailures;
			OutFailureMessage += " Weak Sub Conditions";
			return false;
		}

		return true;
	}
	return false;
}

void FEmpathGestureCondition02::InvertSubConditions()
{
	for (FEmpathGestureCondition03& CurrCondition : StrongSubConditions)
	{
		CurrCondition.InvertHand();
		CurrCondition.InvertSubConditions();
	}
	for (FEmpathGestureCondition03& CurrCondition : WeakSubConditions)
	{
		CurrCondition.InvertHand();
		CurrCondition.InvertSubConditions();
	}
	return;
}

const bool FEmpathGestureCondition01::AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailureMessage)
{
	if (AreBaseConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
	{
		// Check strong sub conditions
		for (int32 Idx = 0; Idx < StrongSubConditions.Num(); Idx++)
		{
			// If any strong sub condition fails, fail
			if (!StrongSubConditions[Idx].AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, OutFailureMessage))
			{
				OutFailureMessage += " Strong Sub Condition ";
				OutFailureMessage += FString::FromInt(Idx);
				return false;
			}
		}

		// Check weak sub conditions
		if (WeakSubConditions.Num() > 0)
		{
			// Initialize our own failure message for better logging
			FString WeakFailureMessage = "";
			FString TotalFailures = "";

			// We need at last one weak sub condition to succeed for us to pass
			for (int32 Idx = 0; Idx < WeakSubConditions.Num(); Idx++)
			{
				if (WeakSubConditions[Idx].AreAllConditionsMet(GestureConditionCheck, FrameConditionCheck, WeakFailureMessage))
				{
					return true;
				}
				else
				{
					TotalFailures += " ";
					TotalFailures += WeakFailureMessage;
				}
			}

			// If we found no successes, fail
			OutFailureMessage += TotalFailures;
			OutFailureMessage += " Weak Sub Conditions";
			return false;
		}

		OutFailureMessage = "";
		return true;
	}
	return false;
}

void FEmpathGestureCondition01::InvertSubConditions()
{
	for (FEmpathGestureCondition02& CurrCondition : StrongSubConditions)
	{
		CurrCondition.InvertHand();
		CurrCondition.InvertSubConditions();
	}
	for (FEmpathGestureCondition02& CurrCondition : WeakSubConditions)
	{
		CurrCondition.InvertHand();
		CurrCondition.InvertSubConditions();
	}
	return;
}

void FEmpathPoseData::CalculateLeftHandConditions()
{
	StaticData.CalculateLeftHandConditions();
	DynamicData.CalculateLeftHandConditions();

	return;
}
