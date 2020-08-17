// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BMGameplayServerCharacter.generated.h"

// forwards
class UInputComponent;

UCLASS(config=Game, BlueprintType)
class ABMGameplayServerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABMGameplayServerCharacter();

protected:
	virtual void BeginPlay();

public:

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Mesh;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** 1st person AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FP_FireAnimation;

	/** Gun mesh: 3rd person view (seen only by others) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* TP_Gun;

	/** 3rd person AnimMontage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* TP_FireAnimation;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Projectile)
	TSubclassOf<class ABMGameplayServerProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UBMHealthComponent* HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UBMSphereAttackComponent* SphereAttackComp;

	/** Respawn time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float RespawnTime;

	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	
	/** Fires a projectile. */
	void OnFire();

	/** Fires a projectile. */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnFireBP"))
	void OnFireBP();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);
	
	/** Death control. */
	UPROPERTY(ReplicatedUsing = OnRep_Death, VisibleAnywhere, BlueprintReadOnly)
	bool bDeath;

	/** RepNotify for death: activate ragdoll / respawn */
	UFUNCTION()
	void OnRep_Death();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* Respawn character on server at random position with navmesh */
	void Respawn();

	/* Activates ragdolls on clients */
	void Ragdoll();

	/* On respawn reset character properties */
	void ResetCharacter();

	/** Activate spell */
	void OnActivateSpell();

	/** Deactivate spell */
	void OnDeactivateSpell();

	/* Activate death camera mode */
	void ActivateDeathMode();

	/* Deactivate death camera mode */
	void DeactivateDeathMode();

public:
	/** Returns FP_Mesh subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetFPMesh() const { return FP_Mesh; }
	
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Health change event binded to HealthComponent */
	UFUNCTION()
	void HealthChange();

	/** Client widget update events */

	// Health change event
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnHealthEvent"))
	void OnHealthEvent();

	// Sphere radius event
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnSphereEvent"))
	void OnSphereEvent();

	// Cooldown change event
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnCooldownEvent"))
	void OnCooldownEvent();

	// Sphere enemy overlap event
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnEnemyOverlapEvent"))
	void OnEnemyOverlapEvent();
};
