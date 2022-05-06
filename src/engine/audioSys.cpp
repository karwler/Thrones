#include "audioSys.h"
#include "fileSys.h"

AudioSys::AudioSys(const uint8& avolume) :
	volume(avolume)
{
	SDL_AudioSpec spec{};	// TODO: adjust values
	spec.freq = 22050;
	spec.format = AUDIO_S16;
	spec.channels = 1;
	spec.samples = 4096;
	spec.callback = callback;
	spec.userdata = this;
	if (device = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &spec, &aspec, SDL_AUDIO_ALLOW_ANY_CHANGE); !device)
		throw std::runtime_error("failed to initialize audio system");
	if (aspec.freq != spec.freq)
		logInfo("audio frequency changed from ", spec.freq, " to ", aspec.freq);
	if (aspec.format != spec.format)
		logInfo("audio format changed from ", toStr<16>(spec.format), " to ", aspec.format);
	if (aspec.channels != spec.channels)
		logInfo("audio channels changed from ", spec.channels, " to ", aspec.channels);
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

void SDLCALL AudioSys::callback(void* udata, uint8* stream, int len) {
	AudioSys* as = static_cast<AudioSys*>(udata);
	SDL_memset(stream, 0, len);
	if (uint32(len) > as->alen) {
		if (!as->alen) {
			SDL_PauseAudioDevice(as->device, SDL_TRUE);
			return;
		}
		len = as->alen;
	}
	SDL_MixAudioFormat(stream, as->apos, as->aspec.format, len, as->volume);
	as->apos += len;
	as->alen -= len;
}
