//
// Created by PinkySmile on 11/08/2026.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <dbghelp.h>
#include <cpptrace/cpptrace.hpp>
#include <TGUI/Widgets/RichTextLabel.hpp>
#include "Crash.hpp"
#include "Utils.hpp"


struct CrashPacket {
	uint32_t excode;
	DWORD    pid;
	DWORD    tid;
	void     *exceptionPointersParam;
	void     *faultAddress;
	size_t   frameCount;
	cpptrace::safe_object_frame frames[MAX_STACK_FRAMES];
};

struct CrashConfig {
	HANDLE rpipe;
	HANDLE wpipe;
};

static const CrashConfig* gCrashConfig = nullptr;

bool createCoreDump(const CrashPacket *packet, const std::filesystem::path &path)
{
	HANDLE hParent = OpenProcess(
		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
		FALSE,
		packet->pid
	);

	if (!hParent || hParent == INVALID_HANDLE_VALUE)
		return false;

	// Create dump file
	HANDLE hFile = CreateFileW(
		path.wstring().c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		CloseHandle(hParent);
		return false;
	}

	MINIDUMP_EXCEPTION_INFORMATION exInfo{};
	exInfo.ThreadId          = packet->tid;
	exInfo.ExceptionPointers = static_cast<PEXCEPTION_POINTERS>(packet->exceptionPointersParam);
	exInfo.ClientPointers    = TRUE; // Tells DbgHelp that pointers are in target process memory

	MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
		MiniDumpWithFullMemory |
		MiniDumpWithFullMemoryInfo |
		MiniDumpWithHandleData
	);

	BOOL success = MiniDumpWriteDump(
		hParent,
		packet->pid,
		hFile,
		dumpType,
		packet->exceptionPointersParam != nullptr ? &exInfo : nullptr,
		nullptr,
		nullptr
	);

	CloseHandle(hFile);
	CloseHandle(hParent);
	return success;
}

static size_t captureSafeTrace(cpptrace::safe_object_frame *buffer, size_t maxFrames) {
	cpptrace::frame_ptr rawFrames[MAX_STACK_FRAMES];
	size_t count = cpptrace::safe_generate_raw_trace(rawFrames, maxFrames, 0);

	for (size_t i = 0; i < count; ++i) {
		uintptr_t addr = rawFrames[i];
		HMODULE hModule = nullptr;

		buffer[i].raw_address = addr;
		buffer[i].object_path[0] = '\0';
		buffer[i].address_relative_to_object_start = 0;

		// Query the module containing the instruction pointer
		if (GetModuleHandleExA(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(addr),
			&hModule
		) && hModule != nullptr) {
			// Get the absolute path to the module (.exe or .dll)
			DWORD len = GetModuleFileNameA(
				hModule,
				buffer[i].object_path,
				sizeof(buffer[i].object_path)
			);

			if (len > 0 && len < sizeof(buffer[i].object_path))
				buffer[i].address_relative_to_object_start = addr - reinterpret_cast<uintptr_t>(hModule);
		}
	}
	return count;
}


static LONG WINAPI exceptionFilter(EXCEPTION_POINTERS* ep)
{
	HANDLE wpipe = gCrashConfig->wpipe;
	HANDLE rpipe  = gCrashConfig->rpipe;

	CrashPacket packet{};
	if (ep && ep->ExceptionRecord) {
		packet.excode       = ep->ExceptionRecord->ExceptionCode;
		packet.faultAddress = ep->ExceptionRecord->ExceptionAddress;
	}

	// Capture safe trace with ASLR offset resolution
	packet.frameCount = captureSafeTrace(packet.frames, MAX_STACK_FRAMES);
	packet.pid = GetCurrentProcessId();
	packet.tid = GetCurrentThreadId();
	packet.exceptionPointersParam = ep;

	STARTUPINFOA si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;

	// Pass read_pipe to stdin, and forward parent's stdout/stderr handles to child
	si.hStdInput  = rpipe;
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

	char cmd[] = "crash_handler";
	char exe_path[MAX_PATH];
	GetModuleFileNameA(nullptr, exe_path, MAX_PATH);

	if (!CreateProcessA(exe_path, cmd, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
		TerminateProcess(GetCurrentProcess(), packet.excode);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// Parent closes read handle so child process is sole owner
	CloseHandle(rpipe);

	DWORD bytes_written = 0;
	WriteFile(wpipe, &packet, sizeof(packet), &bytes_written, nullptr);
	CloseHandle(wpipe);

	// Wait for child crash handler process to finish printing trace output
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	TerminateProcess(GetCurrentProcess(), packet.excode);
	return EXCEPTION_EXECUTE_HANDLER;
}

bool initCrashHandler()
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	void *rawPage = VirtualAlloc(nullptr, sys_info.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!rawPage) return false;

	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;

	HANDLE r_pipe, w_pipe;
	if (!CreatePipe(&r_pipe, &w_pipe, &sa, 0)) {
		VirtualFree(rawPage, 0, MEM_RELEASE);
		return false;
	}

	auto config = static_cast<CrashConfig *>(rawPage);
	config->rpipe  = r_pipe;
	config->wpipe = w_pipe;

	DWORD old_protect;
	if (!VirtualProtect(rawPage, sys_info.dwPageSize, PAGE_READONLY, &old_protect)) {
		CloseHandle(r_pipe);
		CloseHandle(w_pipe);
		VirtualFree(rawPage, 0, MEM_RELEASE);
		return false;
	}

	gCrashConfig = static_cast<const CrashConfig *>(rawPage);
	SetUnhandledExceptionFilter(exceptionFilter);
	ULONG guarantee = 32 * 1024;
	SetThreadStackGuarantee(&guarantee);
	return true;
}

void readCrashPacket(CrashPacket *packet)
{
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD bytes_read = 0;

	ReadFile(hStdin, packet, sizeof(*packet), &bytes_read, nullptr);
}

DWORD FormatNtStatus(NTSTATUS nsCode, wchar_t **ppszMessage) {
	HMODULE hNtDll = LoadLibraryA("NTDLL.DLL");

	if (hNtDll == nullptr) return 0;

	// Call FormatMessage(), note use of RtlNtStatusToDosError().
	DWORD dwRes = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
		hNtDll,
		RtlNtStatusToDosError(nsCode),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)ppszMessage,
		0,
		nullptr
	);

	FreeLibrary(hNtDll);
	return dwRes;
}

