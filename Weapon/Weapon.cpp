#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Casing.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; //for weapon to have authority only on the server, we nned to set it to replicate. (the serve will be in charge of all weapon objects)

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

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State; //this will be only be set by the server, but will replicate.

	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:

		ShowPickupWidget(false); //once the character eqip the weapon, we should hide the pickup widget.(the widget doesn't get hidden, need to replicate weapon state)
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
	
}

void AWeapon::OnRep_WeaponState()
{
	/*
	*   switch (WeaponState)
		{
		case EWeaponState::EWS_Equipped:
			ShowPickupWidget(false);
			break;
		}
	*/
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
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			FTransform SocketTransform = AmmoEject->GetSocketTransform(WeaponMesh);
			GetWorld()->SpawnActor<ACasing>(CasingClass, SocketTransform);
		}
	}
}
