#pragma once

#include "oven/oven.h"

class AudioSys {
private:
	SDL_AudioDeviceID device;
	SDL_AudioSpec aspec;
	umap<string, Sound> sounds;
	uint8* apos;
	uint32 alen;

public:
	AudioSys();
	~AudioSys();

	void play(const string& name);

private:
	static void callback(void* udata, uint8* stream, int len);
};
