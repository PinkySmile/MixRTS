//
// Created for the MixRTS test target.
//

#ifndef MIXRTS_MOCKEDTYPE_HPP
#define MIXRTS_MOCKEDTYPE_HPP

enum MockMode {
	//! Textures/sounds are never actually decoded from disk
	MODE_NORMAL,
	//! Textures/sounds are loaded, and recorded to a video/audio file
	MODE_RECORD,
	//! Textures/sounds are loaded and a window is opened
	MODE_DEBUG,
};

//! Tick/frame rate assumed for MODE_RECORD by both MockedScreen (video, one
//! MockedScreen::display() call == one frame) and MockedSoundManager (audio,
//! one MockedSoundManager::update() call == one RECORD_FRAMERATE-th of a
//! second's worth of mixed samples). Shared so that, as long as both get
//! called once per GameFixture::tick(), a recording's video and audio stay
//! the same length and can be merged back together after the fact.
constexpr unsigned RECORD_FRAMERATE = 50;

#endif // MIXRTS_MOCKEDTYPE_HPP