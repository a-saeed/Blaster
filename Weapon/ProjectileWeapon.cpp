// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectileWeapon::Fire(const FVector& HitTarget) //hit target is the impact point of the line trace done in combat comp.
{
	Super::Fire(HitTarget);
	
	//we can play animations of the weapon firing on all machines
	//but the act of spawning a projectile should only happen on the server
	//has authority checks if this weapon is spawned on the server or not
	//since its parent weapon class is set to replicate,then so does projectile weapon
	//after this check we know that projectile weapon has authority on the server only. and the projectile will only spawn on the server. lec 76

	//we need a location to spawn our projectile from. use the muzzle socket on the weapon mesh.
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());

	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation(); //rotation of the projectile so that it faces tha hit target.

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner(); //the owner of this weapon class(i.e. the character).
		SpawnParams.Instigator = InstigatorPawn; //will be used to grant credit fro elemination.

		/*
		* Handle cases of when and when not to use Server side rewind
		*/

		AProjectile* SpawnedProjectile = nullptr;

		if (bUseServerSideRewind)	//weapon uses SSR
		{
			if (InstigatorPawn->HasAuthority())	// server character
			{
				if (InstigatorPawn->IsLocallyControlled())	// server, locally controlled - spawn replicated projectile / No SSR
				{
					SpawnedProjectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;		// projectile won't send a server score request
					SpawnedProjectile->Damage = Damage;						// set projectile damage to the damage of its weapon
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else										// server, client controlled - spawn non-replicated projectile / use SSR
				{
					SpawnedProjectile = GetWorld()->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;			// SSR bullets shoulddn't cause damage on the server
				}
			}
			else	// client character, using SSR
			{
				if (InstigatorPawn->IsLocallyControlled())	// client, locally controlled - spawn non-replicated projectile / use SSR
				{
					SpawnedProjectile = GetWorld()->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;			//projectile will send a server score request
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					SpawnedProjectile->Damage = Damage;						// Seeting Damage here is unnecssary as this is a client controlled character.. the server checks for weapon damage in SSR function. but anyways
				}
				else										// client, client controlled - spawn non-replicated projectile / No SSR
				{
					SpawnedProjectile = GetWorld()->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else    // weapon doesn't use SSR
		{
			if (InstigatorPawn->HasAuthority())	// server character, all server characters will spawn replicated projectiles that clients must wait for it to reach them.. the original case before using SSR
			{
				SpawnedProjectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;		
				SpawnedProjectile->Damage = Damage;
				SpawnedProjectile->HeadShotDamage = HeadShotDamage;
			}
		}
	}
}