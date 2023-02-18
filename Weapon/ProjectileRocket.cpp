// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Blaster/Weapon/RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("RocketMesh")));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);							//the mesh is purely cosmetic.. coliosion  box is responsible for collision

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	/*Create Smoke trail*/
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
	/*Play Attached rocket sound and stop OnHit .. not when the projectile is destroyed.. since destruction is delayed*/
	if (RocketLoopingSound && RocketLoopingSoundAttenuation)
	{
		RocketLoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(RocketLoopingSound,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			RocketLoopingSoundAttenuation,
			(USoundConcurrency *)nullptr,
			false
		);
	}
	/*Allow all clients to register a hit event*/
	if (!HasAuthority()) 
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/*
	* -If Hit self, Return to not apply damage; Rocket launcher will continue moving to check for other hits
	* -Both Server and clients can now register hit events
	* -Deal Damagae only on the server
	* -Play FX
	* -Hide rocket mesh & Disable collision
	* -Deactivate the system instance stored in the niagara component(the one we created in begin play) to stop generating more particles
	* -Stop playing rocket sound
	* -Actually destroy the projectile after the smoke trail has finished
	*/

	if (OtherActor == GetOwner())
	{
		return;
	}
	/*Get Instigator controller from Instigator pawn in order to apply damage*/
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			 UGameplayStatics::ApplyRadialDamageWithFalloff(
				GetWorld(),				
				Damage,								//BaseDamage
				Damage / 4,							//MinDamage
				GetActorLocation(),					//OriginOfDamage
				200,								//DamageInnerRadius
				500,								//DamageOuterRadius
				1.f,								//DamageFallof (Linear)	
				UDamageType::StaticClass(),			//DamageType
				TArray<AActor*>(),					//IgnoredActors
				this,								//DamageCauser (this projectile)
				FiringController);					//InstigatorController
		}
	}

	if (ParticlesToPlay)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticlesToPlay, GetActorTransform());
	}
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundToPlay, GetActorLocation());
	}

	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}

	if (RocketLoopingSoundComponent && RocketLoopingSoundComponent->IsPlaying())
	{
		RocketLoopingSoundComponent->Stop();
	}
	/*Destroy the projectile after a timer to let smoke trail finish*/
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectileRocket::DestroyTimerFinished,
		DestroyTime);
}

void AProjectileRocket::DestroyTimerFinished()
{
	Destroy();
}
