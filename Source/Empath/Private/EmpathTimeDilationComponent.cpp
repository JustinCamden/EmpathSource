// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathTimeDilationComponent.h"
#include "EmpathTimeDilator.h"
#include "EmpathFunctionLibrary.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UEmpathTimeDilationComponent::UEmpathTimeDilationComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	ActorTimeDilation = 1.0f;
}

void UEmpathTimeDilationComponent::SetActorTimeDilation(const float NewTimeDilation /*= 1.0f*/)
{
	if (ActorTimeDilation != NewTimeDilation)
	{
		ActorTimeDilation = FMath::Clamp(NewTimeDilation, 0.000100f, 20.000000f);
		if (bIsActive)
		{
			if (AActor* Owner = GetOwner())
			{
				Owner->CustomTimeDilation = ActorTimeDilation / GetWorld()->GetWorldSettings()->TimeDilation;
			}
		}
	}
	return;
}

AEmpathTimeDilator* UEmpathTimeDilationComponent::GetTimeDilator()
{
	if (TimeDilator)
	{
		return TimeDilator;
	}
	else
	{
		TimeDilator = UEmpathFunctionLibrary::GetTimeDilator(this);
		if (TimeDilator)
		{
			TimeDilator->OnTimeDilationChanged.AddDynamic(this, &UEmpathTimeDilationComponent::OnGlobalTimeDilationChanged);
		}
		return TimeDilator;
	}
}

void UEmpathTimeDilationComponent::OnGlobalTimeDilationChanged(float NewTimeDilation)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->CustomTimeDilation = ActorTimeDilation / NewTimeDilation;
	}
	return;
}

void UEmpathTimeDilationComponent::Activate(bool bReset /*= false*/)
{
	Super::Activate(bReset);

	if (bReset)
	{
		ActorTimeDilation = 1.0f;
	}

	if (!TimeDilator)
	{
		TimeDilator = UEmpathFunctionLibrary::GetTimeDilator(this);
		if (TimeDilator)
		{
			TimeDilator->OnTimeDilationChanged.AddDynamic(this, &UEmpathTimeDilationComponent::OnGlobalTimeDilationChanged);
		}
	}

	float WorldTimeDilation = GetWorld()->GetWorldSettings()->TimeDilation;

	if (WorldTimeDilation != ActorTimeDilation)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->CustomTimeDilation = ActorTimeDilation / GetWorld()->GetWorldSettings()->TimeDilation;
		}
	}

	return;
}

void UEmpathTimeDilationComponent::Deactivate()
{
	Super::Deactivate();
	if (AActor* Owner = GetOwner())
	{
		Owner->CustomTimeDilation = 1.0f;
	}

	return;
}

void UEmpathTimeDilationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->CustomTimeDilation = 1.0f;
	}
	if (TimeDilator)
	{
		TimeDilator->OnTimeDilationChanged.RemoveDynamic(this, &UEmpathTimeDilationComponent::OnGlobalTimeDilationChanged);
	}
	Super::EndPlay(EndPlayReason);
}
