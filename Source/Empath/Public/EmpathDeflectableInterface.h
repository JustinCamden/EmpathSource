// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "EmpathDeflectableInterface.generated.h"

class AController;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEmpathDeflectableInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class EMPATH_API IEmpathDeflectableInterface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathDeflectableInterface)
	bool IsDeflectable(const UPrimitiveComponent* DeflectedComponent) const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = EmpathDeflectableInterface)
	bool BeDeflected(const UPrimitiveComponent* DeflectedComponent, const FVector DeflectionImpulse, const AController* DeflectInstigator, const AActor* DeflectCauser);
};
