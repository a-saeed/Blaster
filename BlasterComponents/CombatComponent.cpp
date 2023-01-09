#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Blaster/BlasterTypes/CombatState.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon); //equipped weapon only gets set by the server, replicate it so that every charatcer see the new animation pose for the character that equipped the weapon..
	DOREPLIFETIME(UCombatComponent, bAiming);
	/*only the ownning client needs to see his own carried ammo..opposite to the ammo on the weapon as if a client dropped itand another picked it, it needs to see the how much ammo was used*/
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	//set defaultFOV
	if (Character->GetFollowCamera())
	{
		DefaultFOV = Character->GetFollowCamera()->FieldOfView;
		CurrentFOV = DefaultFOV;
	}
	/*initialize carried ammo only on the server*/
	if (Character->HasAuthority())
	{
		InitializeCarriedAmmo();
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint; //needed each frame in the anime instance to correct the weapon rotation.

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}
/*
*
* DopWeapon, EquipWeapon , RPCs , REP_NOTIFIES
*
*/		
void UCombatComponent::DropWeapon() //called in Blaster Character's Eliminate()
{
	if (!EquippedWeapon) return;

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Dropped); //replicated

	EquippedWeapon->GetWeaponMesh()->DetachFromComponent(DetachRules);
	EquippedWeapon->SetOwner(nullptr);
	/*a weapon shouldn't store info about a player that dropped it*/
	EquippedWeapon->SetOwnerCharacter(nullptr);
	EquippedWeapon->SetOwnerController(nullptr);
}	
	
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;
	/*if we already have an equipped weapon, drop it if we are to equip another weapon*/
	if (EquippedWeapon)
	{
		DropWeapon();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); //this is the enum we created in weapon class. (we need to replicate it to all clients)

	const USkeletalMeshSocket* HandSocket = Character-> GetMesh()-> GetSocketByName(FName("RightHandSocket")); //we created a socket in the character skeletal mesh and named it RightHandSocket

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //attach the weapon to the character mesh
	}

	/*will trigger OnRep_Owner in the weapon class*/
	EquippedWeapon->SetOwner(Character);

	/*dispaly ammo amount in the weapon just equipped*/
	EquippedWeapon->SetHUDAmmo();

	/*set and display carried ammo on the server once weapon equipped, will trigger OnRep_CarriedAmmo*/
	SetCarriedAmmoOnEquip();
	UpdateHUDCarriedAmmo();

	//once we equip the weapon, adjust these settings to allow strafing pose
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	/*need to make sure that the weapon state gets replicated first before we attach the weapon as we can't attach if physics is still enabeled*/
	if (EquippedWeapon && Character) //on replicating equipped weapon to clients
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); //this is the enum we created in weapon class. (we need to replicate it to all clients)

		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")); //we created a socket in the character skeletal mesh and named it RightHandSocket

		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //attach the weapon to the character mesh
		}
		/*--------------------------------------------------------------------------------------*/
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}
/*
*
* Reload
*
*/
void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)//already reloading
	{
		ServerReload();
	}
}

void UCombatComponent::FinishReloading() //called from animation bluprint
{
	/*this is an anim notify function which will be called when the reload animation has finished.*/
	if (!Character) return;
	/*we call it on the server only and since combat state is replicated, it will trigger OnRep_CombatState*/
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied; //reset combat state to unoccupied.
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (!Character) return;
	
	CombatState = ECombatState::ECS_Reloading; //trigger OnRep_CombatState.
	HandleReload();
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}
/*
*
* SetAiming , RPCs , REP_NOTIFIES
*
*/				
void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming; //bAiming is what's being checked every frame by the ABP to see whether to play the aiming pose or not. client can set it and see his aim pose before sending rpc to server to allow for a smooth xp. once rpc reaches server it will replicate to all clients to see this client's aim pose.

	if (Character)
	{
		//reduce character speed if we're aiming (it'll be set locally, but the movement compoenet takes care of replication..still, we can set in the server as well)
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	ServerSetAiming(bIsAiming); //whether server or client, this rpc will be called from the respective machine and only executed on server, vraiable already replicates.
}
					
void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;

	if (Character)
	{
		//reduce character speed if we're aiming
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}
/*
*	
* FireButtonPressed , RPCs , REP_NOTIFIES
* 
*/					
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	/*
	* the default scenario before making any rpcs is that ONLY the client(or server) would see the fire weapon animation when shooting
	* creating a server rpc only made it play on the server, since bFireButtonPressed is not replicated.
	* we don't replicate bFireButtonPressed since we intend to implement automatic weapons, and replication only works when a variable changes value(replication would work for semi-auto).
	* the fix is to keep the server rpc and within it call a multicast(server) rpc that will play the effect on all clients and server.
	* the way to know it's a multicast server rpc: -a client/server press fire button -call serverFire(a server rpc.. not client) -call multicast rpc.
	*/
	bFireButtonPressed = bPressed;
	
	if (bFireButtonPressed && CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);
		StartFireTimer();
	}
	else if (bFireButtonPressed && EquippedWeapon->IsEmpty())
	{
		if (CarriedAmmo == 0 && EmptySound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), EmptySound, EquippedWeapon->GetActorLocation());
		}
		else
		{
			Reload();
		}	
	}
}
					
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && EquippedWeapon)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}
/*
*
* AUTOMATIC FIRE
*
*/
void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon || !Character) return;

	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if (!EquippedWeapon) return;

	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->IsAutomatic())
	{
		FireButtonPressed(true);
	}
}
bool UCombatComponent::CanFire()
{
	return EquippedWeapon && !EquippedWeapon->IsEmpty() && bCanFire;
}
/*
*
* Crosshairs and HUD
*
*/
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (!EquippedWeapon) return;
	//make a line trace from the middle of the screen
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); //in screen space
	//convert to world space
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(GetWorld(), 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		//perform the line trace
		FVector Start = CrosshairWorldPosition;
		//make the trace start front of the character, so we don't get a hit on the character.
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}
		FVector End = Start + CrosshairWorldDirection * EquippedWeapon->GetWeaponFiringRange(); //world direction is a unit vec.
		GetWorld()->LineTraceSingleByChannel(TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit) //if we didn't hit smth. set the impact point to the end vector.
		{
			TraceHitResult.ImpactPoint = End;
		}
	}

	SetCrosshairsColor(TraceHitResult);
}

