// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EmpathGenericEventInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEmpathGenericEventInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class EMPATH_API IEmpathGenericEventInterface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = EmpathGenericEventInterface)
	void ReceiveGenericEvent(const FName EventName);
};
