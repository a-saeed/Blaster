
#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "TimerManager.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh()); // attach to static mesh as capsule will change size while crouching!
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = 600;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false; //lock player rotation while freely moving the camera.
	GetCharacterMovement()->bOrientRotationToMovement = true; //rotate the character towards the direction of movement.
	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true); //set combat component to replicate.

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true; //set to true to enable crouching on character movement comp.

	//don't make capsule and mesh block the camera
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	//make mesh block the Visibility channel to be able to do line traces against it.
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	//set the collision object type for the mesh to be the custom made skeletal mesh.
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	//set default turning pose to not turning
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	//set net update frequency
	NetUpdateFrequency = 66;
	MinNetUpdateFrequency = 33;

	//set spawn handling
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	//construct the dissolve timeline
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//replication EX: once the overlapping weapon is set on the server blaster character, set it on all client blaster characters.
	//C7_3: we rgister the variables we want to replicate here in this function, with a condition to replicate it to only the client that owns the character.
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	LimitPitchView();

	UpdateHUDHealth(); //set to max health

	//bind ReceiveDamage to OnTakeAnyDamage delgate, only take damage on the server
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);

	HideCameraIfCharatcterClose();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::moveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat-> Character = this; //combat comp private vars (charatcer) are now available to blaster character since it's a friend.
	}
}

void ABlasterCharacter::LimitPitchView()
{
	if (Controller)
	{
		BlasterPlayerController = Cast<ABlasterPlayerController>(Controller);
		if (BlasterPlayerController)
		{
			//limit the pitch to (70, -70)
			BlasterPlayerController->PlayerCameraManager->ViewPitchMax = 70.f;
			BlasterPlayerController->PlayerCameraManager->ViewPitchMin = -70.f;
		}
	}
}

void ABlasterCharacter::MoveForward(float value)
{
	if (Controller != nullptr && value != 0)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, value);
	}
}

void ABlasterCharacter::moveRight(float value)
{
	if (Controller != nullptr && value != 0)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, value);
	}
}

void ABlasterCharacter::LookUp(float value)
{
	AddControllerPitchInput(value);
}

void ABlasterCharacter::Turn(float value)
{
	AddControllerYawInput(value);
}

void ABlasterCharacter::Jump()
{
	//make character stand up if jump button is pressed while crouching.
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat) //only equip weapon through the server.
	{
		if (HasAuthority()) //if ur the server, equip weapon
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed(); //ur a client whos's trying to equip a weapon, call the RPC
		}	
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation() //this is an RPC, we have to write "_implementation" in the function defintion in .cpp 
{
	//enable client blaster characters to also pickup a weapon when pressing E.
	//this function will be called from the client and only the server will excecute it..
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched) //bIsCrouched, Crouch, unCrouch ae all memebers of character movement comp.
	{
		UnCrouch();
	}
	else
	{
		Crouch(); 
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true); //setAiming will also send an rpc to server to replicate bAiming.
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	//C7_10: we know this function will only be called by the server, as seen in the weapon class hasAuthority fn.
	//if a client blaster character overlapped with a weapon, then the server will replicate the overlappingWeapon variable to only this client as seen in C7_3
	//if the server blaster character overlapped withh a weapon, then the overlappingWeapon variable will not replicate and will be set only on the sever.
	if (OverlappingWeapon && IsLocallyControlled())
	{
		OverlappingWeapon->ShowPickupWidget(false); //C7_13: hide the pickup widget on end overlap for the server.
	}

	OverlappingWeapon = Weapon;

	if (OverlappingWeapon && IsLocallyControlled()) //check if the blaster character that ovrelapped with the weapon is locally controlled (server) or not (then it's a client character)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	//C7_9: this function only gets called when this variable replicates from server to client. it doesn't get called on the server.
	if (OverlappingWeapon)
	{
		OverlappingWeapon-> ShowPickupWidget(true);
	}
	if (LastWeapon) //C7_12: hide the pickup widget on end overlap.. what is the last value for overlappingWeapon before it changed and not yet replicated (either null or a weapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ABlasterCharacter::isWeaponEquipped() const
{
	return (Combat && Combat->EquippedWeapon); //we didn't do the check on "OverlappedWeapon" because overlapping with the weapon doesn't mean we equipped it. we check that in cmbat comp.
}

