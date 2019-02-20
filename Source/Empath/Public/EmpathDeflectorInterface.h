// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "EmpathDeflectorInterface.generated.h"



// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEmpathDeflectorInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class EMPATH_API IEmpathDeflectorInterface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathDeflectorInterface)
	bool IsDeflecting(const UPrimitiveComponent* DeflectingComponent) const;

};
