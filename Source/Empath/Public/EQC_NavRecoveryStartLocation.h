// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EQC_NavRecoveryStartLocation.generated.h"

/**
 * 
 */
UCLASS()
class EMPATH_API UEQC_NavRecoveryStartLocation : public UEnvQueryContext
{
	GENERATED_BODY()
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
	
	
	
};
