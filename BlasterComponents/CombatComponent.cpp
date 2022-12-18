#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon); //equipped weapon only gets set by the server, replicate it so that every charatcer see the new animation pose for the character that equipped the weapon..
	DOREPLIFETIME(UCombatComponent, bAiming);
}

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
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming; //bAiming is what's being checked every frame by the ABP to see whether to play the aiming pose or not. client can set it and see his aim pose before sending rpc to server to allow for a smooth xp. once rpc reaches server it will replicate to all clients to see this client's aim pose.

	ServerSetAiming(bIsAiming); //whether server or client, this rpc will be called from the respective machine and only executed on server, vraiable already replicates.
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
}