//
// Created by PinkySmile on 11/08/2026.
//


#include <cpptrace/cpptrace.hpp>
#include <cstdint>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <csignal>
#include <sys/wait.h>
#include <TGUI/Widgets/RichTextLabel.hpp>
#include "Crash.hpp"
#include "Utils.hpp"


#define MAX_RAW_STACK_FRAMES 750000
#define SIG_STACK_SIZE (sizeof(CrashPacket) * MAX_STACK_FRAMES + sizeof(cpptrace::frame_ptr) * MAX_RAW_STACK_FRAMES + 1024 * 10)

struct CrashConfig {
	int readFd;
	int writeFd;
};

struct CrashPacket {
	uint32_t signal;
	void *faultAddress;
	pid_t pid;
	size_t frameCount;
	cpptrace::safe_object_frame frames[MAX_STACK_FRAMES];
};

static const CrashConfig *gCrashConfig = nullptr;

static size_t captureSafeTrace(cpptrace::safe_object_frame *buffer, size_t maxFrames)
{
	cpptrace::frame_ptr raw_frames[MAX_RAW_STACK_FRAMES];
	size_t count = cpptrace::safe_generate_raw_trace(raw_frames, MAX_RAW_STACK_FRAMES, 0);
	cpptrace::safe_object_frame temp;
	size_t j = 0;

	memset(buffer, 0, maxFrames * sizeof(*buffer));
	for (size_t i = 0; i < count && j < maxFrames; ++i) {
		cpptrace::get_safe_object_frame(raw_frames[i], &temp);
		if (j > 0 && memcmp(&temp, &buffer[j - 1], sizeof(temp)) == 0) {
			buffer[j].raw_address = -1;
			buffer[j].address_relative_to_object_start += 1;
		} else {
			if (buffer[j].raw_address == static_cast<cpptrace::frame_ptr>(-1)) {
				sprintf(buffer[j].object_path, "<repeating %zu times>", buffer[j].address_relative_to_object_start);
				j++;
			}
			memcpy(&buffer[j++], &temp, sizeof(temp));
		}
	}
	if (buffer[j].raw_address == static_cast<cpptrace::frame_ptr>(-1)) {
		sprintf(buffer[j].object_path, "<repeating %zu times>", buffer[j].address_relative_to_object_start);
		j++;
	}
	return j;
}

static void signalHandler(int sig, siginfo_t *info, void *)
{
	int wfd = gCrashConfig->writeFd;
	int rfd  = gCrashConfig->readFd;

	pid_t pid = vfork();
	if (pid == 0) {
		// --- CHILD PROCESS ---
		dup2(rfd, STDIN_FILENO);
		close(rfd);
		close(wfd);

		execle("/proc/self/exe", "crash_handler", nullptr, environ);
		_exit(127);
	}

	// --- PARENT PROCESS ---
	// Close read end in parent so child owns it exclusively
	close(rfd);

	// 2. Prepare packet and write to pipe (child is now reading in parallel via execve)
	CrashPacket packet{};
	packet.pid          = getpid();
	packet.signal       = static_cast<uint32_t>(sig);
	packet.faultAddress = info ? info->si_addr : nullptr;
	packet.frameCount   = captureSafeTrace(packet.frames, MAX_STACK_FRAMES);

	size_t len = sizeof(packet);
	auto ptr = reinterpret_cast<uintptr_t>(&packet);

	do {
		ssize_t bytes = write(wfd, reinterpret_cast<void *>(ptr), len);

		len -= bytes;
		ptr += bytes;
	} while (len != 0);
	close(wfd);

	_exit(128 + sig);
}

bool initCrashHandler()
{
	long pageSize = sysconf(_SC_PAGESIZE);
	void *rawPage = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (rawPage == MAP_FAILED) return false;

	int pipeFds[2];
	if (pipe2(pipeFds, O_CLOEXEC) == -1) {
		munmap(rawPage, pageSize);
		return false;
	}

	auto config = static_cast<CrashConfig *>(rawPage);
	config->readFd  = pipeFds[0];
	config->writeFd = pipeFds[1];

	if (mprotect(rawPage, pageSize, PROT_READ) == -1) {
		close(pipeFds[0]);
		close(pipeFds[1]);
		munmap(rawPage, pageSize);
		return false;
	}

	gCrashConfig = static_cast<const CrashConfig *>(rawPage);

	stack_t ss;
	ss.ss_sp = malloc(SIG_STACK_SIZE);
	ss.ss_size = SIG_STACK_SIZE;
	ss.ss_flags = 0;
	if (sigaltstack(&ss, NULL) == -1) {
		perror("sigaltstack");
		exit(EXIT_FAILURE);
	}

	struct sigaction sa{};
	sa.sa_sigaction = signalHandler;
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

	sigaction(SIGABRT, &sa, nullptr);
	sigaction(SIGFPE,  &sa, nullptr);

	sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGSTKFLT,&sa, nullptr);
	sigaction(SIGSEGV,  &sa, nullptr);
	sigaction(SIGBUS,   &sa, nullptr);
	sigaction(SIGILL,   &sa, nullptr);
	return true;
}

