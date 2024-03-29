
#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "particles/ParticleSystem.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Components/BoxComponent.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/Pickups/Artifact.h"

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
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true; //set to true to enable crouching on character movement comp.

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);
	
	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));	//-- we only intend to use lag compenstion on the server; no nned to replicate it --.

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

	//make Rockets/Grenades spawn at muzzle fl;ash socket location also when not in the server's view
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	//Attach grenade to socket in right hand
	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/*
	* Hit boxes used for server side rewind
	*/

	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	backpack->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("backpack"), backpack);

	blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	blanket->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("blanket"), blanket);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	/* when doing line traces against the character's hitboxes for projectile weapons during SSR
	* if tracing against the visibility channel
	* we could hit the projectile hit box instead of the character's hitbox
	* thus we make our custom channel for character's hitboxes
	*/
	for (auto& Boxpair : HitCollisionBoxes)
	{
		if (Boxpair.Value)
		{
			// Disable its collision
			Boxpair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			//set its collision object type to custom made channel "HitBox"
			Boxpair.Value->SetCollisionObjectType(ECC_HitBox);

			// Ignore all trace channels except newly created HitBox channel
			Boxpair.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Boxpair.Value->SetCollisionResponseToChannel(ECC_HitBox ,ECollisionResponse::ECR_Block);
		}
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//replication EX: once the overlapping weapon is set on the server blaster character, set it on all client blaster characters.
	//C7_3: we rgister the variables we want to replicate here in this function, with a condition to replicate it to only the client that owns the character.
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
	DOREPLIFETIME(ABlasterCharacter, bSprinting);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();

	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpawnSound, GetActorLocation());
	}
	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}

	LimitPitchView();

	UpdateHUDHealth(); //set to max health
	UpdateHUDShield();

	//bind ReceiveDamage to OnTakeAnyDamage delgate, only take damage on the server
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDisableGameplay)											//disable rotation in place and control rotation
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (GetCharacterMovement()->GetMovementName() == FString("Flying"))
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	AimOffset(DeltaTime);

	HideCameraIfCharatcterClose();
	PollInit();
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		//on the frame that player state is not null
		if (BlasterPlayerState)
		{
			// set color based on team
			SetTeamColor(BlasterPlayerState->GetTeam());

			// a character died but still in the lead; respawn with crown
			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(GetWorld()));
			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
			{
				MultiCastGainedTheLead();
			}
		}
	}
	/*
	if (BlasterPlayerController == nullptr)
	{
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
		if (BlasterPlayerController)
		{
			SpawnDefaultWeapon();
			UpdateHUDHealth();
			UpdateHUDShield();
		}
	}*/
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveUp", this, &ABlasterCharacter::MoveUp);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::moveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ABlasterCharacter::SprintButtonPressed);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ABlasterCharacter::SprintButtonReleased);

	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ABlasterCharacter::GrenadeButtonPressed);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat-> Character = this; //combat comp private vars (charatcer) are now available to blaster character since it's a friend.
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched);

		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
	if (LagCompensation)
	{
		LagCompensation->BlasterCharacter = this;
		if (Controller)
		{
			LagCompensation->BlasterController = Cast<ABlasterPlayerController>(Controller);
		}
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
	if (bDisableGameplay) return;

	bMovingForward = value > 0.f ? true : false;	
	
	if (Controller != nullptr && value != 0)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, value);
	}
}

void ABlasterCharacter::MoveUp(float value)
{
	if (bDisableGameplay) return;

	FVector InputVelocity = FVector(0.0f, 0.0f, value);

	AddMovementInput(InputVelocity, true);
}

void ABlasterCharacter::SprintButtonPressed()
{
	bool bShouldSprint = bMovingForward &&
		Combat &&
		!Combat->bAiming &&
		Combat->EquippedWeapon;

	if (bShouldSprint) ServerSprint(true);
}

void ABlasterCharacter::SprintButtonReleased()
{
	if (bSprinting) ServerSprint(false);
}

void ABlasterCharacter::ServerSprint_Implementation(bool bSprint)
{
	if (bSprint)
	{
		bSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed *= 1.4;
	}
	else
	{
		bSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed /= 1.4;
	}
}

void ABlasterCharacter::OnRep_Sprinting()
{
	if (bSprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed *= 1.4;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed /= 1.4;
	}
}

