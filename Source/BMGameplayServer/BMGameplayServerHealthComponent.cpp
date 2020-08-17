// Fill out your copyright notice in the Description page of Project Settings.


#include "BMGameplayServerHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
//#include "BMGameplayServerCharacter.h"

// Sets default values for this component's properties
UBMGameplayServerHealthComponent::UBMGameplayServerHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//Initialize the player's Health
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
}


// Called when the game starts
void UBMGameplayServerHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UBMGameplayServerHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

/*
void UBMGameplayServerHealthComponent::PostLoad()
{
    Super::PostLoad();

    //CharacterOwner = Cast<ABMGameplayServerCharacter>(GetOwner());
}
*/

// Replicated properties
void UBMGameplayServerHealthComponent::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate current health.
	DOREPLIFETIME(UBMGameplayServerHealthComponent, CurrentHealth);
}

void UBMGameplayServerHealthComponent::OnRep_CurrentHealth()
{
    OnHealthUpdate();
}

void UBMGameplayServerHealthComponent::OnHealthUpdate()
{
    //Client-specific functionality
    if(GetOwnerRole() == ROLE_AutonomousProxy)
    {
        FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

        if (CurrentHealth <= 0)
        {
            FString deathMessage = FString::Printf(TEXT("You have been killed."));
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
        }

        /*
        if (CharacterOwner)
        {
            CharacterOwner->OnHealthUpdateEvent();
        }
        */
    }

    //Server-specific functionality
    if (GetOwnerRole() == ROLE_Authority)
    {
        FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, healthMessage);
    }

    // Functions that occur on all machines. 
    // Any special functionality that should occur as a result of damage or death should be placed here.
}

void UBMGameplayServerHealthComponent::SetCurrentHealth(float healthValue)
{
    if (GetOwnerRole() == ROLE_Authority)   // We only modify health on the server
    {
        CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);  // Impossible to set CurrentHealth to an invalid value
        OnHealthUpdate();   // This is necessary because the server will not recieve the RepNotify
    }
}
