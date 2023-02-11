// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()
	
private:
	/*
	* Warmup time
	*/
	UPROPERTY(meta = (BindWidget))
		class UTextBlock* WarmupTimeText;
	/*
	* Announcement text
	*/
	UPROPERTY(meta = (BindWidget))
		UTextBlock* AnnouncementText;
	/*
	* Info text
	*/
	UPROPERTY(meta = (BindWidget))
		UTextBlock* InfoText;
public:

	FORCEINLINE UTextBlock* GetAnnouncementText() const { return AnnouncementText; }
	FORCEINLINE UTextBlock* GetWarmupText() const { return WarmupTimeText; }
	FORCEINLINE UTextBlock* GetInfoText() const { return InfoText; }

};
