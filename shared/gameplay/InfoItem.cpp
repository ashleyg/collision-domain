/**
* @file		Gameplay.cpp
* @brief	Manages the view ports for a window
*/

/*-------------------- INCLUDES --------------------*/
#include "stdafx.h"
#include "GameIncludes.h"
#include <sstream>
#include <math.h>
#include "InfoItem.h"
#include "BitStream.h"
InfoItem::InfoItem(OverlayType ot, RakNet::Time startTime, RakNet::Time endTime)
{
	mOT			= ot;
	mStartTime	= startTime;
	mEndTime	= endTime;
	mDrawn		= false;
	this->sendPacket();
}

InfoItem::InfoItem(OverlayType ot, RakNet::Time startTime, int seconds)
{
	mOT			 = ot;
	mStartTime	 = startTime;
	mEndTime	 = startTime + RakNet::Time(seconds);
	mDrawn		= false;
	this->sendPacket();
}

InfoItem::InfoItem(OverlayType ot, int delay, int seconds)
{
	mOT		   = ot;
	mStartTime = RakNet::GetTime() + RakNet::Time(delay);
	mEndTime   = mStartTime + RakNet::Time(seconds);
	mDrawn		= false;
	this->sendPacket();
}

void InfoItem::sendPacket()
{
	RakNet::BitStream bs;
    bs.Write((char*)this,sizeof(this));
	//GameCore::mNetworkCore->InfoItemTransmit(bs);
}

RakNet::Time InfoItem::getStartTime()
{
	return mStartTime;
}

RakNet::Time InfoItem::getEndTime()
{
	return mEndTime;
}

OverlayType	InfoItem::getOverlayType()
{
	return mOT;
}

void InfoItem::setDrawn()
{
	mDrawn = true;
}

bool InfoItem::getDrawn()
{
	return mDrawn;
}