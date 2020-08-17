// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BMSphereAttackComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BMGAMEPLAYSERVER_API UBMSphereAttackComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(Transient, DuplicateTransient)
	class ABMGameplayServerCharacter* CharacterOwner;

public:	
	// Sets default values for this component's properties
	UBMSphereAttackComponent();

	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void ActivateSphere();

	void DeactivateSphere();

	void FireSpell();

	int CheckOverlapEnemies();

	/** The sphere's maximum radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float InitialRadius;

	/** The sphere's maximum radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float MaxRadius;

	/** The sphere's speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float SpeedRadius;

	/** The sphere's cooldown */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float Cooldown;

	/** Enemies in range */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	int NumEnemies;

	/** The spell's damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	float DamageAmount;

	/** The sphere's current radius */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentRadius)
	float CurrentRadius;

	/** The sphere's current cooldown */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCooldown)
	float CurrentCooldown;

	/** Spell tick activation */
	UPROPERTY(Replicated)
	bool Activated;

	/** RepNotify for changes made to current radius */
	UFUNCTION()
	void OnRep_CurrentRadius();

	/** RepNotify for changes made to current cooldown */
	UFUNCTION()
	void OnRep_CurrentCooldown();

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, reliable)
	void ServerActivateSphere();

	UFUNCTION(Server, reliable)
	void ServerDeactivateSphere();

	/** Is cooldown active */
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE bool IsInCooldown() const { return CurrentCooldown > 0; }

	/** Normalized sphere radius */
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE float GetNormalizedRadius() const { return (CurrentRadius - InitialRadius) / (MaxRadius - InitialRadius); }

	/** Normalized cooldown */
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE float GetNormalizedCooldown() const { return CurrentCooldown / Cooldown; }

	/** Normalized cooldown */
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE float GetCurrentCooldown() const { return CurrentCooldown; }

	/** Normalized cooldown */
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	FORCEINLINE int GetNumEnemies() const { return NumEnemies; }

};
