// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EmpathIgnoreCollisionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEmpathIgnoreCollisionInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class EMPATH_API IEmpathIgnoreCollisionInterface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** Returns whether an actor should ignore collision with this component. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathIgnoreCollisionInterface)
	bool ShouldOtherActorIgnoreCollision(const USceneComponent* Component) const;
	
};