void ABlasterCharacter::moveRight(float value)
{
	if (bDisableGameplay || bSprinting) return;

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
	if (bDisableGameplay) return;
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
	if (bDisableGameplay) return;

	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;

	if (Combat && Combat->EquippedWeapon)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat && Combat->EquippedWeapon)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;

	if (Combat) //only equip weapon through the server.
	{
		if (OverlappingWeapon || (Combat->CombatState == ECombatState::ECS_Unoccupied && Combat->ShouldSwapWeapons())) //only send rpc if u want to equip or swap weapons
		{
			ServerEquipButtonPressed();
		}

		bool bSwap = Combat->ShouldSwapWeapons() &&
			!HasAuthority() &&
			Combat->CombatState == ECombatState::ECS_Unoccupied &&
			OverlappingWeapon == nullptr;

		if (bSwap)
		{
			bFinishedSwapping = false; //disable FABRIK / also needs to be set for the server
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			PlaySwapMontage();
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation() //this is an RPC, we have to write "_implementation" in the function defintion in .cpp 
{
	//enable client blaster characters to also pickup a weapon when pressing E.
	//this function will be called from the client and only the server will excecute it..
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			//handle setting the state and playing the swap animation in SwapWeapons()
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay || !Combat || Combat->Artifact) return;

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
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(true); //setAiming will also send an rpc to server to replicate bAiming.
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;

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

bool ABlasterCharacter::isAiming() const
{
	return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if(!Combat){ return nullptr; }
	return Combat->EquippedWeapon;
}

/**
* Team Color
*/

void ABlasterCharacter::SetTeamColor(ETeam Team)
{
	if (!GetMesh() || !OriginalMaterial) return;

	switch (Team) 
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, OriginalMaterial);
		DissolveMaterialInstance = BlueDissolveMaterialInstance;
		break;

	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		DissolveMaterialInstance = BlueDissolveMaterialInstance;
		break;

	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		DissolveMaterialInstance = RedDissolveMaterialInstance;
		break;

	case ETeam::ET_MAX:
		break;
	}
}

ETeam ABlasterCharacter::GetPlayerTeam() const	// used to prevent friendly fire
{
	if (BlasterPlayerState) return BlasterPlayerState->GetTeam();
	else return ETeam::ET_NoTeam;
}

/*
* Default Weapon
*/

