#include "Weapon.h"
#include "Components/SphereComponent.h"

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

}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) //check if this machine is the server based on its network role (authority)
	{
		AreaSphere-> SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
