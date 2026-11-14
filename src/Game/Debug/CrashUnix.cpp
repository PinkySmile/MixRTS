//
// Created by PinkySmile on 11/08/2026.
//

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cpptrace/cpptrace.hpp>
#include <cstdint>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <csignal>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/prctl.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <vector>
#include <TGUI/Widgets/RichTextLabel.hpp>
#include "Crash.hpp"
#include "Data/Replay.hpp"
#include "Utils.hpp"


#define MAX_RAW_STACK_FRAMES 750000
#define SIG_STACK_SIZE (sizeof(CrashPacket) * MAX_STACK_FRAMES + sizeof(cpptrace::frame_ptr) * MAX_RAW_STACK_FRAMES + 1024 * 10)

struct CrashConfig {
	int readFd;
	int writeFd;
	int ackReadFd;
	int ackWriteFd;
};

struct CrashPacket {
	uint32_t signal;
	void *faultAddress;
	pid_t pid;
	const void *replayAnchor;
	const void *stateAnchor;
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
	// Snapshot the tick that was running before anything else can move it.
	crashReplayAnchor.captureCrashTick();

	int wfd = gCrashConfig->writeFd;
	int rfd  = gCrashConfig->readFd;
	int ackWrite = gCrashConfig->ackWriteFd;
	int ackRead = gCrashConfig->ackReadFd;

	pid_t pid = vfork();
	if (pid == 0) {
		// --- CHILD PROCESS ---
		// Close the originals before reusing their numbers. The packet read end is usually
		// fd 3, which is also where the ack write end has to survive exec.
		dup2(rfd, STDIN_FILENO);
		if (rfd != STDIN_FILENO)
			close(rfd);
		if (wfd != STDIN_FILENO)
			close(wfd);
		if (dup2(ackWrite, CRASH_ACK_FD) < 0)
			_exit(127);
		if (ackWrite != CRASH_ACK_FD)
			close(ackWrite);
		if (ackRead != STDIN_FILENO && ackRead != CRASH_ACK_FD)
			close(ackRead);

		execl("/proc/self/exe", "crash_handler", static_cast<char *>(NULL));
		_exit(127);
	}

	// --- PARENT PROCESS ---
	// vfork only returns here after the child has exec'd, and the child blocks until this packet arrives.
	prctl(PR_SET_PTRACER, pid, 0, 0, 0);
	// Close read end in parent so child owns it exclusively
	close(rfd);

	CrashPacket packet{};
	packet.pid          = getpid();
	packet.signal       = static_cast<uint32_t>(sig);
	packet.faultAddress = info ? info->si_addr : nullptr;
	packet.replayAnchor = &crashReplayAnchor;
	packet.stateAnchor  = &crashStateAnchor;
	packet.frameCount   = captureSafeTrace(packet.frames, MAX_STACK_FRAMES);

	size_t len = sizeof(packet);
	auto ptr = reinterpret_cast<uintptr_t>(&packet);

	do {
		ssize_t bytes = write(wfd, reinterpret_cast<void *>(ptr), len);

		if (bytes <= 0)
			_exit(128 + sig);
		len -= bytes;
		ptr += bytes;
	} while (len != 0);
	close(wfd);

	alarm(30);
	char ack = 0;
	while (read(ackRead, &ack, 1) < 0 && errno == EINTR)
		;
	_exit(128 + sig);
}

