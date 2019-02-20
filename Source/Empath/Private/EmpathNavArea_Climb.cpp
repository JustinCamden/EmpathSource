// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathNavArea_Climb.h"
#include "EmpathTypes.h"

UEmpathNavArea_Climb::UEmpathNavArea_Climb(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DrawColor = FColor(255, 0, 128);
	AreaFlags |= EmpathNavAreaFlags::Climb;
}