bool ABlasterCharacter::isAiming() const
{
	return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if(!Combat){ return nullptr; }
	return Combat->EquippedWeapon;
}
/*
* ANIMATION MONTAGE
*/
void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimeInstance = GetMesh()->GetAnimInstance();
	if (AnimeInstance && FireWeaponMontage)
	{
		AnimeInstance->Montage_Play(FireWeaponMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip"); //check which monatge section to play
		AnimeInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayHitreactMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimeInstance = GetMesh()->GetAnimInstance();
	if (AnimeInstance && HitReactMontage)
	{
		AnimeInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimeInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimeInstance = GetMesh()->GetAnimInstance();
	if (AnimeInstance && ElimMontage)
	{
		AnimeInstance->Montage_Play(ElimMontage);
	}
}

/***************** AIM OFFSET & TURNING IN PLACE ************************/

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	//we need to apply yaw aim offsets (looking left & right) only when the charcater is equipped & standing (not walking or jumping)
	if (Combat && Combat->EquippedWeapon == nullptr) return;

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f; 
	float Speed = Velocity.Length();

	bool bInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bInAir) //standing still, not jumping
	{
		//the yaw offset we need is the delta btween the last pose for the character before it stopped walking/falling & the current pose that's affected by the mouse yaw input
		FRotator CurrentAimRotation(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaBaseAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, LastAimRotation);
		AO_Yaw = DeltaBaseAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning) //ie. if ao_yaw haven't exceeded 90 or -90
		{
			InterpAO_Yaw = AO_Yaw; //we get out of this (if) check with a vlaue for interpAO_Yaw = 90 or -90
		}

		//stop character from following the controller (mouse).
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);
	}
	if(Speed > 0.f || bInAir)
	{
		LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f; //if not standing, set the aim offset yaw to zero(i.e. don't apply aim offsets while moving)
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	//for pitch, we can set it in any pose (walk, jump, stand, crouch).. need to convert values from [270-360) to [-90,0)
	AO_Pitch = GetBaseAimRotation().GetNormalized().Pitch;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if(AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning) //if we are turning
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f) //once we've turned enough
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); //since wev'e turned(i.e. MOVED) we need to reset the last aim rotation to the rotation we're currently in after turning.
		}
	}
}

FVector ABlasterCharacter::GetHitTarget()
{
	if (!Combat) return FVector();

	return Combat->HitTarget;
}

/*
* CAMERA
*/
void ABlasterCharacter::HideCameraIfCharatcterClose()
{
	if (!IsLocallyControlled()) return; //we only need to hide for our character

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Length() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(false);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(true);
		}
	}
}
/*
* PLAYER STATS
*/
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCausor)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth); // 0.f < H-D > MH .. health is repliacated, call onRep_Health

	UpdateHUDHealth();
	PlayHitreactMontage();

	if (Health == 0.f)
	{
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
		if (BlasterPlayerController)
		{
			ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);

			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitreactMontage();
}

void ABlasterCharacter::UpdateHUDHealth()
{
	if (Controller && !BlasterPlayerController)
	{
		BlasterPlayerController = Cast<ABlasterPlayerController>(Controller);
	}
	if (BlasterPlayerController)
	{
		//set hud health values
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}
/*
* ELIMINATION
*/
void ABlasterCharacter::Eliminate()//only called on server
{
	if (Combat)
	{
		Combat->DropWeapon();
	}

	MulticastEliminate();
	//set timer that wil trigger respawn
	GetWorldTimerManager().SetTimer(
		ElimTimerHandle,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay);
}

void ABlasterCharacter::MulticastEliminate_Implementation()
{
	bElimed = true;
	PlayElimMontage();
	//we want all machines to see the dissolve effect
	if (DissolveMaterialInstance)
	{
		//if it's vaild, create the dynamic version based on it.
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);

		//set the mesh with newly created dynamic material instance
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);

		//set the parametes we created in the Material instance in blueprint (glow, Dissolve)
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.55f); //0.55 is when the material isn't dissolved at all
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	//play the timeline with the function we created
	StartDissolve();

	//Disable character movement once eliminated
	GetCharacterMovement()->DisableMovement(); //Stop moving with WASD.
	GetCharacterMovement()->StopMovementImmediately(); //Prevents us from rotating character.
	if (BlasterPlayerController)
	{
		DisableInput(BlasterPlayerController);
	}
	//Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	//respawn player
	BlasterGameMode->RequestRespawn(this, Controller);
}
/*
* DISSOLVE EFFECTS
*/
void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	//when the dissolve timeline gets played, this function is called.
	//bind callback function to the dissolve timeline track
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);

	//add the float cureve to the timeline
	if (DissolveCurve && DissolveTimeline)
	{
		//set the dissolve timeline to use the dissolve curve and associte that curve with the dissolveTrack.which has our UpdateDissolveMaterial callback bound to it
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		//start the timeline
		DissolveTimeline->Play();
	}
}