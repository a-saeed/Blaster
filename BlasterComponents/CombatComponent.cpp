#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

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
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult OutHitResult;
	TraceUnderCrosshairs(OutHitResult);
}

///**************************************************************************************///
/*
*
* EquipWeapon , RPCs , REP_NOTIFIES
*
*/
					/// EW
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

					///EW: RPCs

					///EW: REP_NOTIFIES
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character) //on replicating equipped weapon to clients
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

///**************************************************************************************///
/*
*
* SetAiming , RPCs , REP_NOTIFIES
*
*/
					/// SA
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
					/// SA: REP_NOTIFIES
	
///**************************************************************************************///
/*
*	
* FireButtonPressed , RPCs , REP_NOTIFIES
* 
*/
					/// FBP
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
		ServerFire();
	}
}
					/// FBP: RPCs
void UCombatComponent::ServerFire_Implementation()
{
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{
	if (Character && EquippedWeapon)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire();
	}
}
				/// FBP: REP_NOTIFIES

///**************************************************************************************///

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
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

	if (bScreenToWorld && EquippedWeapon)
	{
		//perform the line trace
		FVector Start = CrosshairWorldPosition;
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
		else
		{
			DrawDebugSphere(GetWorld(),
				TraceHitResult.ImpactPoint,
				25.f,
				32.f,
				FColor::Red
			);
		}
	}
}