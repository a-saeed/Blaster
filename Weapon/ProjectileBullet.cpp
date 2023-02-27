// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include"GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileBullet::BeginPlay()
{
	AActor::BeginPlay();

	SpawnBulletTracer();

	StartDestroyTimer();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileBullet::OnBounce);
}

void AProjectileBullet::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	SetImpactSurfaceEffects(ImpactResult.GetActor());

	ACharacter* TargetCharacter = Cast<ACharacter>(ImpactResult.GetActor());
	if (TargetCharacter)
	{
		ProjectileMovementComponent->SetVelocityInLocalSpace(FVector().ZeroVector);
		if (HasAuthority())
		{
			ApplyPointDamage(TargetCharacter);
		}
	}
	
}

void AProjectileBullet::ApplyPointDamage(AActor* Target)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (OwnerCharacter != Target)
		{
			AController* OwnerController = OwnerCharacter->Controller;
			if (OwnerController)
			{
				UGameplayStatics::ApplyDamage(Target, Damage, OwnerController, this, UDamageType::StaticClass());
			}
		}
	}

	Destroy();
}

/*
void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
if (OwnerCharacter)
{
	AController* OwnerController = OwnerCharacter->Controller;
	if (OwnerController)
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
	}
}

//the Super version has a call to Destroy. keep super version last.
Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
*/