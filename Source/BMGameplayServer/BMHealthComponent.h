// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BMHealthComponent.generated.h"


UCLASS()
class BMGAMEPLAYSERVER_API UBMHealthComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthChangeDelegate);

private:
	UPROPERTY(Transient, DuplicateTransient)
	class ABMGameplayServerCharacter* CharacterOwner;

public:	
	// Sets default values for this component's properties
	UBMHealthComponent();

	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Broadcast when health change received */
	UPROPERTY(BlueprintAssignable)
	FOnHealthChangeDelegate OnHealthChangeDelegate;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** RepNotify for changes made to current health */
	UFUNCTION()
	void OnRep_CurrentHealth();

	/** Response to health being updated. Called on the server immediately after modification, and on clients in response to a RepNotify */
	void OnHealthUpdate();

	/** The player's maximum health. This is the highest that their health can be, and the value that their health starts at when spawned */
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth;

	/** The player's current health */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;

	/** Setter for Current Health. Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. Should only be called on the server.*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetCurrentHealth(float healthValue);

	/** Damage handler for owner actor */
	UFUNCTION(BlueprintCallable)
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Getter for Max Health.*/
	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** Getter for Current Health.*/
	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	/** Player has max health */
	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE bool IsMaxHealth() const { return CurrentHealth == MaxHealth; }

	/** Getter for Current Health.*/
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetNormalizedHealth() const;

	/** Heal the player */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void BMHeal(float healAmount);

	/** Damage the player */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void BMDamage(float damageAmount);

	/** Restore health to its maximum */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void RestoreHealth();

};
