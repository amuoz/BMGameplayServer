// Fill out your copyright notice in the Description page of Project Settings.


#include "BMSphereAttackComponent.h"

#include "Net/UnrealNetwork.h"
#include "BMGameplayServerCharacter.h"
#include "DrawDebugHelpers.h"			// DrawDebugSphere
#include "Engine/Engine.h"				// GEngine
#include "Kismet/KismetSystemLibrary.h"	// Sphere overlap

// Sets default values for this component's properties
UBMSphereAttackComponent::UBMSphereAttackComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// initial config values
	InitialRadius = 100.0f;
	MaxRadius = 500.0f;
	SpeedRadius = 400.0f;
	Cooldown = 5.0f;
	DamageAmount = 50.0f;
	
	CurrentRadius = 0.0f;
	CurrentCooldown = 0.0f;

	Activated = false;
}


// Called when the game starts
void UBMSphereAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	CharacterOwner = (ABMGameplayServerCharacter*)GetOwner();
}


// Called every frame
void UBMSphereAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Modify radius and cooldown only on server side
	if (CharacterOwner->GetLocalRole() == ROLE_Authority)
	{
		if (Activated)
		{
			CurrentRadius += SpeedRadius * DeltaTime;
			CurrentRadius = FMath::Clamp(CurrentRadius, InitialRadius, MaxRadius);
		}

		if (IsInCooldown())
		{
			CurrentCooldown -= DeltaTime;
			CurrentCooldown = FMath::Clamp(CurrentCooldown, 0.0f, Cooldown);
		}
	}

	// Logic for remote clients
	if (CharacterOwner->GetLocalRole() < ROLE_Authority)
	{
		if (Activated)
		{
			// Draw sphere on both clients
			DrawDebugSphere(GetWorld(), CharacterOwner->GetActorLocation(), CurrentRadius, 24, FColor::Yellow, false, 0.01f, 0, 1.0f);

			// check for enemy overlap info in local client
			if (CharacterOwner->IsLocallyControlled())
			{
				int currentOverlapEnemies = CheckOverlapEnemies();
				if (NumEnemies != currentOverlapEnemies)
				{
					NumEnemies = currentOverlapEnemies;
					CharacterOwner->OnEnemyOverlapEvent();
				}
			}
		}

		// TO-DO more elegant
		if (!Activated && NumEnemies != 0)
		{
			NumEnemies = 0;
			CharacterOwner->OnEnemyOverlapEvent();
		}
	}

}

void UBMSphereAttackComponent::OnRep_CurrentRadius()
{
	// update locally controlled hud
	if (CharacterOwner->IsLocallyControlled())
	{
		CharacterOwner->OnSphereEvent();
	}
}

void UBMSphereAttackComponent::OnRep_CurrentCooldown()
{
	// update locally controlled hud
	if (CharacterOwner->IsLocallyControlled())
	{
		CharacterOwner->OnCooldownEvent();
	}
}

void UBMSphereAttackComponent::ActivateSphere()
{
	if (!IsInCooldown())
	{
		Activated = true;
	}
}

void UBMSphereAttackComponent::DeactivateSphere()
{
	if (Activated)
	{
		FireSpell();

		// Restore current radius
		CurrentRadius = InitialRadius;
		// Restore cooldown to max
		CurrentCooldown = Cooldown;
		// Deactivate sphere
		Activated = false;
	}
}

int UBMSphereAttackComponent::CheckOverlapEnemies()
{
	TArray<AActor*> outActors;
	TArray<AActor*> actorsToIgnore;
	actorsToIgnore.Add(CharacterOwner);
	TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
	traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	if (UKismetSystemLibrary::SphereOverlapActors(GetWorld(),
		CharacterOwner->GetActorLocation(), CurrentRadius, traceObjectTypes, NULL, actorsToIgnore, outActors))
	{
		return outActors.Num();
	}

	return 0;
}

void UBMSphereAttackComponent::FireSpell()
{
	if (CharacterOwner->GetLocalRole() == ROLE_Authority)
	{
		TArray<AActor*> outActors;
		TArray<AActor*> actorsToIgnore;
		actorsToIgnore.Add(CharacterOwner);
		TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
		traceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

		if (UKismetSystemLibrary::SphereOverlapActors(GetWorld(),
			CharacterOwner->GetActorLocation(), CurrentRadius, traceObjectTypes, NULL, actorsToIgnore, outActors))
		{
			for (AActor* actor : outActors)
			{
				FDamageEvent DamageEvent;
				actor->TakeDamage(DamageAmount, DamageEvent, CharacterOwner->GetController(), CharacterOwner);
			}
		}
	}
}

// Replicated properties
void UBMSphereAttackComponent::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBMSphereAttackComponent, CurrentRadius);
	DOREPLIFETIME(UBMSphereAttackComponent, CurrentCooldown);
	DOREPLIFETIME(UBMSphereAttackComponent, Activated);
}

void UBMSphereAttackComponent::ServerActivateSphere_Implementation()
{
	ActivateSphere();
}

void UBMSphereAttackComponent::ServerDeactivateSphere_Implementation()
{
	DeactivateSphere();
}