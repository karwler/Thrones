#pragma once

#include "utils/utils.h"

class AudioSys {
private:
	umap<string, Sound> sounds;
	SDL_AudioSpec aspec;
	uint8* apos;
	uint32 alen = 0;
	SDL_AudioDeviceID device;
	const uint8& volume;

public:
	AudioSys(const uint8& avolume);
	~AudioSys();

	void play(const string& name);

private:
	static void SDLCALL callback(void* udata, uint8* stream, int len);
};