bool initCrashHandler()
{
	long pageSize = sysconf(_SC_PAGESIZE);
	void *rawPage = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	CrashConfig *config;
	stack_t ss;
	struct sigaction sa{};

	if (rawPage == MAP_FAILED) return false;

	int pipeFds[2];
	int ackFds[2];
	if (pipe2(pipeFds, O_CLOEXEC) == -1)
		goto onerror_munmap;
	if (pipe2(ackFds, O_CLOEXEC) == -1)
		goto onerror_pipefd;

	config = static_cast<CrashConfig *>(rawPage);
	config->readFd  = pipeFds[0];
	config->writeFd = pipeFds[1];
	config->ackReadFd = ackFds[0];
	config->ackWriteFd = ackFds[1];

	if (mprotect(rawPage, pageSize, PROT_READ) == -1)
		goto onerror_ackfd;

	gCrashConfig = static_cast<const CrashConfig *>(rawPage);

	ss.ss_sp = malloc(SIG_STACK_SIZE);
	ss.ss_size = SIG_STACK_SIZE;
	ss.ss_flags = 0;
	if (sigaltstack(&ss, NULL) == -1)
		goto onerror;

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

onerror:
	free(ss.ss_sp);
onerror_ackfd:
	close(ackFds[0]);
	close(ackFds[1]);
onerror_pipefd:
	close(pipeFds[0]);
	close(pipeFds[1]);
onerror_munmap:
	munmap(rawPage, pageSize);
	return false;
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
	tgui::Font traceFont("assets/NotoSansMono-Regular.ttf");
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

static constexpr size_t MAX_CRASH_HEADER = 1u << 20;
static constexpr size_t MAX_CRASH_STREAM = 32u << 20;
static constexpr size_t MAX_CRASH_STATE = 64u << 20;
static constexpr uint32_t MAX_CRASH_PLAYERS = 64;

static bool readRemote(pid_t pid, uintptr_t address, void *dst, size_t size)
{
	if (size == 0)
		return true;

	iovec local{dst, size};
	iovec remote{reinterpret_cast<void *>(address), size};
	ssize_t copied = process_vm_readv(pid, &local, 1, &remote, 1, 0);

	if (copied == static_cast<ssize_t>(size))
		return true;

	auto *out = static_cast<unsigned char *>(dst);
	size_t done = 0;

	while (done < size) {
		errno = 0;

		long word = ptrace(PTRACE_PEEKDATA, pid, reinterpret_cast<void *>(address + done), nullptr);

		if (word == -1 && errno != 0)
			return false;

		size_t chunk = sizeof(word);

		if (chunk > size - done)
			chunk = size - done;
		std::memcpy(out + done, &word, chunk);
		done += chunk;
	}
	return true;
}

struct FrozenThreads {
	std::vector<pid_t> tids;

	~FrozenThreads()
	{
		for (pid_t tid : this->tids)
			ptrace(PTRACE_DETACH, tid, nullptr, nullptr);
	}
};

static bool freezeThreads(pid_t pid, FrozenThreads &frozen)
{
	std::error_code error;
	std::filesystem::path tasks = std::filesystem::path{"/proc"} / std::to_string(pid) / "task";

	for (const auto &entry : std::filesystem::directory_iterator(tasks, error)) {
		pid_t tid;

		try {
			tid = static_cast<pid_t>(std::stoi(entry.path().filename().string()));
		} catch (const std::exception &) {
			continue;
		}
		if (ptrace(PTRACE_SEIZE, tid, nullptr, nullptr) != 0) {
			if (errno == ESRCH)
				continue;
			return false;
		}
		frozen.tids.push_back(tid);
		if (ptrace(PTRACE_INTERRUPT, tid, nullptr, nullptr) != 0) {
			if (errno == ESRCH)
				continue;
			return false;
		}

		int status = 0;

		while (true) {
			if (waitpid(tid, &status, __WALL) < 0) {
				if (errno == EINTR)
					continue;
				if (errno == ECHILD)
					break;
				return false;
			}
			break;
		}
	}
	return !error && !frozen.tids.empty();
}

bool extractCrashReplay(const CrashPacket &packet, const std::filesystem::path &dir)
{
	if (!packet.replayAnchor)
		return false;

	std::vector<uint8_t> header;
	std::vector<std::vector<uint8_t>> encoded;
	uint32_t tick = 0;

	{
		FrozenThreads frozen;

		if (!freezeThreads(packet.pid, frozen))
			return false;

		CrashReplayAnchor anchor{};

		if (!readRemote(packet.pid, reinterpret_cast<uintptr_t>(packet.replayAnchor), &anchor, sizeof(anchor)))
			return false;
		if (anchor.magic != CrashReplayAnchor::MAGIC)
			return false;
		if (!anchor.header || anchor.headerSize == 0 || anchor.headerSize > MAX_CRASH_HEADER)
			return false;
		if (anchor.playerCount > MAX_CRASH_PLAYERS || (anchor.playerCount > 0 && !anchor.streams))
			return false;

		header.resize(anchor.headerSize);
		if (!readRemote(packet.pid, reinterpret_cast<uintptr_t>(anchor.header), header.data(), header.size()))
			return false;

		std::vector<CrashReplayStream> streams(anchor.playerCount);

		if (anchor.playerCount && !readRemote(
			packet.pid,
			reinterpret_cast<uintptr_t>(anchor.streams),
			streams.data(),
			streams.size() * sizeof(CrashReplayStream)
		))
			return false;
		encoded.resize(anchor.playerCount);
		for (uint32_t i = 0; i < anchor.playerCount; i++) {
			if (streams[i].size > MAX_CRASH_STREAM || (streams[i].size && !streams[i].data))
				return false;
			encoded[i].resize(streams[i].size);
			if (streams[i].size && !readRemote(
				packet.pid,
				reinterpret_cast<uintptr_t>(streams[i].data),
				encoded[i].data(),
				encoded[i].size()
			))
				return false;
		}
		tick = anchor.capturedTick();
	}
	return ReplayData::saveRecovered(header.data(), header.size(), tick, encoded, dir / "replay.replay");
}

bool extractCrashState(const CrashPacket &packet, const std::filesystem::path &dir)
{
	if (!packet.stateAnchor)
		return false;

	std::vector<uint8_t> bytes;

	{
		FrozenThreads frozen;

		if (!freezeThreads(packet.pid, frozen))
			return false;

		CrashStateAnchor anchor{};

		if (!readRemote(packet.pid, reinterpret_cast<uintptr_t>(packet.stateAnchor), &anchor, sizeof(anchor)))
			return false;
		if (anchor.magic != CrashStateAnchor::MAGIC || !anchor.data || anchor.size == 0 || anchor.size > MAX_CRASH_STATE)
			return false;
		bytes.resize(anchor.size);
		if (!readRemote(packet.pid, reinterpret_cast<uintptr_t>(anchor.data), bytes.data(), bytes.size()))
			return false;
	}

	std::ofstream stream(dir / "state.state", std::ios::binary);

	if (!stream)
		return false;
	stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	return static_cast<bool>(stream);
}

CrashPacket *allocCrashPacket()
{
	return new CrashPacket;
}