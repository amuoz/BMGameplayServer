// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BMGameplayServerCharacter.h"
#include "BMGameplayServerProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "BMHealthComponent.h"
#include "Engine/Engine.h"
#include "BMGameplayServerGameMode.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "NavigationSystem.h"
#include "BMSphereAttackComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ABMGameplayServerCharacter

ABMGameplayServerCharacter::ABMGameplayServerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FP_Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Mesh"));
	FP_Mesh->SetOnlyOwnerSee(true);
	FP_Mesh->SetupAttachment(FirstPersonCameraComponent);
	FP_Mesh->bCastDynamicShadow = false;
	FP_Mesh->CastShadow = false;
	FP_Mesh->bReceivesDecals = false;
	FP_Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	FP_Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	FP_Mesh->SetCollisionObjectType(ECC_Pawn);
	FP_Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FP_Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	FP_Mesh->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	FP_Mesh->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(FP_Mesh, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for FP_Mesh, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create 3rd person gun mesh component
	TP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TP_Gun"));
	TP_Gun->SetOwnerNoSee(true);			// only other players will see this mesh
	TP_Gun->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));

	// ACharacter default mesh for 3rd person
	GetMesh()->SetOwnerNoSee(true);

	HealthComp = CreateDefaultSubobject<UBMHealthComponent>(TEXT("CharHealthComp"));
	if (HealthComp)
	{
		HealthComp->SetIsReplicated(true); // Enable replication
		HealthComp->OnHealthChangeDelegate.AddDynamic(this, &ABMGameplayServerCharacter::HealthChange);
	}

	SphereAttackComp = CreateDefaultSubobject<UBMSphereAttackComponent>(TEXT("SphereAttackComp"));
	if (SphereAttackComp)
	{
		SphereAttackComp->SetIsReplicated(true); // Enable replication
	}

	bDeath = false;
	RespawnTime = 5.0f;
}

void ABMGameplayServerCharacter::Respawn()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// Respawn at random navmesh location
		UNavigationSystemV1* navSys = UNavigationSystemV1::GetCurrent(GetWorld());
		FNavLocation navLocation;
		navSys->GetRandomPoint(navLocation);
		
		SetActorLocation(navLocation);

		// Max health
		HealthComp->RestoreHealth();
		
		bDeath = false;
	}
}

void ABMGameplayServerCharacter::Ragdoll()
{
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName("Ragdoll");

	if (IsLocallyControlled())
	{
		GetMesh()->SetOwnerNoSee(false);
		TP_Gun->SetOwnerNoSee(false);

		FP_Mesh->SetOwnerNoSee(true);
		FP_Gun->SetOwnerNoSee(true);

		// Disable input temporally
		ActivateDeathMode();
	}
}

void ABMGameplayServerCharacter::ResetCharacter()
{
	GetMesh()->AttachTo(GetCapsuleComponent(), NAME_None, EAttachLocation::SnapToTarget, true);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName("CharacterMesh");

	if (IsLocallyControlled())
	{
		GetMesh()->SetOwnerNoSee(true);
		TP_Gun->SetOwnerNoSee(true);

		FP_Mesh->SetOwnerNoSee(false);
		FP_Gun->SetOwnerNoSee(false);

		// Enable input
		DeactivateDeathMode();
	}
}

void ABMGameplayServerCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(FP_Mesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
	FP_Mesh->SetHiddenInGame(false, true);
	TP_Gun->SetOwnerNoSee(true);
	GetMesh()->SetOwnerNoSee(true);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABMGameplayServerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABMGameplayServerCharacter::OnFire);

	// Bind spell event
	PlayerInputComponent->BindAction("Spell", IE_Pressed, this, &ABMGameplayServerCharacter::OnActivateSpell);
	PlayerInputComponent->BindAction("Spell", IE_Released, this, &ABMGameplayServerCharacter::OnDeactivateSpell);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ABMGameplayServerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABMGameplayServerCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ABMGameplayServerCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ABMGameplayServerCharacter::LookUpAtRate);
}

void ABMGameplayServerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate current health.
	DOREPLIFETIME(ABMGameplayServerCharacter, bDeath);
}

void ABMGameplayServerCharacter::OnFire()
{
	// try fire a projectile
	if (ProjectileClass != NULL)
	{
		OnFireBP();
	}
}

void ABMGameplayServerCharacter::OnActivateSpell()
{
	SphereAttackComp->ServerActivateSphere();
}

void ABMGameplayServerCharacter::OnDeactivateSpell()
{
	SphereAttackComp->ServerDeactivateSphere();
}

void ABMGameplayServerCharacter::ActivateDeathMode()
{
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));

	DisableInput(Cast<APlayerController>(GetController()));
}

void ABMGameplayServerCharacter::DeactivateDeathMode()
{
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(45.0f, 0.0f, 0.0f));

	EnableInput(Cast<APlayerController>(GetController()));
}

void ABMGameplayServerCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ABMGameplayServerCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ABMGameplayServerCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ABMGameplayServerCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ABMGameplayServerCharacter::HealthChange()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// If player has died time to respawn
		if (HealthComp->GetCurrentHealth() <= 0 && !bDeath)
		{
			bDeath = true;

			// After 10 sec respawn
			FTimerHandle respawnTimer;
			GetWorldTimerManager().SetTimer<ABMGameplayServerCharacter>
				(respawnTimer, this, &ABMGameplayServerCharacter::Respawn, RespawnTime, false);
		}
	}
	
	// update local client widget
	if (IsLocallyControlled())
	{
		OnHealthEvent();
	}
}

void ABMGameplayServerCharacter::OnRep_Death()
{
	if (bDeath)
	{
		// If death Ragdoll in clients
		Ragdoll();
	}
	else
	{
		// If respawn restore character
		ResetCharacter();
	}
}