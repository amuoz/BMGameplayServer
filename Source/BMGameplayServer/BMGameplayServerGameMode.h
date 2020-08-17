// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BMGameplayServerGameMode.generated.h"

UCLASS(minimalapi)
class ABMGameplayServerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABMGameplayServerGameMode();

	void Respawn(class ABMGameplayServerCharacter* Character);

};



