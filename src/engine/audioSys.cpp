#include "audioSys.h"
#include "fileSys.h"
#include <iostream>

AudioSys::AudioSys(const uint8& avolume) :
	volume(avolume)
{
	SDL_AudioSpec spec = Sound::defaultSpec;
	spec.callback = callback;
	spec.userdata = this;
	if (device = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &spec, &aspec, SDL_AUDIO_ALLOW_ANY_CHANGE); !device)
		throw std::runtime_error("failed to initialize audio system");
	if (aspec.freq != spec.freq)
		std::cout << "audio frequency changed from " << spec.freq << " to " << aspec.freq << std::endl;
	if (aspec.format != spec.format)
		std::cout << "audio format changed from " << std::hex << std::uppercase << spec.format << " to " << aspec.format << std::nouppercase << std::dec << std::endl;
	if (aspec.channels != spec.channels)
		std::cout << "audio channels changed from " << spec.channels << " to " << aspec.channels << std::endl;
	sounds = FileSys::loadAudios(aspec);
}

AudioSys::~AudioSys() {
	SDL_CloseAudioDevice(device);
	for (auto& [name, sound] : sounds)
		sound.free();
}

void AudioSys::play(const string& name) {
	if (!volume)
		return;
	umap<string, Sound>::iterator it = sounds.find(name);
	if (it == sounds.end())
		return;

	apos = it->second.data;
	alen = it->second.length;
	SDL_PauseAudioDevice(device, SDL_FALSE);
}

void AudioSys::callback(void* udata, uint8* stream, int len) {
	AudioSys* as = static_cast<AudioSys*>(udata);
	SDL_memset(stream, 0, uint(len));
	if (uint32(len) > as->alen) {
		if (!as->alen) {
			SDL_PauseAudioDevice(as->device, SDL_TRUE);
			return;
		}
		len = int(as->alen);
	}
	SDL_MixAudioFormat(stream, as->apos, as->aspec.format, uint32(len), as->volume);
	as->apos += len;
	as->alen -= uint32(len);
}
