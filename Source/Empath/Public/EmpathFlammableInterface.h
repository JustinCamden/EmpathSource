// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EmpathTypes.h"
#include "EmpathFlammableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEmpathFlammableInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class EMPATH_API IEmpathFlammableInterface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = EmpathFlammableInterface)
	EEmpathFlammableType GetFlammableType();
};
