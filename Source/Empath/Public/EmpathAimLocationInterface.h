// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EmpathAimLocationInterface.generated.h"

/** Interface used so characters can override their aim location or provide one of multiple aim locations depending on direction.*/
UINTERFACE(MinimalAPI)
class UEmpathAimLocationInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


/**
 * 
 */
class EMPATH_API IEmpathAimLocationInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/** Gets the aim location of the actor, along with the component to aim at if appropriate. Returns true if successful. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathAimLocationInterface)
	bool GetCustomAimLocationOnActor(FVector LookOrigin, FVector LookDirection, FVector& OutAimLocation, USceneComponent*& OutAimLocationComponent) const;
};
