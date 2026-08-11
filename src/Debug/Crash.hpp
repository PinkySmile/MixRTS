//
// Created by PinkySmile on 11/08/2026.
//

#ifndef MIXRTS_CRASH_HPP
#define MIXRTS_CRASH_HPP


#define MAX_STACK_FRAMES 100

#include <filesystem>
#include <TGUI/Widgets/Panel.hpp>

#include "../../cmake-build-debug-emscripten/_deps/cpptrace-src/include/cpptrace/cpptrace.hpp"

struct CrashPacket;

CrashPacket *allocCrashPacket();
int crashHandlerMain();
bool initCrashHandler();
bool createCoreDump(const CrashPacket *packet, const std::filesystem::path &path);
void readCrashPacket(CrashPacket *packet);
tgui::Panel::Ptr crashPacketPanel(const CrashPacket &packet);
std::string crashPacketToString(const CrashPacket &packet);
std::string stacktraceToRichString(cpptrace::stacktrace st);
void cleanStackTrace(cpptrace::stacktrace &st);
// bool fillReplayStruct(const CrashPacket &packet, ReplayData &data);
// bool fillStateVector(const CrashPacket &packet, std::vector<byte> &data);


#endif //MIXRTS_CRASH_HPP