tgui::Panel::Ptr crashPacketPanel(const CrashPacket &packet)
{
	tgui::Font traceFont("mods/NotoSansMono-Regular.ttf");
	auto panel = tgui::Panel::create({"100%", "100%"});
	auto title = tgui::Label::create("Game crashed !");
	std::wstring signalStr;
	wchar_t *ppszMessage = nullptr;
	DWORD len = FormatNtStatus(packet.excode, &ppszMessage);
	while (len > 0 && (ppszMessage[len - 1] == '\n' || ppszMessage[len - 1] == '\r' || ppszMessage[len - 1] == '.'))
		len--;
	if (ppszMessage)
		ppszMessage[len] = 0;
	signalStr = L"Exception code: <color=#ff0000>0x" + wtoHex(packet.excode) + L"</color> (<color=#ff0000>" + (ppszMessage ? ppszMessage : L"Unknown exception") + L"</color>)";
	auto signal = tgui::RichTextLabel::create(signalStr);
	auto faultAddr = tgui::RichTextLabel::create("Fault Address: <color=#0000ff>0x" + toHex(reinterpret_cast<uintptr_t>(packet.faultAddress)) + "</color>");
	std::stringstream ss;

	title->setTextSize(40);
	title->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
	title->setPosition({10, 10});
	title->setSize({"&.w - 20", 60});
	title->setHorizontalAlignment(tgui::HorizontalAlignment::Center);
	panel->add(title, "title");

	signal->setPosition({10, "title.y + title.h + 10"});
	signal->setSize({"&.w - 20", 20});
	panel->add(signal, "signal");

	faultAddr->setPosition({10, "signal.y + signal.h + 10"});
	faultAddr->setSize({"&.w - 20", 20});
	panel->add(faultAddr, "faultAddr");

	std::vector<cpptrace::object_frame> object_frames;
	object_frames.reserve(packet.frameCount);
	for (size_t i = 0; i < packet.frameCount; ++i)
		object_frames.push_back(packet.frames[i].resolve());

	cpptrace::object_trace obj_trace(std::move(object_frames));
	std::string richString = stacktraceToRichString(obj_trace.resolve());

	auto stackTrace = tgui::RichTextLabel::create(richString);
	stackTrace->setPosition({10, "faultAddr.y + faultAddr.h + 10"});
	stackTrace->setAutoSize(true);
	stackTrace->getScrollbar()->setPolicy(tgui::Scrollbar::Policy::Automatic);
	stackTrace->getRenderer()->setTextColor(tgui::Color::Black);
	stackTrace->getRenderer()->setFont(traceFont);
	stackTrace->getRenderer()->setBackgroundColor(tgui::Color{0xA0, 0xA0, 0xA0});
	stackTrace->setSize({"&.w - 20", "&.h - y - 10"});
	panel->add(stackTrace, "last");
	return panel;
}

std::string crashPacketToString(const CrashPacket &packet)
{
	std::stringstream ss;

	wchar_t *ppszMessage = nullptr;
	DWORD len = FormatNtStatus(packet.excode, &ppszMessage);
	while (len > 0 && (ppszMessage[len - 1] == '\n' || ppszMessage[len - 1] == '\r' || ppszMessage[len - 1] == '.'))
		len--;
	if (ppszMessage)
		ppszMessage[len] = 0;
	ss << "Exception code: 0x" << std::hex << std::uppercase << packet.excode << " (";
	if (ppszMessage != nullptr) {
		for (size_t i = 0; ppszMessage[i]; i++)
			ss.put(ppszMessage[i]);
	} else
		ss << "Unknown exception";
	ss << ")" << std::dec << std::endl;

	ss << "Fault Address: 0x" << std::hex << std::setfill('0') << std::setw(sizeof(void *) * 2) << reinterpret_cast<ptrdiff_t>(packet.faultAddress) << std::endl << std::endl;

	std::vector<cpptrace::object_frame> object_frames;
	object_frames.reserve(packet.frameCount);
	for (size_t i = 0; i < packet.frameCount; ++i)
		object_frames.push_back(packet.frames[i].resolve());

	cpptrace::object_trace obj_trace(std::move(object_frames));
	cpptrace::stacktrace trace = obj_trace.resolve();

	trace.print(ss, false);
	return ss.str();
}

CrashPacket *allocCrashPacket()
{
	return new CrashPacket;
}