void ABlasterCharacter::SpawnDefaultWeapon()
{
	/*only spawn a default weapon in a map that is controlled by blaster game mode*/
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	if (BlasterGameMode && !bElimed && DefaultWeaponClass)	//GmaeMode is only valid on the server.. so we spawn on server only
	{
		StartingWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
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

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimeInstance = GetMesh()->GetAnimInstance();
	if (AnimeInstance && ThrowGrenadeMontage)
	{
		AnimeInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimeInstance = GetMesh()->GetAnimInstance();
	if (AnimeInstance && ReloadMontage)
	{
		AnimeInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case(EWeaponType::EWT_AssaultRifle):
			SectionName = FName("Rifle");
			break;

		case(EWeaponType::EWT_RocketLauncher):
			SectionName = FName("RocketLauncher");
			break;

		case(EWeaponType::EWT_Pistol):
			SectionName = FName("Pistol");
			break;

		case(EWeaponType::EWT_SubmachineGun):
			SectionName = FName("Pistol");
			break;

		case(EWeaponType::EWT_Shotgun):
			SectionName = FName("Shotgun");
			break;

		case(EWeaponType::EWT_SniperRifle):
			SectionName = FName("Sniper");
			break;

		case(EWeaponType::EWT_GrenadeLauncher):
			SectionName = FName("GrenadeLauncher");
			break;
		}	

		AnimeInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimeInstance = GetMesh()->GetAnimInstance();
	if (AnimeInstance && SwapMontage)
	{
		AnimeInstance->Montage_Play(SwapMontage);
	}
}

/*
* AIM OFFSET & TURNING IN PLACE
*/

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
	if (Combat->CombatState == ECombatState::ECS_ThrowingGrenade)
	{
		if (AO_Yaw > 0.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (AO_Yaw < 0.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
	}

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
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->SetVisibility(false);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(true);
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->SetVisibility(true);
		}
	}
}

/*
* HEALTH / SHIELD
*/

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCausor)
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (bElimed || !BlasterGameMode) return;
	
	Damage = BlasterGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitreactMontage();

	if (Health == 0.f)
	{
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
		if (BlasterPlayerController)
		{
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);

			if (BlasterGameMode && AttackerController)
			{
				BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
			}
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (Health < LastHealth)
	{
		PlayHitreactMontage();
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (Shield < LastShield)
	{
		PlayHitreactMontage();
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

/**
* Gain/Lose the lead
*/

void ABlasterCharacter::MultiCastGainedTheLead_Implementation()
{
	//only spawn the crown 
	if (CrownSystem && CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetMesh(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 120.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void ABlasterCharacter::MultiCastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
		CrownComponent = nullptr;
	}
}

/*
* ELIMINATION / LEAVING THE GAME
*/

void ABlasterCharacter::Eliminate(bool bPlayerLeftGame)//only called on server
{
	bElimed = true; // set on server first.. to not equip artifact on elimmed characters

	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			Combat->EquippedWeapon->Dropped();
		}
		if (Combat->SecondaryWeapon)
		{
			Combat->SecondaryWeapon->Dropped();
		}

		if (Combat && Combat->Artifact)
		{
			Combat->Artifact->Dropped();
			SetArtifact(nullptr);
		}

		if (StartingWeapon && StartingWeapon->GetOwner() == nullptr) StartingWeapon->Destroy();
	}

	MulticastEliminate(bPlayerLeftGame);
}

void ABlasterCharacter::MulticastEliminate_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;	//Did player leave the game?
	bElimed = true;
	/*if character is elimmed, set the ammo HUD to zero*/
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
		BlasterPlayerController->SetHUDCarriedAmmo(0);
		BlasterPlayerController->SetHUDWeaponType(FText::FromString(" "));
	}

	// Destroy crown if player in lead
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}

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

	bDisableGameplay = true;
	if (Combat)											//Disable shooting if eliminated
	{
		Combat->FireButtonPressed(false);
	}
	//Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Elim Bot effect
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnPoint, GetActorRotation());
	}
	if (ElimSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ElimSound, GetActorLocation());
	}

	//UnScope sniper rifle if eliminated while scoping
	if (IsLocallyControlled() && Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle && Combat->bAiming)
	{
		Combat->DrawSniperScope(false);
	}

	//set timer that wil trigger respawn
	GetWorldTimerManager().SetTimer(
		ElimTimerHandle,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void ABlasterCharacter::ElimTimerFinished()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	//Only request a respawn if the player didn't leave the game.. 
	if (BlasterGameMode && !bLeftGame)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
	//Broadcast the OnleftDelegte so that ReturnMenu can use its callback
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;	//leaving player state
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	if (BlasterGameMode && BlasterPlayerState)
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);	//checks to see if leaving player is among the top scoring players
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	//destroy elim bot
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	bool bShouldAlsoDestroyWeapon =
		Combat &&
		Combat->EquippedWeapon &&
		BlasterGameMode &&
		BlasterGameMode->GetMatchState() != MatchState::InProgress;

	if (bShouldAlsoDestroyWeapon)
	{
		Combat->EquippedWeapon->Destroy();										//Don't leave the weapon hanging once cooldown state is over.
	}
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

/*
* Public Setters/Getters
*/

bool ABlasterCharacter::isWeaponEquipped() const
{
	return (Combat && Combat->EquippedWeapon); //we didn't do the check on "OverlappedWeapon" because overlapping with the weapon doesn't mean we equipped it. we check that in cmbat comp.
}

ECombatState ABlasterCharacter::GetCombatState()
{
	if (!Combat) return ECombatState::ECS_MAX;

	return Combat->CombatState;
}

bool ABlasterCharacter::bIsLocallyReloading()
{
	return Combat && Combat->bLocalClientSideReloading;
}
/*
* Function called by other classes.. Usually to access BlasterCharacter's components
*/

// called by the artifact to attach to backpack
void ABlasterCharacter::SetArtifact(AActor* Actor)
{
	if (!Combat) return;

	AArtifact* Artifact = Cast<AArtifact>(Actor);
	if (Artifact)
	{
		Combat->Artifact = Artifact;
		Combat->AttachActorToBackpack(Actor);

		SetMovementMode(EMovementMode::MOVE_Flying);
	}
	else
	{
		Combat->Artifact = nullptr; 
		SetMovementMode(EMovementMode::MOVE_Walking);
	}
}

bool ABlasterCharacter::IsHoldingArtifact() const
{
	return (Combat && Combat->Artifact);
}

AArtifact* ABlasterCharacter::GetArtifact()
{
	if (!Combat) return nullptr;

	return Combat->Artifact;
}

void ABlasterCharacter::SetMovementMode(EMovementMode Mode)
{
	GetCharacterMovement()->SetMovementMode(Mode);
}