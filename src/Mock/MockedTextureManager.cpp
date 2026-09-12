//
// Created for the MixRTS test target.
//

#include "MockedTextureManager.hpp"
#include "Resources/Game.hpp"

MockedTextureManager::MockedTextureManager(MockMode mode) :
	_mode(mode)
{
	this->_dummyTexture.clear(Color::Transparent);
	this->_dummyTexture.display();
}

MockMode MockedTextureManager::getMode() const
{
	return this->_mode;
}

unsigned MockedTextureManager::_loadDummy(const std::string &key)
{
	auto it = this->_allocatedDummies.find(key);

	if (it != this->_allocatedDummies.end()) {
		it->second.count++;
		return it->second.index;
	}

	unsigned index;

	if (this->_freedDummyIndexes.empty()) {
		this->_lastDummyIndex += this->_lastDummyIndex == 0;
		index = this->_lastDummyIndex++;
	} else {
		index = this->_freedDummyIndexes.back();
		this->_freedDummyIndexes.pop_back();
	}
	this->_allocatedDummies[key] = {index, 1};
	this->_dummyKeys[index] = key;
	return index;
}

unsigned MockedTextureManager::load(const std::filesystem::path &file, Vector2u *size, bool repeated, bool smooth)
{
	if (this->_mode != MODE_NORMAL)
		return this->_real.load(file, size, repeated, smooth);
	if (size)
		*size = {1, 1};
	return this->_loadDummy(file.string());
}

unsigned MockedTextureManager::load(const std::filesystem::path &file, const std::filesystem::path &palette, Vector2u *size, bool repeated, bool smooth)
{
	if (this->_mode != MODE_NORMAL)
		return this->_real.load(file, palette, size, repeated, smooth);
	if (palette.empty())
		return this->load(file, size, repeated, smooth);
	if (size)
		*size = {1, 1};
	return this->_loadDummy(file.string() + ":" + palette.string());
}

unsigned MockedTextureManager::load(const std::filesystem::path &file, const std::array<Color, 256> &palette, Vector2u *size, bool repeated, bool smooth)
{
	if (this->_mode != MODE_NORMAL)
		return this->_real.load(file, palette, size, repeated, smooth);

	// Only used to build a dedup key below: the actual colors never matter in
	// MODE_NORMAL, since no pixel ever gets produced from them anyway.
	std::string paletteName;

	paletteName.resize(256 * 6);

	auto ptr = paletteName.data();

	for (auto color : palette) {
		sprintf(ptr, "%02X%02X%02X", color.r, color.g, color.b);
		ptr += 6;
	}
	if (size)
		*size = {1, 1};
	return this->_loadDummy(file.string() + ":" + paletteName);
}

Vector2u MockedTextureManager::getTextureSize(unsigned id) const
{
	if (this->_mode != MODE_NORMAL)
		return this->_real.getTextureSize(id);
	// Real sizes are never known in MODE_NORMAL, since nothing was ever actually
	// decoded from disk.
	return {1, 1};
}

void MockedTextureManager::addRef(unsigned id)
{
	if (this->_mode != MODE_NORMAL) {
		this->_real.addRef(id);
		return;
	}
	if (id == 0)
		return;

	auto &key = this->_dummyKeys.at(id);

	this->_allocatedDummies.at(key).count++;
}

void MockedTextureManager::remove(unsigned id)
{
	if (this->_mode != MODE_NORMAL) {
		this->_real.remove(id);
		return;
	}
	if (id == 0)
		return;

	auto &key = this->_dummyKeys.at(id);
	auto &dummy = this->_allocatedDummies.at(key);

	if (dummy.count) {
		dummy.count--;
		if (dummy.count)
			return;
	}
	this->_allocatedDummies.erase(key);
	this->_dummyKeys.erase(id);
	this->_freedDummyIndexes.push_back(id);
}

const std::optional<std::array<Color, 256>> &MockedTextureManager::getPalette(unsigned id)
{
	if (this->_mode != MODE_NORMAL)
		return this->_real.getPalette(id);
	return this->_emptyPalette;
}

const sf::Texture &MockedTextureManager::getTexture(unsigned id) const
{
	if (this->_mode != MODE_NORMAL)
		return this->_real.getTexture(id);
	return this->_dummyTexture.getTexture();
}

void MockedTextureManager::reloadEverything()
{
	if (this->_mode != MODE_NORMAL)
		this->_real.reloadEverything();
	// Nothing was ever actually loaded in MODE_NORMAL, so there is nothing to
	// reload.
}

void MockedTextureManager::addOverride(const std::filesystem::path &base, const std::filesystem::path &newVal)
{
	// Harmless bookkeeping, no filesystem access: always forwarded so the real
	// TextureManager stays consistent if MODE_RECORD/MODE_DEBUG ever uses it.
	this->_real.addOverride(base, newVal);
}

void MockedTextureManager::removeOverride(const std::filesystem::path &base)
{
	this->_real.removeOverride(base);
}

std::optional<std::array<Color, 256>> MockedTextureManager::getEmbeddedPalette(const std::filesystem::path &path)
{
	if (this->_mode == MODE_NORMAL)
		return std::nullopt;
	return this->_real.getEmbeddedPalette(path);
}
