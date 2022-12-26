// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

AProjectileWeapon::AProjectileWeapon()
{
}

void AProjectileWeapon::BeginPlay()
{
	Super::BeginPlay();

	SetWeaponFiringRange(40000);
}

void AProjectileWeapon::Fire(const FVector& HitTarget) //hit target is the impact point of the line trace done in combat comp.
{
	Super::Fire(HitTarget);
	
	FActorSpawnParameters SpawnParams;

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	SpawnParams.Owner = GetOwner(); //the owner of this weapon class(i.e. the character).
	SpawnParams.Instigator = InstigatorPawn; //will be used to grant credit fro elemination.

	//we need a location to spawn our projectile from. use the muzzle socket on the weapon mesh.
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		if (ProjectileClass && InstigatorPawn)
		{
			FVector ToTarget = HitTarget - SocketTransform.GetLocation();
			FRotator TargetRotation = ToTarget.Rotation(); //rotation of the projectile so that it faces tha hit target.

			GetWorld()->SpawnActor<AProjectile>(
				ProjectileClass,
				SocketTransform.GetLocation(),
				TargetRotation,
				SpawnParams);
		}
	}
}