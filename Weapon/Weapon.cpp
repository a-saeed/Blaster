#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Casing.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; //for weapon to have authority only on the server, we nned to set it to replicate. (the serve will be in charge of all weapon objects)
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh-> SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block); //when dropped it can bounce off walls for ex.
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore); //ignore any pawns (they could step on it)
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); //this will be the initial state of weapon before picking it up

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); //we want it to detect overlaps only on the server, so we construct it ignore all collisions.
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); //only let the server enable its collision

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PicupWidget"));
	PickupWidget->SetupAttachment(RootComponent);

	/*Custom depth outline effect*/
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);									//all weapons start off with custom depth enabled.
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	
	if (PickupWidget) //hide pickup widget at the beginning.
	{
		ShowPickupWidget(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::BindToHighPingDelegate(bool bShouldBind)
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && bUseServerSideRewind) //weapon doesn't use SSR? don't even bind/unbind to the delegate
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController && HasAuthority())
		{
			if (bShouldBind && !BlasterOwnerController->HighPingDelegate.IsBound()) // only bind fn to the delgate on the server where there are no other fns are bound to it.
			{
				BlasterOwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingToohigh);
			}

			if (!bShouldBind && BlasterOwnerController->HighPingDelegate.IsBound())
			{
				BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingToohigh);
			}
		}
	}
}

void AWeapon::OnPingToohigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState); //repilcate the weapon state enum
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly); //only matters on the server and the locally controlled client
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//show pickup widget on weapon only when the actor overlapping with its area sphere is a blaster character.
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this); //C7_5
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//C7_11
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::ShowPickupWidget(bool bPickupWidget) //C7_7
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bPickupWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false); //play the fire animation on the weapon skeletal mesh.
	}
	if (CasingClass)
	{
		//we need a location to spawn our Casing from. use the Shell socket on the weapon mesh.
		const USkeletalMeshSocket* AmmoEject = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEject)
		{
			FTransform SocketTransform = AmmoEject->GetSocketTransform(WeaponMesh);
			//add random rotation to the bullet shell when ejected.
			FRotator SocketRotation = SocketTransform.GetRotation().Rotator();
			SocketRotation.Pitch += FMath::RandRange(0.f, 45.f);
			SocketRotation.Yaw += FMath::RandRange(0.f, 30.f);
			GetWorld()->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketRotation);
		}
	}

	SpendRound(); //Client-side predicting ammo
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped); //replicated

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	/*a weapon shouldn't store info about a player that dropped it*/
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

/**
*   SCATTER
*/

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (!MuzzleFlashSocket) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	const FVector RandomPointInSphere = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0, SphereRadius);
	const FVector EndLocation = SphereCenter + RandomPointInSphere;
	const FVector ToEnd = EndLocation - TraceStart;
	
	return FVector(TraceStart + ToEnd);
}

/*
*  Weapon State
*/

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State; //this will be only be set by the server, but will replicate.

	OnWeaponStateSet();
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:

		HandlePrimaryWeaponEquipped();
		break;

	case EWeaponState::EWS_EquippedSeconadry:
		
		HandleSecondaryWeaponEquipped();
		break;

	case EWeaponState::EWS_Dropped:

		HandleWeaponDropped();
		break;
	}
}

void AWeapon::HandlePrimaryWeaponEquipped()
{
	ShowPickupWidget(false); //once the character eqip the weapon, we should hide the pickup widget.(the widget doesn't get hidden, need to replicate weapon state)
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWeaponMeshPhysics(false);
	EnablePhysicsSMG(true);

	EnableCustomDepth(false);

	BindToHighPingDelegate(true);	//check if we should enable/disable SSR for this weapon based on the ping
}

void AWeapon::HandleSecondaryWeaponEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWeaponMeshPhysics(false);
	EnablePhysicsSMG(true);

	if (WeaponMesh)
	{
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();
	}

	BindToHighPingDelegate(false);
}

void AWeapon::HandleWeaponDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	SetWeaponMeshPhysics(true);
	EnablePhysicsSMG(false);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	BindToHighPingDelegate(false);
}

/*
*  Mesh Physics
*/

void AWeapon::SetWeaponMeshPhysics(bool bEnable)
{
	WeaponMesh->SetSimulatePhysics(bEnable);
	WeaponMesh->SetEnableGravity(bEnable);
	if (bEnable)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWeapon::EnablePhysicsSMG(bool bEnable)
{
	/*Enabeling physics on SMGs in Equipped state to allow their straps to move*/
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		if (bEnable)	//Equipped State
		{
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		}
		else			//Dropped State
		{
			WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block); //when dropped it can bounce off walls for ex.
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore); //ignore any pawns (they could step on it)
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
		}
	}
}

/*
*  AMMO
*/

void AWeapon::SpendRound()
{
	//called on server and client;
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);

	//apply client side prediction / Server Reconciliation
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else if(BlasterOwnerCharacter && BlasterOwnerCharacter->IsLocallyControlled())
	{
		++Sequence;
	}
	SetHUDAmmo();
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	//Server Reconciliation Algorithm
	--Sequence;
	Ammo = ServerAmmo - Sequence;
}

void AWeapon::AddAmmo(int32 AmmoAmount)
{
	Ammo = FMath::Clamp(Ammo + AmmoAmount, 0, GetMagCapacity());
	SetHUDAmmo();

	//only called on the server.. send to client to update the ammo
	ClientAddAmmo(Ammo);
}

void AWeapon::ClientAddAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;

	Ammo = ServerAmmo;

	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombatComponent() && IsFull())
	{
		BlasterOwnerCharacter->GetCombatComponent()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	/*need to set the ammo hud when the weapon is just equipped, we can't set it if the weapon's owner isn't set yet*/
	/*in combat component, once the the weapon is equipped on the server(or dropped), we set its owner, owner is replicated by default in actor class*/
	/*once the owner is set on the server and is replicated, this is a good time to set the HUD ammo for all clients*/
	Super::OnRep_Owner();
	if (Owner)
	{
		//don't let picking a secondary weapon change the HUD ammo for the equipped weapon<
		BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo();
		}
	}
	else //owner just got set to nullptr on the server ( drop() )
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

/*
*  Cosmetics
*/

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}

}

void AWeapon::PlayEquipSound()
{
	if (EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), EquipSound, GetActorLocation());
	}
}