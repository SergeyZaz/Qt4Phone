#include "mixer.h"

HMIXER mixerInit(SoundDirections d, int Component, int dev, int &cChannels, int &dwControlID)
{
	HMIXER hMixer;
	HRESULT hr;		
	hr = mixerOpen(&hMixer, 0, 0, 0, (d==Player) ? MIXER_OBJECTF_WAVEOUT : MIXER_OBJECTF_WAVEIN);
	if ( FAILED( hr ) ) return 0;

	MIXERLINE mxl;
	memset(&mxl, 0, sizeof(MIXERLINE));
	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = Component;
	hr = mixerGetLineInfo((HMIXEROBJ)hMixer, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

	if (FAILED(hr) || mxl.cControls==0)
	{
		mixerClose(hMixer);
		return 0;
	}

    MIXERCONTROL mc;
	MIXERLINECONTROLS mxlc;

	memset(&mxlc, 0, sizeof(MIXERLINECONTROLS));
	memset(&mc, 0, sizeof(MIXERCONTROL));

	mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
	mxlc.dwLineID = mxl.dwLineID;
	mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mxlc.cControls = 1;
	mxlc.pamxctrl = &mc;
	mxlc.cbmxctrl = sizeof(MIXERCONTROL);

	hr = mixerGetLineControls((HMIXEROBJ)hMixer, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);
	if(FAILED(hr))
	{
		mixerClose(hMixer);
		return 0;
	}

	if(mxl.cChannels==0)
	{
		mixerClose(hMixer);
		return 0;
	}

	cChannels = mxl.cChannels;
	dwControlID = mc.dwControlID;
	return hMixer;
}

double GetVolumeX(SoundDirections d, int Component, int dev)
{
	int cChannels, dwControlID;
	HMIXER hMixer = mixerInit(d, Component, dev, cChannels, dwControlID);
	if(!hMixer)
	{
/*		if(d==Recorder)
		{
			hMixer = mixerInit(Player, Component, dev, cChannels, dwControlID);
			if(!hMixer)
				return -1;
		}
		else
*/			return -1;
	}

	// getting value
	MIXERCONTROLDETAILS mxcd;
	memset(&mxcd, 0, sizeof(MIXERCONTROLDETAILS));
	mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mxcd.cChannels = cChannels;
	mxcd.dwControlID = dwControlID;
	
	MIXERCONTROLDETAILS_UNSIGNED mxdu[2];
	memset(mxdu, 0, sizeof(MIXERCONTROLDETAILS_UNSIGNED)*2);
	mxdu[0].dwValue = -1; 
	mxdu[1].dwValue = -1; 
	mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED)*mxcd.cChannels;
	mxcd.paDetails = mxdu;

	if( mixerGetControlDetails((HMIXEROBJ)hMixer, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
	{
		mixerClose(hMixer);
		return -1;
	}

	mixerClose(hMixer);

	double val = -1;
	if(mxcd.cChannels==1)
		val = (double)mxdu[0].dwValue/65535.0;
	if(mxcd.cChannels==2)
		val = (double)(mxdu[0].dwValue+mxdu[1].dwValue)/2.0/65535.0;
	return val;
}

int SetVolumeX(SoundDirections d, int Component, double volume, int dev)
{
	int cChannels, dwControlID;
	HMIXER hMixer = mixerInit(d, Component, dev, cChannels, dwControlID);
	if(!hMixer)
	{
/*		if(d==Recorder)
		{
			hMixer = mixerInit(Player, Component, dev, cChannels, dwControlID);
			if(!hMixer)
				return -1;
		}
		else
*/			return -1;
	}

	// getting value
	MIXERCONTROLDETAILS mxcd;
	memset(&mxcd, 0, sizeof(MIXERCONTROLDETAILS));
	mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mxcd.cChannels = cChannels;
	mxcd.dwControlID = dwControlID;
	
	MIXERCONTROLDETAILS_UNSIGNED mxdu[2];
	mxdu[0].dwValue = (int)(volume*65535.0);//(mc.Bounds.dwMaximum - mc.Bounds.dwMinimum));
	mxdu[1].dwValue = mxdu[0].dwValue;
	mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED) * mxcd.cChannels;
	mxcd.paDetails = mxdu;

	if(mixerSetControlDetails((HMIXEROBJ)hMixer, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
	{
		mixerClose(hMixer);
		return -1;
	}

	mixerClose(hMixer);
	return 0;
}
