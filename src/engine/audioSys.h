#pragma once

#include "oven/oven.h"

class AudioSys {
private:
	umap<string, Sound> sounds;
	SDL_AudioSpec aspec;
	uint8* apos;
	uint32 alen;
	SDL_AudioDeviceID device;

public:
	AudioSys(WindowSys* win);
	~AudioSys();

	void play(const string& name, uint8 volume);

private:
	static void callback(void* udata, uint8* stream, int len);
};
