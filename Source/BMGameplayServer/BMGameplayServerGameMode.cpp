// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BMGameplayServerGameMode.h"
#include "BMGameplayServerHUD.h"
#include "BMGameplayServerCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "BMHealthComponent.h"
#include "NavigationSystem.h"

ABMGameplayServerGameMode::ABMGameplayServerGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ABMGameplayServerHUD::StaticClass();
}

void ABMGameplayServerGameMode::Respawn(ABMGameplayServerCharacter* Character)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		AController* thisPC = Character->GetController();
		Character->DetachFromControllerPendingDestroy();
		FTransform transform = Character->GetTransform();
		
		UNavigationSystemV1* navSys = UNavigationSystemV1::GetCurrent(GetWorld());
		FNavLocation navLocation;
		navSys->GetRandomPoint(navLocation);
		transform.SetLocation(navLocation.Location);
		
		ABMGameplayServerCharacter* newChar = Cast<ABMGameplayServerCharacter>(GetWorld()->SpawnActor(DefaultPawnClass, &transform));
		if (newChar)
		{
			thisPC->Possess(newChar);
			newChar->HealthComp->RestoreHealth();
		}
	}
}
