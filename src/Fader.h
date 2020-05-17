#pragma once
#include "Structs.h"

class Fader
{
public:
	Fader();
	~Fader();
	FadeStatus status = FadeStatus::initial;

	float fadeVal = 0;
	void process(double time)
	{
		if (status == FadeStatus::initial)
		{
			fadeVal = 0;
			status = FadeStatus::fading;
		}

		fadeVal += (float) time *3;

 //disables fader entirely
		 if (fadeVal >= 1.0)
		{
			fadeVal = 1.0;
			status = FadeStatus::complete;
		}
	}
	float getAlpha()
	{
		if (isVisible() == false)
			return 0;
		if (status == FadeStatus::initial)
			return 1.0;
		return 1.0f - fadeVal;

	}
	bool isVisible()
	{
		return (status != FadeStatus::complete);
	}

	void reset()
	{
		status = FadeStatus::initial;
		fadeVal = 0;
	}

};

