//
// Created for the MixRTS test target.
//

#ifndef MIXRTS_MOCKEDSOUNDMANAGER_HPP
#define MIXRTS_MOCKEDSOUNDMANAGER_HPP


#include <filesystem>
#include <memory>
#include <SFML/Audio.hpp>
#include "MockType.hpp"
#include "SFML/SoundManager.hpp"

//! Drop-in replacement for SoundManager used by the tests target (see
//! SOUND_MANAGER_CLASS_IMPLEM in Resources/Game.hpp and CMakeLists.txt).
//!
//! Behaves differently depending on the Mode given to the constructor:
//!  - MODE_NORMAL: nothing is ever actually loaded (load() always returns the
//!    "unset" handle 0, same idea as MockedTextureManager), and play() does
//!    nothing. This is the fast, headless path meant for CI / automated runs,
//!    where no audio device is needed/available and sound files don't even
//!    need to exist. isPlaying() always reports false.
//!  - MODE_DEBUG: everything forwards to an internal, real SoundManager,
//!    exactly like the game itself, so sounds are actually heard through the
//!    audio device.
//!  - MODE_RECORD: sounds are really loaded (needed to get real samples), but
//!    play() never touches a real audio device. Instead, it queues up the
//!    sound to be mixed into a WAV file at the path given to the constructor
//!    by update() (see below and the .cpp for the exact caveats of this
//!    "recording").
//!    isPlaying() always reports false, since nothing is ever actually
//!    "in progress" the way a real device playback would be.
class MockedSoundManager {
public:
	explicit MockedSoundManager(MockMode mode = MODE_NORMAL, std::filesystem::path wavPath = {});
	MockedSoundManager(const MockedSoundManager &) = delete;
	~MockedSoundManager();

	[[nodiscard]] MockMode getMode() const;
	[[nodiscard]] const std::filesystem::path &getWavPath() const;
	//! Closes the WAV file opened in MODE_RECORD (a no-op otherwise, or if
	//! nothing was ever played), finalizing it so it can safely be read (e.g.
	//! muxed with MockedScreen's video, see tests/main.cpp's
	//! mergeRecording()) right away. Also done by the destructor, but Game
	//! (and therefore this MockedSoundManager) is deliberately never
	//! destroyed/leaked (see GameFixture.hpp), so callers that need the WAV
	//! file to be complete before process exit must call this explicitly.
	void finishRecording();

	unsigned load(const std::filesystem::path &file);
	void addRef(unsigned id);
	void remove(unsigned id);
	void play(unsigned id, float volume) const;
	//! See the class doc: only ever true in MODE_DEBUG.
	bool isPlaying(unsigned id) const;
	void setVolume(float volume);
	//! Meant to be called once per GameFixture::tick(), MODE_RECORD only (a
	//! no-op otherwise). Mixes together, sample by sample, one
	//! RECORD_FRAMERATE-th of a second's worth of every sound queued by
	//! play() since the previous call (silence if none are queued), and
	//! appends the result to the WAV file, so that concurrently playing
	//! sounds actually end up merged together instead of being naively
	//! concatenated one after another. See the .cpp for the exact caveats.
	void update();

private:
	//! One sound queued by play() (MODE_RECORD only) that update() hasn't
	//! fully mixed into the WAV file yet.
	struct Voice {
		//! Own copy of the sound's buffer, rather than just its id: id may
		//! be removed from _real (e.g. because the Sound wrapper that played
		//! it got destroyed) well before update() is done mixing it in.
		sf::SoundBuffer buffer;
		//! Index (in frames, i.e. not multiplied by the channel count) of
		//! the first sample of buffer that hasn't been mixed in yet.
		std::uint64_t frame = 0;
		float volume;
	};

	//! Queues id up to be mixed in by future update() calls, lazily opening
	//! _wavFile using its buffer's format if this is the very first sound
	//! ever played. Used in MODE_RECORD only.
	void _queueVoice(unsigned id, float volume) const;

	MockMode _mode;
	std::filesystem::path _wavPath;
	SoundManager _real;
	//! Only ever opened in MODE_RECORD, by the first play() call (mutable since
	//! play() is otherwise a const, side-effect-free query on the real
	//! SoundManager side).
	mutable std::unique_ptr<sf::OutputSoundFile> _wavFile;
	//! Format _wavFile was opened with, i.e. that of the very first sound
	//! ever played: update() drops (with a warning) any later queued sound
	//! whose buffer doesn't share it, since samples are never resampled/
	//! remixed to a common format.
	mutable unsigned _wavSampleRate = 0;
	mutable unsigned _wavChannelCount = 0;
	//! Sounds queued by play(), not yet fully mixed into _wavFile by update().
	mutable std::vector<Voice> _voices;
};


#endif //MIXRTS_MOCKEDSOUNDMANAGER_HPP
