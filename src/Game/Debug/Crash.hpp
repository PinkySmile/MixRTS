//
// Created by PinkySmile on 11/08/2026.
//

#ifndef MIXRTS_CRASH_HPP
#define MIXRTS_CRASH_HPP


#define MAX_STACK_FRAMES 100

#include <filesystem>
#include <TGUI/Widgets/Panel.hpp>
#ifdef NO_TRACE
struct Stacktrace {
	void print(std::ostream & = std::cout, bool = false) {}
};
#else
#include <cpptrace/cpptrace.hpp>

typedef cpptrace::stacktrace Stacktrace;
#endif

struct CrashPacket;

CrashPacket *allocCrashPacket();
int crashHandlerMain();
bool initCrashHandler();
bool createCoreDump(const CrashPacket *packet, const std::filesystem::path &path);
void readCrashPacket(CrashPacket *packet);
tgui::Panel::Ptr crashPacketPanel(const CrashPacket &packet);
std::string crashPacketToString(const CrashPacket &packet);
std::string stacktraceToRichString(Stacktrace st);
void cleanStackTrace(Stacktrace &st);
//! File descriptor the crash handler inherits and writes one byte to once the game may exit.
static constexpr int CRASH_ACK_FD = 3;
//! Reads the in-progress replay out of the crashed process, if one was published.
bool extractCrashReplay(const CrashPacket &packet, const std::filesystem::path &dir);
//! Reads the published simulation state out of the crashed process, if one was published.
bool extractCrashState(const CrashPacket &packet, const std::filesystem::path &dir);

tgui::Panel::Ptr generateCrashPanel(Stacktrace st, const std::exception &e);
void handleCrash(Stacktrace st, const std::exception &e);
//! Creates crash-YYYY-MM-DD-HH-MM-SS and returns that path.
std::filesystem::path createCrashFolder();


#endif //MIXRTS_CRASH_HPP
