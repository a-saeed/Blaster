// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	
	//set the projectile to replicate(i.e. it's spawned on the server, and the act of spawning propagates to clients, and the server maintains authority)
	bReplicates = true;
	//setup collision box, set it as root and configure its collision settings
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); //set to wolrd dynamic as a projectile will be moving across the world (not static)
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); //projectile will be used to generate overlap and hit events
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); //ignore all channels, we'll specify our custom channels we want this projectile to overlap/generate hit events with.
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility,ECollisionResponse::ECR_Block);  //block the visibility channel.
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic,ECollisionResponse::ECR_Block); //block (can hit) world static objects
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block); //block the custom-made skeletal mesh channel to be be able to hit character's skeletal mesh and not the capsule.
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnBulletTracer();

	//bind custom-made OnHit fn to the collisionBox HitEvent(but only on the server)
	if (HasAuthority()) //this check isn't meaningless since projectile class is set to replicate
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit); //it only generates on the server, we won't get hit events on clients.
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
/*
* HIT EVENT
*/
void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	SetImpactSurfaceEffects(OtherActor);

	Destroy();
}

void AProjectile::SetImpactSurfaceEffects(AActor* OtherActor)
{
	if (OtherActor->Implements<UInteractWithCrosshairsInterface>())
	{
		ParticlesToPlay = CharacterImpactParticles;
		SoundToPlay = CharacterImpactSound;
	}
	else
	{
		ParticlesToPlay = ImpactParticles;
		SoundToPlay = ImpactSound;
	}
}

void AProjectile::Destroyed()
{
	//on hit, play FX
	if (ParticlesToPlay)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticlesToPlay, GetActorTransform());
	}

	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundToPlay, GetActorLocation());
	}

	Super::Destroyed();
}
/*
*
* Common Functionalities for Rokects and Grenade Launchers
*
*/
void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false);
	}
}

void AProjectile::SpawnBulletTracer()
{
	//attach tracer particle system to bullet once spawned
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::ExplodeDamageWithFX()
{
	/*
	* -Apply radial damage
	* -Spawn Hit Effects
	*/

	/*Get Instigator controller from Instigator pawn in order to apply damage*/
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				GetWorld(),
				Damage,											 //BaseDamage
				Damage / 4,										 //MinDamage
				GetActorLocation(),								 //OriginOfDamage
				DamageInnerRadius,								 //DamageInnerRadius
				DamageOuterRadius,								 //DamageOuterRadius
				1.f,											 //DamageFallof (Linear)	
				UDamageType::StaticClass(),						 //DamageType
				TArray<AActor*>(),								 //IgnoredActors
				this,											 //DamageCauser (this projectile)
				FiringController);								 //InstigatorController
		}
	}

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
	}
}
