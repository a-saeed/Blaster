// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("RocketMesh")));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);							//the mesh is purely cosmetic.. coliosion  box is responsible for collision
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/*Get Instigator controller from Instigator pawn in order to apply damage*/
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
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

	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);                //Destoys projectile and play particle effects agter applying damage.
}
