// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BMGameplayServerHUD.generated.h"

UCLASS()
class ABMGameplayServerHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABMGameplayServerHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

