//
// Created by PinkySmile on 06/10/2026.
//

#include "Crash.hpp"

int crashHandlerMain()
{
	puts("Stub!");
	return EXIT_SUCCESS;
}

void simplifySym(std::string &)
{
}

bool initCrashHandler()
{
	return true;
}

bool createCoreDump(const CrashPacket *, const std::filesystem::path &)
{
	return true;
}

bool extractCrashReplay(const CrashPacket &, const std::filesystem::path &)
{
	return false;
}

bool extractCrashState(const CrashPacket &, const std::filesystem::path &)
{
	return false;
}

tgui::Panel::Ptr crashPacketPanel(const CrashPacket &)
{
	return tgui::Panel::create();
}

std::string crashPacketToString(const CrashPacket &)
{
	return "";
}

std::string stacktraceToRichString(Stacktrace)
{
	return "";
}

void cleanStackTrace(Stacktrace &)
{
}