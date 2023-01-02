#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
//#include "Blaster/HUD/BlasterHUD.h"
#include "Camera/CameraComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon); //equipped weapon only gets set by the server, replicate it so that every charatcer see the new animation pose for the character that equipped the weapon..
	DOREPLIFETIME(UCombatComponent, bAiming);
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
* EquipWeapon , RPCs , REP_NOTIFIES
*
*/					/// EW
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); //this is the enum we created in weapon class. (we need to replicate it to all clients)

	const USkeletalMeshSocket* HandSocket = Character-> GetMesh()-> GetSocketByName(FName("RightHandSocket")); //we created a socket in the character skeletal mesh and named it RightHandSocket

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh()); //attach the weapon to the character mesh
	}

	EquippedWeapon->SetOwner(Character); //set this character as the owner of this weapon.

	//once we equip the weapon, adjust these settings to allow strafing pose
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}
					///EW: REP_NOTIFIES
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character) //on replicating equipped weapon to clients
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}
/*
*
* SetAiming , RPCs , REP_NOTIFIES
*
*/					/// SA
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
					/// SA: RPCs
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
*/					/// FBP
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

	if (bFireButtonPressed)
	{
		ServerFire(HitTarget);
	}
}
					/// FBP: RPCs
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
		FVector End = CrosshairWorldPosition + CrosshairWorldDirection * EquippedWeapon->GetWeaponFiringRange(); //world direction is a unit vec.

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
	if (!EquippedWeapon) return;
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

	HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
	HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
	HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
	HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
	HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;

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
	if (bFireButtonPressed)
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