//
// Created for the MixRTS test target.
//

#include <algorithm>
#include <utility>
#include "MockedSoundManager.hpp"
#include "Resources/Game.hpp"
#include "Data/Logger.hpp"

MockedSoundManager::MockedSoundManager(MockMode mode, std::filesystem::path wavPath) :
	_mode(mode),
	_wavPath(std::move(wavPath.replace_extension("wav")))
{
}

MockedSoundManager::~MockedSoundManager()
{
	this->finishRecording();
}

void MockedSoundManager::finishRecording()
{
	if (this->_wavFile) {
		this->_wavFile->close();
		this->_wavFile.reset();
	}
}

MockMode MockedSoundManager::getMode() const
{
	return this->_mode;
}

const std::filesystem::path &MockedSoundManager::getWavPath() const
{
	return this->_wavPath;
}

unsigned MockedSoundManager::load(const std::filesystem::path &file, bool directionalSound)
{
	if (this->_mode != MODE_NORMAL)
		return this->_real.load(file, directionalSound);
	return 1;
}

void MockedSoundManager::addRef(unsigned id)
{
	if (this->_mode != MODE_NORMAL)
		this->_real.addRef(id);
}

void MockedSoundManager::remove(unsigned id)
{
	if (this->_mode != MODE_NORMAL)
		this->_real.remove(id);
}

bool MockedSoundManager::isPlaying(unsigned id) const
{
	if (this->_mode == MODE_DEBUG)
		return this->_real.isPlaying(id);
	// MODE_NORMAL never actually plays anything, and MODE_RECORD only ever
	// appends samples to a WAV file instantly: nothing is ever "still playing"
	// in either case.
	return false;
}

void MockedSoundManager::setCamera(Vector2f pos, Vector2f size)
{
	if (this->_mode != MODE_NORMAL)
		this->_real.setCamera(pos, size);
}

void MockedSoundManager::setVolume(float volume)
{
	if (this->_mode != MODE_NORMAL)
		this->_real.setVolume(volume);
}

void MockedSoundManager::play(unsigned id, float volume, Vector2f pos) const
{
	switch (this->_mode) {
	case MODE_DEBUG:
		this->_real.play(id, volume, pos);
		break;
	case MODE_RECORD:
		this->_queueVoice(id, volume);
		break;
	case MODE_NORMAL:
		// Nothing is played and nothing is recorded: this is the fast, headless
		// path meant for CI / automated runs.
		break;
	}
}

void MockedSoundManager::_queueVoice(unsigned id, float volume) const
{
	if (id == 0)
		return;

	auto &buffer = this->_real.getBuffer(id);

	if (!this->_wavFile) {
		this->_wavFile = std::make_unique<sf::OutputSoundFile>();
		this->_wavSampleRate = buffer.getSampleRate();
		this->_wavChannelCount = buffer.getChannelCount();
		if (!this->_wavFile->openFromFile(this->_wavPath, this->_wavSampleRate, this->_wavChannelCount, buffer.getChannelMap())) {
			game->logger.error("MockedSoundManager: failed to open " + this->_wavPath.string() + " for recording");
			this->_wavFile.reset();
			return;
		}
	}
	this->_voices.push_back({buffer, 0, volume});
}

void MockedSoundManager::update()
{
	if (this->_mode != MODE_RECORD || !this->_wavFile)
		return;

	// One RECORD_FRAMERATE-th of a second's worth of frames (i.e. samples,
	// not multiplied by the channel count): matches one MockedScreen video
	// frame, so a recording's WAV file ends up exactly as long as its video
	// as long as both update() and MockedScreen::display() get called once
	// per GameFixture::tick() (see tests/GameFixture.cpp/.hpp).
	std::uint64_t samplesPerTick = this->_wavSampleRate / RECORD_FRAMERATE;
	// Zero-initialized: silence is written out on ticks where nothing is
	// queued, so the WAV file's length keeps tracking elapsed ticks even
	// through quiet periods.
	std::vector<std::int16_t> mixed(samplesPerTick * this->_wavChannelCount, 0);

	for (auto it = this->_voices.begin(); it != this->_voices.end();) {
		auto &buffer = it->buffer;

		if (buffer.getChannelCount() != this->_wavChannelCount || buffer.getSampleRate() != this->_wavSampleRate) {
			// TODO: Sounds are never resampled/remixed to the WAV's format
			//       (that of the very first sound ever played): anything else
			//       is simply dropped rather than mixed in wrong.
			game->logger.warn("MockedSoundManager: dropping a queued sound with a different format than the WAV being recorded");
			it = this->_voices.erase(it);
			continue;
		}

		const std::int16_t *samples = buffer.getSamples();
		std::uint64_t totalFrames = buffer.getSampleCount() / this->_wavChannelCount;
		std::uint64_t framesToMix = std::min(totalFrames - it->frame, samplesPerTick);

		for (std::uint64_t frame = 0; frame < framesToMix; frame++)
			for (unsigned channel = 0; channel < this->_wavChannelCount; channel++) {
				std::uint64_t srcIndex = (it->frame + frame) * this->_wavChannelCount + channel;
				std::uint64_t dstIndex = frame * this->_wavChannelCount + channel;
				int scaled = static_cast<int>(samples[srcIndex] * (it->volume / 100.f));
				int sum = mixed[dstIndex] + scaled;

				mixed[dstIndex] = static_cast<std::int16_t>(std::clamp(sum, -32768, 32767));
			}

		it->frame += framesToMix;
		if (it->frame >= totalFrames)
			it = this->_voices.erase(it);
		else
			++it;
	}
	this->_wavFile->write(mixed.data(), mixed.size());
}
