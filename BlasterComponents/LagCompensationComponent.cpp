// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Blaster.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	PopulateFrameHistory();
}

void ULagCompensationComponent::PopulateFrameHistory()
{
	if (!BlasterCharacter || !BlasterCharacter->HasAuthority()) return;		//only savce frame packages on the server

	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float NewestFrameTime = FrameHistory.GetHead()->GetValue().Time;
		float OldestFrameTime = FrameHistory.GetTail()->GetValue().Time;
		float HistoryTime = NewestFrameTime - OldestFrameTime;

		while (HistoryTime > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());

			NewestFrameTime = FrameHistory.GetHead()->GetValue().Time;
			OldestFrameTime = FrameHistory.GetTail()->GetValue().Time;
			HistoryTime = NewestFrameTime - OldestFrameTime;
		}

		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);

		//DrawFramePackage(ThisFrame);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& OutPackage)
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		OutPackage.Time = GetWorld()->GetTimeSeconds();	//the server time.. since we're only using lag compensation on the server
		OutPackage.Character = BlasterCharacter;

		for (auto& BoxPair : BlasterCharacter->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			if (BoxPair.Value)
			{
				BoxInformation.Location = BoxPair.Value->GetComponentLocation();
				BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
				BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

				OutPackage.HitBoxMap.Add(BoxPair.Key, BoxInformation);
			}
		}
	}
}

