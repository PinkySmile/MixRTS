//
// Created for the MixRTS test target.
//

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <gtest/gtest.h>
#include "GameFixture.hpp"
#include "Resources/Game.hpp"

namespace {

MockMode parseMode(const std::string &value)
{
	if (value == "normal")
		return MODE_NORMAL;
	if (value == "record")
		return MODE_RECORD;
	if (value == "debug")
		return MODE_DEBUG;
	std::cerr << "Invalid --mode value '" << value << "' (expected normal, record or debug)" << std::endl;
	std::exit(EXIT_FAILURE);
}

Vector2u parseSize(const std::string &value)
{
	auto sep = value.find('x');

	if (sep != std::string::npos)
		try {
			return {
				static_cast<unsigned>(std::stoul(value.substr(0, sep))),
				static_cast<unsigned>(std::stoul(value.substr(sep + 1)))
			};
		} catch (const std::exception &) {
			// Fall through to the error below.
		}
	std::cerr << "Invalid --size value '" << value << "' (expected WIDTHxHEIGHT, e.g. 1280x720)" << std::endl;
	std::exit(EXIT_FAILURE);
}

//! Returns the value part of arg if it is of the form "--<name>=<value>", or
//! std::nullopt otherwise.
std::optional<std::string> takeOption(const std::string &arg, const std::string &name)
{
	std::string prefix = "--" + name + "=";

	if (arg.compare(0, prefix.size(), prefix) != 0)
		return std::nullopt;
	return arg.substr(prefix.size());
}

//! Runs, at process exit, after every test has finished (see main() below):
//! muxes the WAV file progressively written to by MockedSoundManager::update()
//! and the video written to by MockedScreen (see GameFixture::tick(), which
//! calls both once per tick) into a single video file with sound.
//!
//! A no-op unless Game was actually constructed in MODE_RECORD (nothing was
//! ever recorded otherwise, see GameFixture::configure()), and both files
//! actually exist (e.g. because at least one tick, and one played sound,
//! happened).
void mergeRecording()
{
	if (!game || game->soundMgr.getMode() != MODE_RECORD)
		return;

	// Game (and therefore its screen/soundMgr) is deliberately leaked (see
	// GameFixture.hpp), so their destructors never run: the ffmpeg process
	// backing the video and the WAV file must be explicitly finalized here,
	// or both would still be incomplete/unreadable at this point.
	game->screen->finishRecording();
	game->soundMgr.finishRecording();

	auto video = game->screen->getVideoPath();
	auto wav = game->soundMgr.getWavPath();

	if (!std::filesystem::exists(video) || !std::filesystem::exists(wav)) {
		std::cerr << "Skipping audio/video merge: " << video << " and/or " << wav << " don't exist" << std::endl;
		return;
	}

	// e.g. "video.mkv" -> "video-with-audio.mkv", next to the original files.
	auto merged = video;

	merged.replace_filename(video.stem().string() + "-with-audio" + video.extension().string());

	std::ostringstream cmd;

	// Both streams are just copied as-is (no re-encoding): MockedScreen's
	// video is already in a codec Matroska/MP4 support directly, and
	// Matroska/MP4 both support raw PCM audio, exactly what the WAV file
	// already contains. -shortest keeps the result as long as the shorter of
	// the two (there is no video without at least one tick, but there can be
	// ticks - and therefore video - without any sound ever being played).
	cmd << "ffmpeg"
		<< " -y -loglevel error"
		<< " -i \"" << video.string() << "\""
		<< " -i \"" << wav.string() << "\""
		<< " -c:v copy -c:a copy -shortest"
		<< " \"" << merged.string() << "\"";

	if (std::system(cmd.str().c_str()) != 0)
		std::cerr << "Failed to merge " << video << " and " << wav << " into " << merged << std::endl;
	else
		std::cout << "Merged recording written to " << merged << std::endl;
}

}

//! Custom entry point (instead of simply linking GTest::gtest_main) so that
//! the Mode/video path/window size the shared Game instance (see
//! GameFixture::configure()) is constructed with can be controlled from the
//! command line, e.g. to actually watch (--mode=debug) or record
//! (--mode=record --video=some/path.mp4) a test run instead of running it
//! through the default, fast and headless MODE_NORMAL.
//!
//! Any argument recognized by GoogleTest itself (e.g. --gtest_filter=...) is
//! consumed by InitGoogleTest() below and never reaches the parsing loop.
int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);

	MockMode mode = MODE_NORMAL;
	std::filesystem::path video = "video";
	Vector2u size{1280, 1024};

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (auto value = takeOption(arg, "mode"))
			mode = parseMode(*value);
		else if (auto value = takeOption(arg, "video"))
			video = *value;
		else if (auto value = takeOption(arg, "size"))
			size = parseSize(*value);
		else {
			std::cerr <<
				"Unknown argument '" << arg << "'\n"
				"Expected --mode=<normal|record|debug>, --video=<path> and/or --size=<WIDTHxHEIGHT>"
			<< std::endl;
			return EXIT_FAILURE;
		}
	}

	GameFixture::configure(mode, video, size);

	int result = RUN_ALL_TESTS();

	// Always attempted, even if some test failed above, so a recording is
	// still available to help figure out why.
	mergeRecording();
	return result;
}
