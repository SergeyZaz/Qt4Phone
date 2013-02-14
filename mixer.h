#pragma once

#include <iostream>
#include "windows.h"
#pragma comment(lib,"Winmm")

enum SoundDirections {
	Recorder,
	Player
};

double GetVolumeX(SoundDirections d, int Component, int dev=0);
int SetVolumeX(SoundDirections d, int Component, double volume, int dev=0);
