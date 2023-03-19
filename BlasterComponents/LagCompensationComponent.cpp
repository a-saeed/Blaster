// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage FramePackage;
	SaveFramePackage(FramePackage);
	DrawFramePackage(FramePackage);
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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

		DrawFramePackage(ThisFrame);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& OutPackage)
{
	OutPackage.Time = GetWorld()->GetTimeSeconds();	//the server time.. since we're only using lag compensation on the server

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		for (auto& BoxPair : BlasterCharacter->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			OutPackage.HitBoxMap.Add(BoxPair.Key, BoxInformation);
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

void ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, FVector_NetQuantize& TraceStart, FVector_NetQuantize& HitLocation, float HitTime)
{
	bool bReturn =
		!HitCharacter ||
		!HitCharacter->GetLagCompensation() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetHead() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetTail();

	if (bReturn) return;

	//Frame Pcakage that we check to verify a hit.
	FFramePackage FrametoCheck;
	bool bShouldInterpolate = true;
	//Frame History for the character that got hit
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;

	float OldestHistoryTime = History.GetTail()->GetValue().Time;
	float NewestHistoryTime = History.GetHead()->GetValue().Time;

	//Too far back.. Too laggy to do SSR
	if (HitTime < OldestHistoryTime)
	{
		return;
	}
	//Oldest History time is at the tail
	if (HitTime == OldestHistoryTime)
	{
		FrametoCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	//Oldest History time is at the head
	if (HitTime >= NewestHistoryTime)
	{
		FrametoCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	//March back until OlderTime < HitTime < YoungerTime
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* YoungerIterator;
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* OlderIterator = FrameHistory.GetHead();

	while (OlderIterator->GetValue().Time > HitTime)
	{
		if (!OlderIterator->GetNextNode()) break;

		OlderIterator = OlderIterator->GetNextNode();
	} //exit loop with OlderIterator just one frame behind the frame that contains HitTime

	if (OlderIterator->GetValue().Time == HitTime)	//highly unlikely, but we found our frame to check.
	{
		FrametoCheck = OlderIterator->GetValue();
		bShouldInterpolate = false;
	}
	YoungerIterator = OlderIterator->GetPrevNode();

	if (bShouldInterpolate)
	{
		//Interpolate between younger and older
	}
}
