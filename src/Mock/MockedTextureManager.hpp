//
// Created for the MixRTS test target.
//

#ifndef MIXRTS_MOCKEDTEXTUREMANAGER_HPP
#define MIXRTS_MOCKEDTEXTUREMANAGER_HPP


#include <array>
#include <optional>
#include <unordered_map>
#include "MockType.hpp"
#include "SFML/TextureManager.hpp"

//! Drop-in replacement for TextureManager used by the tests target (see
//! TEXTURE_MANAGER_CLASS_IMPLEM in Resources/Game.hpp and CMakeLists.txt).
//!
//! Unlike MockedScreen, this does NOT try to reuse TextureManager's internals for
//! its fake path: it just owns a real, internal TextureManager (used in MODE_RECORD
//! / MODE_DEBUG, exactly like the game itself, since actual pixels are needed to
//! produce meaningful recorded frames or a real debug window) and, for MODE_NORMAL,
//! keeps its own tiny bookkeeping so that handles/ref-counting/dedup-by-path still
//! behave the same way from the caller's point of view, but no file is ever
//! actually opened/decoded: every handle just maps to the same shared 1x1 dummy
//! texture. This is the fast, headless path meant for CI / automated runs, where
//! tests don't need to ship or read real texture assets at all.
class MockedTextureManager {
public:
	explicit MockedTextureManager(MockMode mode = MODE_NORMAL);

	[[nodiscard]] MockMode getMode() const;

	unsigned load(const std::filesystem::path &file, Vector2u *size = nullptr, bool repeated = false, bool smooth = false);
	unsigned load(const std::filesystem::path &file, const std::filesystem::path &palette, Vector2u *size = nullptr, bool repeated = false, bool smooth = false);
	unsigned load(const std::filesystem::path &file, const std::array<Color, 256> &palette, Vector2u *size = nullptr, bool repeated = false, bool smooth = false);
	Vector2u getTextureSize(unsigned id) const;
	void addRef(unsigned id);
	void remove(unsigned id);
	const std::optional<std::array<Color, 256>> &getPalette(unsigned id);
	const sf::Texture &getTexture(unsigned id) const;
	void reloadEverything();
	void addOverride(const std::filesystem::path &base, const std::filesystem::path &newVal);
	void removeOverride(const std::filesystem::path &base);
	std::optional<std::array<Color, 256>> getEmbeddedPalette(const std::filesystem::path &path);

private:
	struct DummyEntry {
		unsigned index;
		unsigned count;
	};

	//! Allocates (or, if key is already known, dedups/refs) a dummy handle for
	//! key without touching the filesystem at all.
	unsigned _loadDummy(const std::string &key);

	MockMode _mode;
	TextureManager _real; //!< Only actually used in MODE_RECORD / MODE_DEBUG.

	// Bookkeeping used in MODE_NORMAL only, mirroring TextureManager's own
	// dedup-by-key/ref-count scheme (see TextureManager::_allocatedTextures)
	// closely enough that callers can't tell the difference, minus the actual
	// pixel data.
	sf::RenderTexture _dummyTexture{{1, 1}};
	unsigned _lastDummyIndex = 0;
	std::vector<unsigned> _freedDummyIndexes;
	std::unordered_map<std::string, DummyEntry> _allocatedDummies;
	std::unordered_map<unsigned, std::string> _dummyKeys;
	//! Always empty: MODE_NORMAL never has a real palette to report.
	std::optional<std::array<Color, 256>> _emptyPalette;
};


#endif //MIXRTS_MOCKEDTEXTUREMANAGER_HPP
