// Fill out your copyright notice in the Description page of Project Settings.


#include "BMHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "BMGameplayServerCharacter.h"

// Sets default values for this component's properties
UBMHealthComponent::UBMHealthComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    //Initialize the player's Health
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
}

// Called when the game starts
void UBMHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (Owner != nullptr) {
        Owner->OnTakeAnyDamage.AddDynamic(this, &UBMHealthComponent::HandleTakeAnyDamage);
    }
}

// Called every frame
void UBMHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// Replicated properties
void UBMHealthComponent::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    //Replicate current health.
    DOREPLIFETIME(UBMHealthComponent, CurrentHealth);
}

void UBMHealthComponent::OnRep_CurrentHealth()
{
    OnHealthUpdate();
}

void UBMHealthComponent::OnHealthUpdate()
{
    // Server-specific functionality
    if (GetOwnerRole() == ROLE_Authority)
    {
        FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, healthMessage);
    }

    // Client-specific functionality
    if (GetOwnerRole() == ROLE_AutonomousProxy && IsNetMode(NM_Client))
    {
        FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
        //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
    }

    // Functions that occur on all machines. 
    // Any special functionality that should occur as a result of damage or death should be placed here.
    OnHealthChangeDelegate.Broadcast(); // Broadcast in case owner wants to execute functionality
}

void UBMHealthComponent::SetCurrentHealth(float healthValue)
{
    if (GetOwnerRole() == ROLE_Authority)   // We only modify health on the server
    {
        CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);  // Impossible to set CurrentHealth to an invalid value
        OnHealthUpdate();   // This is necessary because the server will not recieve the RepNotify
    }
}

void UBMHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{   
    BMDamage(Damage);
}

void UBMHealthComponent::BMHeal(float healAmount)
{
    SetCurrentHealth(CurrentHealth + healAmount);
}

void UBMHealthComponent::BMDamage(float damageAmount)
{
    SetCurrentHealth(CurrentHealth - damageAmount);
}

void UBMHealthComponent::RestoreHealth()
{
    SetCurrentHealth(MaxHealth);
}

float UBMHealthComponent::GetNormalizedHealth() const
{
    return CurrentHealth / MaxHealth;
}