bool createCoreDump(const CrashPacket *packet, const std::filesystem::path &path)
{
	pid_t pid = fork();

	if (pid < 0)
		return false;
	if (pid == 0) {
		std::string pid_str = std::to_string(packet->pid);

		execlp("gcore", "gcore", "-o", path.string().c_str(), pid_str.c_str(), nullptr);
		_exit(127);
	}

	int status;

	waitpid(pid, &status, 0);
	return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

void readCrashPacket(CrashPacket *packet)
{
	size_t len = sizeof(*packet);
	ssize_t bytes;
	auto ptr = reinterpret_cast<uintptr_t>(packet);

	do {
		bytes = read(STDIN_FILENO, reinterpret_cast<void *>(ptr), len);
		len -= bytes;
		ptr += bytes;
	} while (len != 0);
}

tgui::Panel::Ptr crashPacketPanel(const CrashPacket &packet)
{
	tgui::Font traceFont("mods/NotoSansMono-Regular.ttf");
	auto panel = tgui::Panel::create({"100%", "100%"});
	auto title = tgui::Label::create("Game crashed !");
	auto signal = tgui::RichTextLabel::create("Signal: <color=#ff0000>" + std::to_string(packet.signal) + "</color> (<color=#ff0000>" + strsignal(packet.signal) + "</color>)");
	auto faultAddr = tgui::RichTextLabel::create("Cannot access <color=#0000ff>0x" + toHex(reinterpret_cast<uintptr_t>(packet.faultAddress)) + "</color>");
	std::vector<cpptrace::object_frame> object_frames;
	object_frames.reserve(packet.frameCount);
	for (size_t i = 0; i < packet.frameCount; ++i)
		object_frames.push_back(packet.frames[i].resolve());

	cpptrace::object_trace obj_trace(std::move(object_frames));
	std::string richString = stacktraceToRichString(obj_trace.resolve());

	title->setTextSize(40);
	title->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
	title->setPosition({10, 10});
	title->setSize({"&.w - 20", 60});
	title->setHorizontalAlignment(tgui::HorizontalAlignment::Center);
	panel->add(title, "title");

	signal->setPosition({10, "title.y + title.h + 10"});
	signal->setSize({"&.w - 20", 20});
	panel->add(signal, "signal");

	std::string last;
	faultAddr->setPosition({10, "signal.y + signal.h + 10"});
	faultAddr->setSize({"&.w - 20", 20});
	if (packet.signal == 11) {
		panel->add(faultAddr, "faultAddr");
		last = "faultAddr";
	} else
		last = "signal";

	auto stackTrace = tgui::RichTextLabel::create(richString);
	stackTrace->setAutoSize(true);
	stackTrace->getRenderer()->setTextColor(tgui::Color::White);
	stackTrace->getRenderer()->setFont(traceFont);

	auto spanel = tgui::ScrollablePanel::create();
	spanel->setPosition({10, tgui::Layout{last + ".y + " + last + ".h + 10"}});
	spanel->setSize({"&.w - 20", "&.h - y - 10"});
	spanel->getRenderer()->setBackgroundColor(tgui::Color{0x40, 0x40, 0x40});
	spanel->add(stackTrace);
	panel->add(spanel, "last");
	return panel;
}

std::string crashPacketToString(const CrashPacket &packet)
{
	std::stringstream ss;

	ss << "Signal: " << packet.signal << " (" << strsignal(packet.signal) << ")" << std::endl;
	ss << "Cannot access 0x" << std::hex << std::setfill('0') << std::setw(sizeof(void *) * 2) << reinterpret_cast<ptrdiff_t>(packet.faultAddress) << std::endl << std::endl;

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