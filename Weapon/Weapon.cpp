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

	if (HasAuthority()) //check if this machine is the server based on its network role (authority)
	{
		AreaSphere-> SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
	
	if (PickupWidget) //hide pickup widget at the beginning.
	{
		ShowPickupWidget(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState); //repilcate the weapon state enum
	DOREPLIFETIME(AWeapon, Ammo);
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

	SpendRound();
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

		HandleWeaponEquipped();
		break;

	case EWeaponState::EWS_Dropped:

		HandleWeaponDropped();
		break;
	}
}

void AWeapon::HandleWeaponEquipped()
{
	ShowPickupWidget(false); //once the character eqip the weapon, we should hide the pickup widget.(the widget doesn't get hidden, need to replicate weapon state)
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWeaponMeshPhysics(false);
	EnablePhysicsSMG(true);
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

void AWeapon::AddAmmo(int32 AmmoAmount)
{
	Ammo = FMath::Clamp(Ammo + AmmoAmount, 0, GetMagCapacity());

	SetHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombatComponent() && IsFull())
	{
		BlasterOwnerCharacter->GetCombatComponent()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity); //if ammo - 1 is in range, set.

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