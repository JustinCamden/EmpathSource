// Copyright 2018 Team Empath All Rights Reserved

#include "EQC_NavRecoveryStartLocation.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "EmpathCharacter.h"

void UEQC_NavRecoveryStartLocation::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	FVector TargetLocation(0.f);

	APawn* const QueryOwner = Cast<APawn>(QueryInstance.Owner.Get());
	if (QueryOwner)
	{
		TargetLocation = QueryOwner->GetActorLocation(); // Fallback to self location

		AEmpathCharacter const* const EmpathChar = Cast<AEmpathCharacter>(QueryOwner);
		if (EmpathChar)
		{
			if (EmpathChar->IsFailingNavigation())
			{
				TargetLocation = EmpathChar->NavRecoveryStartPathingLocation;
			}
		}
	}

	UEnvQueryItemType_Point::SetContextHelper(ContextData, TargetLocation);
}
