#pragma once

#include "utils/objects.h"

struct Settings {
	static constexpr char loopback[] = "127.0.0.1";
	static constexpr float gammaMax = 2.f;

	enum class Screen : uint8 {
		window,
		borderless,
		fullscreen,
		desktop
	};
	static const array<string, sizet(Screen::desktop)+1> screenNames;

	enum class VSync : int8 {
		adaptive = -1,
		immediate,
		synchronized
	};
	static const array<string, 3> vsyncNames;	// add 1 to vsync value to get name

	bool maximized;
	uint8 avolume;
	uint8 display;
	Screen screen;
	VSync vsync;
	uint8 msamples;
	float gamma;
	vec2i size;
	SDL_DisplayMode mode;
	string address;
	uint16 port;

	Settings();
};

struct Setup {
	umap<vec2s, Com::Tile> tiles;
	umap<uint16, Com::Tile> mids;
	umap<vec2s, Com::Piece> pieces;

	void clear();
};

// handles all filesystem interactions
class FileSys {
private:
	static constexpr char fileAudios[] = "data/audio.dat";
	static constexpr char fileMaterials[] = "data/materials.dat";
	static constexpr char fileObjects[] = "data/objects.dat";
	static constexpr char fileSettings[] = "settings.ini";
	static constexpr char fileSetups[] = "setup.ini";
	static constexpr char fileShaders[] = "data/shaders.dat";
	static constexpr char fileTextures[] = "data/textures.dat";

	static constexpr char iniKeywordMaximized[] = "maximized";
	static constexpr char iniKeywordDisplay[] = "display";
	static constexpr char iniKeywordScreen[] = "screen";
	static constexpr char iniKeywordSize[] = "size";
	static constexpr char iniKeywordMode[] = "mode";
	static constexpr char iniKeywordVsync[] = "vsync";
	static constexpr char iniKeywordMsamples[] = "samples";
	static constexpr char iniKeywordGamma[] = "gamma";
	static constexpr char iniKeywordAVolume[] = "volume";
	static constexpr char iniKeywordAddress[] = "address";
	static constexpr char iniKeywordPort[] = "port";
	static constexpr char iniKeywordTile[] = "tile_";
	static constexpr char iniKeywordMid[] = "middle_";
	static constexpr char iniKeywordPiece[] = "piece_";

	static constexpr char errorAudios[] = "failed to load sounds";
	static constexpr char errorFile[] = "failed to load ";
	static constexpr char errorMaterials[] = "failed to load materials";
	static constexpr char errorObjects[] = "failed to load objects";
	static constexpr char errorShaders[] = "failed to load shaders";
	static constexpr char errorTextures[] = "failed to load textures";

public:
	static int setWorkingDir();
	static Settings* loadSettings();
	static bool saveSettings(const Settings* sets);
	static umap<string, Setup> loadSetups();
	static bool saveSetups(const umap<string, Setup>& sets);
	static umap<string, Sound> loadAudios(const SDL_AudioSpec& spec);
	static umap<string, Material> loadMaterials();
	static umap<string, GMesh> loadObjects();
	static umap<string, string> loadShaders();
	static umap<string, Texture> loadTextures();
};
