//
// Created by PinkySmile on 01/03/25.
//

#include "Sprite.hpp"
#include "Resources/Game.hpp"

Sprite::Sprite() :
	Sprite(0)
{
}

Sprite::Sprite(unsigned int handle) :
	sf::Sprite(game->textureMgr.getTexture(handle)),
	_textureHandle(handle)
{
	game->textureMgr.addRef(handle);
}

Sprite::Sprite(const Sprite &o) :
	sf::Sprite(o),
	_textureHandle(o.getHandle())
{
	game->textureMgr.addRef(o.getHandle());
}

Sprite &Sprite::operator=(const Sprite &sprite)
{
	sf::Sprite::operator=(sprite);
	this->_textureHandle = sprite.getHandle();
	game->textureMgr.addRef(this->_textureHandle);
	return *this;
}

Sprite::~Sprite()
{
	if (this->_textureHandle)
		game->textureMgr.remove(this->_textureHandle);
}

void Sprite::load(const std::filesystem::path &file, Vector2u *size, bool repeated, bool smooth)
{
	if (this->_textureHandle)
		game->textureMgr.remove(this->_textureHandle);
	this->_textureHandle = game->textureMgr.load(file, size, repeated, smooth);
	this->setTexture(game->textureMgr.getTexture(this->_textureHandle), true);
}

void Sprite::load(const std::filesystem::path &file, const std::array<Color, 256> &palette, Vector2u *size, bool repeated, bool smooth)
{
	if (this->_textureHandle)
		game->textureMgr.remove(this->_textureHandle);
	this->_textureHandle = game->textureMgr.load(file, palette, size, repeated, smooth);
	this->setTexture(game->textureMgr.getTexture(this->_textureHandle), true);
}

void Sprite::setTexture(unsigned int handle, bool resetRect)
{
	if (this->_textureHandle != handle) {
		game->textureMgr.remove(this->_textureHandle);
		game->textureMgr.addRef(handle);
		this->_textureHandle = handle;
	}
	this->setTexture(game->textureMgr.getTexture(handle), resetRect);
}

unsigned Sprite::getHandle() const
{
	return this->_textureHandle;
}

Vector2u Sprite::getTextureSize() const
{
	return game->textureMgr.getTextureSize(this->_textureHandle);
}

void Sprite::setTextureRect(IntRect rect)
{
	this->sf::Sprite::setTextureRect(rect);
}