void ULagCompensationComponent::DrawFramePackage(const FFramePackage& Package)
{
	for (auto& BoxInfo : Package.HitBoxMap)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			FColor::Orange,
			false,
			2.5f);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	if (!FrameToCheck.Character)	//case when client is too laggy to do SSR
	{
		return FServerSideRewindResult();
	}

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, const float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	
	if (!FramesToCheck[0].Character)	//case when client is too laggy to do SSR
	{
		return FShotgunServerSideRewindResult();
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100 InitialVelocity, const float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	if (!FrameToCheck.Character)	//case when client is too laggy to do SSR
	{
		return FServerSideRewindResult();
	}

	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		!HitCharacter ||
		!HitCharacter->GetLagCompensation() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetHead() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetTail();

	if (bReturn) return FFramePackage();

	//Frame Pcakage that we check to verify a hit.
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;
	//Frame History for the character that got hit
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;

	float OldestHistoryTime = History.GetTail()->GetValue().Time;
	float NewestHistoryTime = History.GetHead()->GetValue().Time;

	//Too far back.. Too laggy to do SSR
	if (HitTime < OldestHistoryTime)
	{
		return FFramePackage();
	}
	//Oldest History time is at the tail
	if (HitTime == OldestHistoryTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	//Oldest History time is at the head
	if (HitTime >= NewestHistoryTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	//March back until OlderTime < HitTime < YoungerTime
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* YoungerIterator;
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* OlderIterator = History.GetHead();

	while (OlderIterator->GetValue().Time > HitTime)
	{
		if (!OlderIterator->GetNextNode()) break;

		OlderIterator = OlderIterator->GetNextNode();
	} //exit loop with OlderIterator just one frame behind the frame that contains HitTime

	if (OlderIterator->GetValue().Time == HitTime)	//highly unlikely, but we found our frame to check.
	{
		FrameToCheck = OlderIterator->GetValue();
		bShouldInterpolate = false;
	}
	YoungerIterator = OlderIterator->GetPrevNode();

	if (bShouldInterpolate)
	{
		FrameToCheck = InterpBetweenFrames(OlderIterator->GetValue(), YoungerIterator->GetValue(), HitTime);
	}

	FrameToCheck.Character = HitCharacter;	//we must set the character for the Hitframe otherwise we won't have any character to apply damage to to apply damage
	return FrameToCheck;
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;	//the frame corrseponding to HitTime that we want to fill and return
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxMap)
	{
		const FName& BoxInfoName = YoungerPair.Key;

		//we need to access the Boxinfo for the older frame as well to interp.. we can do so since we have the box name(that's why we made a map with a name as a key in the first place)
		const FBoxInformation& OlderBoxInfo = OlderFrame.HitBoxMap[BoxInfoName];
		const FBoxInformation& YoungerBoxInfo = YoungerFrame.HitBoxMap[BoxInfoName];

		FBoxInformation InterpBoxInfo;		//the box info that is used to fill the InterpFramePackage we want to return

		InterpBoxInfo.Location = FMath::VInterpTo(OlderBoxInfo.Location, YoungerBoxInfo.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBoxInfo.Rotation, YoungerBoxInfo.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBoxInfo.BoxExtent;		//can use olderBoxInfo since box exten doesn't change anyway.

		InterpFramePackage.HitBoxMap.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& HitFramePackage, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (!HitCharacter) return FServerSideRewindResult();

	//disable collision for the Hitchracter's mesh
	EnableCharacterMeshCollision(HitCharacter ,ECollisionEnabled::NoCollision);

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, HitFramePackage);

	//Enable collision for the head first
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;		//Extend the end to trace THROUGH the object not just the surface

	GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);

	if (ConfirmHitResult.bBlockingHit)      //we hit the head; return early
	{
		ResetBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
		return FServerSideRewindResult{true, true};
	}
	else	//didn't hit the head, check the rest of the boxes
	{
		for (auto& BoxPair : HitCharacter->HitCollisionBoxes)
		{
			if (BoxPair.Value)
			{
				BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				BoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
		if (ConfirmHitResult.bBlockingHit)      
		{
			ResetBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
			return FServerSideRewindResult{ true, false };
		}
	}

	ResetBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& HitFramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	/*
	* we could do all operations in the same loop;
	* but to keep it tidy it's better to create multiple..
	* essentially having the same time complexity.
	*/

	/*Return early if any character within a frame is null.*/
	for (auto& HitFramePackage : HitFramePackages)
	{
		if (!HitFramePackage.Character) return FShotgunServerSideRewindResult();
	}

	/*for each character, Cache and rewind the boxes to their original pose in HitFrame.*/
	FShotgunServerSideRewindResult ShotgunResult;		//returned value
	TArray<FFramePackage> CurrentFrames;				//Array to hold cached frames to reset them later

	for (auto& HitFramePackage : HitFramePackages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = HitFramePackage.Character;		//set this frame's character.. need to access it in the last loop in this fn.

		EnableCharacterMeshCollision(HitFramePackage.Character, ECollisionEnabled::NoCollision);
		CacheBoxPositions(HitFramePackage.Character, CurrentFrame);
		MoveBoxes(HitFramePackage.Character, HitFramePackage);

		CurrentFrames.Add(CurrentFrame);
	}

	/*Enable collision for the head first.*/
	for (auto& HitFramePackage : HitFramePackages)
	{
		UBoxComponent* HeadBox = HitFramePackage.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	/*For each HitLoaction (Shotgun has multiple pellets), we do a line trace*/
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
		//did we hit a character?
		ABlasterCharacter* Character = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
		if (Character)
		{
			if (ShotgunResult.HeadShotsMap.Contains(Character))
			{
				ShotgunResult.HeadShotsMap[Character]++;
			}
			else
			{
				ShotgunResult.HeadShotsMap.Add(Character, 1);
			}
		}
	}

	/*Enable collision for all body parts.. but disable it for the head; since we already populated the headShots map and don't want to add head collisions in the bodyParts map.*/
	for (auto& HitFramePackage : HitFramePackages) //for each frame.. i.e. for each hit character
	{
		for (auto& BoxPair : HitFramePackage.Character->HitCollisionBoxes) //for each collision box map in this character
		{
			if (BoxPair.Value)
			{
				BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				BoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
		//disable head collision
		UBoxComponent* HeadBox = HitFramePackage.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	/*we do a line trace one more time for the rest of the body*/
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		GetWorld()->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
		//did we hit a character?
		ABlasterCharacter* Character = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
		if (Character)
		{
			if (ShotgunResult.BodyShotsMap.Contains(Character))
			{
				ShotgunResult.BodyShotsMap[Character]++;
			}
			else
			{
				ShotgunResult.BodyShotsMap.Add(Character, 1);
			}
		}
	}

	/*Reset boxes and Re-enable character mesh collision */
	for (auto& OriginalFrame : CurrentFrames)
	{
		ResetBoxes(OriginalFrame.Character, OriginalFrame);
		EnableCharacterMeshCollision(OriginalFrame.Character, ECollisionEnabled::QueryOnly);
	}

	return ShotgunResult;
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& HitFramePackage, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100 InitialVelocity, const float HitTime)
{
	if (!HitCharacter) return FServerSideRewindResult();

	//disable collision for the Hitchracter's mesh
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, HitFramePackage);

	//Enable collision for the head first
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	//instead of a line trace..we predict projectile path
	FPredictProjectilePathParams PathParams;

	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;		//we never need to predict a path beyond the sever max record time
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;			//bullet won't be larger than a sphere of radius 5.f
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());
	PathParams.OverrideGravityZ = 1;

	FPredictProjectilePathResult PathResult;

	UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, PathResult);	// predict path.. the equivalent fo a line trace in hitscan weapons

	if (PathResult.HitResult.bBlockingHit)	//we hit the head; return early
	{
		ResetBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
		return FServerSideRewindResult{ true, true };
	}
	else         //didn't hit the head, check the rest of the boxes
	{
		for (auto& BoxPair : HitCharacter->HitCollisionBoxes)
		{
			if (BoxPair.Value)
			{
				BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				BoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, PathResult);	//predict path..
		if (PathResult.HitResult.bBlockingHit)
		{
			ResetBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
			return FServerSideRewindResult{ true, false };
		}

	}

	ResetBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryOnly);
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (!HitCharacter) return;

	for (auto& BoxPair : HitCharacter->HitCollisionBoxes)
	{
		FBoxInformation BoxInformation;
		if (BoxPair.Value)
		{
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			OutFramePackage.HitBoxMap.Add(BoxPair.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& DestinationFramePackage)
{
	/*
	* Move boxes to the position of the hit
	*/

	if (!HitCharacter) return;

	for (auto& BoxPair : HitCharacter->HitCollisionBoxes)
	{
		const FName& BoxName = BoxPair.Key;
		const FBoxInformation& DestinationBox = DestinationFramePackage.HitBoxMap[BoxName];

		if (BoxPair.Value)
		{
			BoxPair.Value->SetWorldLocation(DestinationBox.Location);
			BoxPair.Value->SetWorldRotation(DestinationBox.Rotation);
			BoxPair.Value->SetBoxExtent(DestinationBox.BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& DestinationFramePackage)
{
	/*
	* Restore boxes to their original location + disable their collision
	*/

	if (!HitCharacter) return;

	for (auto& BoxPair : HitCharacter->HitCollisionBoxes)
	{
		const FName& BoxName = BoxPair.Key;
		const FBoxInformation& DestinationBox = DestinationFramePackage.HitBoxMap[BoxName];

		if (BoxPair.Value)
		{
			BoxPair.Value->SetWorldLocation(DestinationBox.Location);
			BoxPair.Value->SetWorldRotation(DestinationBox.Rotation);
			BoxPair.Value->SetBoxExtent(DestinationBox.BoxExtent);

			BoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

/*
* The RPC that will be used by other classes to request server side rewind
*/

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult RewindResult = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (BlasterCharacter && HitCharacter && BlasterCharacter->GetEquippedWeapon() && RewindResult.bHitConfirmed)
	{
		const float DamageToCause = RewindResult.bHeadShot ? BlasterCharacter->GetEquippedWeapon()->GetHeadShotDamage() : BlasterCharacter->GetEquippedWeapon()->GetDamage();
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageToCause,
			BlasterCharacter->GetController(),		//the controller of the shooter charcter
			BlasterCharacter->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ServerShotgunScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult ShotgunResult = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (auto& HitCharacter : HitCharacters)
	{
		if (!HitCharacter || !BlasterCharacter || !BlasterCharacter->GetEquippedWeapon()) continue;		//notice the diff between HitCharacter and BlasterCharacter

		float TotalDamage = 0.f;

		if (ShotgunResult.HeadShotsMap.Contains(HitCharacter))
		{
			float HeadShotDamage = ShotgunResult.HeadShotsMap[HitCharacter] * BlasterCharacter->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}

		if (ShotgunResult.BodyShotsMap.Contains(HitCharacter))
		{
			float BodyShotDamage = ShotgunResult.BodyShotsMap[HitCharacter] * BlasterCharacter->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			BlasterCharacter->GetController(),		//the controller of the shooter charcter
			BlasterCharacter->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ServerProjectileScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100 InitialVelocity, float HitTime)
{
	FServerSideRewindResult RewindResult = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (BlasterCharacter && BlasterCharacter->GetEquippedWeapon() && HitCharacter && RewindResult.bHitConfirmed)
	{
		const float DamageToCause = RewindResult.bHeadShot ? BlasterCharacter->GetEquippedWeapon()->GetHeadShotDamage() : BlasterCharacter->GetEquippedWeapon()->GetDamage();
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageToCause,
			BlasterCharacter->GetController(),		//the controller of the shooter charcter
			BlasterCharacter->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}