void UCombatComponent::SetCrosshairsColor(FHitResult& TraceHitResult)
{
	//check if the ator we hit implements the IInteractWithCrosshairsInterface
	if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
	{
		HUDPackage.CrosshairsColor = FLinearColor::Red;
	}
	else
	{
		HUDPackage.CrosshairsColor = FLinearColor::White;
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime) //called in tick
{
	//need to draw the crosshairs through Blaster HUD Draw fn.
	//Player controller has access to HUD, we cast them to BlasterController and BlasterHUD.. from which we set the corsshairs textures
	if (!Character || !Character->Controller) return;

	if (!BlasterController)
	{
		BlasterController = Cast<ABlasterPlayerController>(Character->Controller);
	}

	if (!BlasterHUD)
	{
		BlasterHUD = Cast<ABlasterHUD>(BlasterController->GetHUD());
	}

	if (!BlasterController || !BlasterHUD) return;

	if (EquippedWeapon)
	{
		HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
		HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
		HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
		HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
		HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
	}
	else
	{
		HUDPackage.CrosshairsCenter = nullptr;
		HUDPackage.CrosshairsTop = nullptr;
		HUDPackage.CrosshairsRight = nullptr;
		HUDPackage.CrosshairsLeft = nullptr;
		HUDPackage.CrosshairsBottom = nullptr;
	}

	SetCrosshairsSpread(DeltaTime);

	BlasterHUD->SetHUDPackage(HUDPackage);
}

void UCombatComponent::SetCrosshairsSpread(float DeltaTime)
{
	//Calculate Crosshairs Spread
	//[0, maxWalkSpeed] -> [0,1]
	FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
	FVector2D VelocityMultiplierRange(0.f, 1.f);
	FVector Velocity = Character->GetVelocity();
	Velocity.Z = 0.f;
	//Velocity Factor
	CrosshairsVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Length());
	//In Air Factor
	if (Character->GetCharacterMovement()->IsFalling())
	{
		CrosshairsInAirFactor = FMath::FInterpTo(CrosshairsInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else
	{
		CrosshairsInAirFactor = FMath::FInterpTo(CrosshairsInAirFactor, 0.f, DeltaTime, 30.f);
	}
	//Aim Factor
	if (bAiming)
	{
		CrosshairsAimFactor = FMath::FInterpTo(CrosshairsAimFactor, 0.58f, DeltaTime, 30.f);
	}
	else
	{
		CrosshairsAimFactor = FMath::FInterpTo(CrosshairsAimFactor, 0.f, DeltaTime, 30.f);
	}
	//Shooting Factor
	if (bFireButtonPressed && EquippedWeapon && !EquippedWeapon->IsEmpty())
	{
		CrosshairsShootingFactor = 0.75f;
	}
	else
	{
		CrosshairsShootingFactor = FMath::FInterpTo(CrosshairsShootingFactor, 0.f, DeltaTime, 40.f);
	}
	//Crosshairs On Target Factor
	if (HUDPackage.CrosshairsColor == FLinearColor::Red)
	{
		if (bAiming && Velocity.Length() <= 0.f)
		{
			CrosshairsOnTargetFactor = 0;
		}
		else
		{
			CrosshairsOnTargetFactor = FMath::FInterpTo(CrosshairsOnTargetFactor, 0.36f, DeltaTime, 30.f);
		}
	}
	else
	{
		CrosshairsOnTargetFactor = FMath::FInterpTo(CrosshairsOnTargetFactor, 0.f, DeltaTime, 30.f);
	}

	HUDPackage.CrosshairSpread =
		0.5f +								//base spread
		CrosshairsVelocityFactor +
		CrosshairsInAirFactor -
		CrosshairsAimFactor +
		CrosshairsShootingFactor - 
		CrosshairsOnTargetFactor
		;
}
/*
*
* AIMING AND FOV
*
*/
void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!EquippedWeapon) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed); //unzoom to defaultFOV at the same rate.
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}
/*
* 
* CARRIED AMMO
* 
*/
void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::SetCarriedAmmoOnEquip()
{
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; //replicated and therfore will triiger OnRep_CarriedAmmo
	}
	else
	{
		CarriedAmmo = 0; //may want to add a rare weapon that no player has its type of ammo.
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	UpdateHUDCarriedAmmo();
}

void UCombatComponent::UpdateHUDCarriedAmmo()
{
	BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : BlasterController;
	if (BlasterController)
	{
		BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
	}
}