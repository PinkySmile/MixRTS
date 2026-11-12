#!/usr/bin/env python3
"""Dump a MixRTS .replay file as text. See documentation/REPLAY.md."""

import argparse
import struct
import sys


MAGIC = 0x7D4E82AF
VERSION = 1
FLAG_SHORT_GAME = 1 << 0
FLAG_RANDOM_SPAWN = 1 << 1
FLAG_INPUTS = 1 << 7

# Bits 0–100 follow sf::Keyboard::Key, starting at A = 0. Bit 100 is Pause.
KEY_NAMES = (
	"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
	"N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
	"Num0", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8", "Num9",
	"Escape", "LControl", "LShift", "LAlt", "LSystem",
	"RControl", "RShift", "RAlt", "RSystem", "Menu",
	"LBracket", "RBracket", "Semicolon", "Comma", "Period", "Apostrophe",
	"Slash", "Backslash", "Grave", "Equal", "Hyphen",
	"Space", "Enter", "Backspace", "Tab",
	"PageUp", "PageDown", "End", "Home", "Insert", "Delete",
	"Add", "Subtract", "Multiply", "Divide",
	"Left", "Right", "Up", "Down",
	"Numpad0", "Numpad1", "Numpad2", "Numpad3", "Numpad4",
	"Numpad5", "Numpad6", "Numpad7", "Numpad8", "Numpad9",
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
	"F11", "F12", "F13", "F14", "F15", "Pause",
)
MOUSE_NAMES = ("Left", "Right", "Middle", "Extra1", "Extra2")

ACTION_NAMES = {
	0: "StartBuild",
	1: "PlaceBuild",
	2: "CancelBuild",
	3: "Deploy",
	4: "Move",
	5: "Harvest",
	6: "Dump",
	7: "SetPrimary",
	8: "Repair",
	9: "Sell",
	10: "Attack",
	11: "Stop",
	12: "OpenQueue",
	13: "CloseQueue",
	200: "ScreenSizeChanged",
	201: "SettingsChanged",
	202: "UIChanged",
}


class ReplayError(Exception):
	pass


class Reader:
	def __init__(self, data):
		self.data = data
		self.i = 0

	def _need(self, size):
		if self.i + size > len(self.data):
			raise ReplayError(f"truncated replay at offset {self.i}: need {size} more byte(s)")

	def u8(self):
		self._need(1)
		value = self.data[self.i]
		self.i += 1
		return value

	def u16(self):
		self._need(2)
		value = struct.unpack_from("<H", self.data, self.i)[0]
		self.i += 2
		return value

	def u32(self):
		self._need(4)
		value = struct.unpack_from("<I", self.data, self.i)[0]
		self.i += 4
		return value

	def f32(self):
		self._need(4)
		value = struct.unpack_from("<f", self.data, self.i)[0]
		self.i += 4
		return value

	def bytes(self, size):
		self._need(size)
		value = self.data[self.i:self.i + size]
		self.i += size
		return value

	def varint(self):
		value = 0
		shift = 0
		for _ in range(10):
			byte = self.u8()
			value |= (byte & 0x7F) << shift
			if (byte & 0x80) == 0:
				return value
			shift += 7
		raise ReplayError(f"replay varint is too long at offset {self.i}")

	def string(self):
		length = self.varint()
		if length > 1_000_000:
			raise ReplayError(f"replay string is too long ({length} bytes) at offset {self.i}")
		raw = self.bytes(length)
		try:
			return raw.decode("utf-8")
		except UnicodeDecodeError:
			return raw.decode("utf-8", errors="backslashreplace")

	def vec2f(self):
		return (self.f32(), self.f32())

	def vec2s(self):
		return (self.u16(), self.u16())

	def indices(self):
		length = self.varint()
		if length > 100_000:
			raise ReplayError(f"replay unit list is too long ({length}) at offset {self.i}")
		return [self.u32() for _ in range(length)]


def fmt_float(value):
	return format(value, ".9g")


def fmt_vec(value):
	return f"({fmt_float(value[0])}, {fmt_float(value[1])})"


def fmt_flags(flags):
	names = []
	if flags & FLAG_SHORT_GAME:
		names.append("short game")
	if flags & FLAG_RANDOM_SPAWN:
		names.append("random spawn")
	if flags & FLAG_INPUTS:
		names.append("player inputs")
	unknown = flags & ~(FLAG_SHORT_GAME | FLAG_RANDOM_SPAWN | FLAG_INPUTS)
	if unknown:
		names.append(f"unknown 0x{unknown:02X}")
	return ", ".join(names) if names else "none"


def read_input(reader):
	mouse = (reader.u16() / 65535.0, reader.u16() / 65535.0)
	bits = reader.bytes(16)
	keys = []
	buttons = []
	reserved = []
	for bit in range(126):
		if (bits[bit // 8] >> (bit % 8)) & 1:
			if bit < len(KEY_NAMES):
				keys.append(KEY_NAMES[bit])
			elif 101 <= bit <= 105:
				buttons.append(MOUSE_NAMES[bit - 101])
			else:
				reserved.append(bit)
	camera_pos = reader.vec2s() if (bits[15] >> 6) & 1 else None
	camera_size = reader.vec2s() if (bits[15] >> 7) & 1 else None
	return {
		"mouse": mouse,
		"keys": keys,
		"buttons": buttons,
		"reserved": reserved,
		"camera_pos": camera_pos,
		"camera_size": camera_size,
	}


def input_is_empty(state):
	return not (
		state["keys"] or state["buttons"] or state["reserved"]
		or state["camera_pos"] is not None or state["camera_size"] is not None
		or state["mouse"] != (0.0, 0.0)
	)


def fmt_input(state):
	parts = [f"mouse {fmt_vec(state['mouse'])}"]
	if state["keys"]:
		parts.append("keys " + ",".join(state["keys"]))
	if state["buttons"]:
		parts.append("mouse buttons " + ",".join(state["buttons"]))
	if state["reserved"]:
		parts.append("reserved bits " + ",".join(str(bit) for bit in state["reserved"]))
	if state["camera_pos"] is not None:
		parts.append(f"camera {fmt_vec(state['camera_pos'])}")
	if state["camera_size"] is not None:
		parts.append(f"camera size {fmt_vec(state['camera_size'])}")
	return " ".join(parts)


def read_action(reader):
	action_id = reader.u8()
	name = ACTION_NAMES.get(action_id)
	if name is None:
		raise ReplayError(f"unknown replay action {action_id} at offset {reader.i - 1}")
	if action_id in (0, 2):
		detail = f"{reader.string()} queue {reader.string()}"
	elif action_id == 1:
		object_id = reader.string()
		queue = reader.string()
		detail = f"{object_id} queue {queue} at {fmt_vec(reader.vec2s())}"
	elif action_id in (3, 7, 8, 9):
		detail = f"object {reader.u32()}"
	elif action_id in (4, 5):
		units = reader.indices()
		detail = f"units {units} to {fmt_vec(reader.vec2s())}"
	elif action_id == 6:
		units = reader.indices()
		detail = f"units {units} into object {reader.u32()}"
	elif action_id == 10:
		units = reader.indices()
		detail = f"units {units} target {reader.u32()}"
	elif action_id == 11:
		detail = f"units {reader.indices()}"
	elif action_id in (12, 13):
		detail = reader.string()
	elif action_id == 200:
		detail = fmt_vec(reader.vec2s())
	elif action_id == 201:
		detail = f"scale {fmt_float(reader.f32())}"
	else:
		detail = f"ui {reader.u32()}"
	return f"{name} {detail}"


def load_replay(path):
	with open(path, "rb") as handle:
		data = handle.read()
	reader = Reader(data)
	magic = reader.u32()
	if magic != MAGIC:
		raise ReplayError(f"not a replay file (magic 0x{magic:08X})")
	version = reader.u8()
	if version != VERSION:
		raise ReplayError(f"unsupported replay version {version}")

	replay = {
		"seed": reader.u32(),
		"map": reader.string(),
		"map_crc": reader.u32(),
		"speed": reader.f32(),
		"flags": reader.u8(),
		"players": [],
		"frames": [],
	}
	player_count = reader.u8()
	for _ in range(player_count):
		replay["players"].append({
			"name": reader.string(),
			"faction": reader.string(),
			"faction_crc": reader.u32(),
			"spawn": reader.vec2f(),
			"color": (reader.u16(), reader.u16(), reader.u16()),
			"team": reader.u8(),
		})
	frame_count = reader.u32()
	has_inputs = replay["flags"] & FLAG_INPUTS
	for _ in range(frame_count):
		frame = []
		for _player in range(player_count):
			state = read_input(reader) if has_inputs else None
			count = reader.u8()
			actions = [read_action(reader) for _action in range(count)]
			frame.append({"input": state, "actions": actions})
		replay["frames"].append(frame)
	replay["trailing"] = len(data) - reader.i
	replay["size"] = len(data)
	return replay


def dump_replay(replay, all_frames, out):
	speed = replay["speed"]
	frame_count = len(replay["frames"])
	duration = frame_count / (50.0 * speed) if speed else 0.0
	print(f"size: {replay['size']} bytes", file=out)
	print(f"version: {VERSION}", file=out)
	print(f"seed: {replay['seed']}", file=out)
	print(f"map: {replay['map']}", file=out)
	print(f"map crc: 0x{replay['map_crc']:08X}", file=out)
	print(f"speed: {fmt_float(speed)}", file=out)
	print(f"flags: {fmt_flags(replay['flags'])}", file=out)
	print(f"frames: {frame_count} ({fmt_float(duration)} s)", file=out)
	print(f"players: {len(replay['players'])}", file=out)
	for index, player in enumerate(replay["players"], start=1):
		hue, saturation, lightness = player["color"]
		print(
			f"  {index}. {player['name']}  {player['faction']}"
			f"  crc 0x{player['faction_crc']:08X}"
			f"  spawn {fmt_vec(player['spawn'])}"
			f"  hsl ({hue}, {saturation}, {lightness})"
			f"  team {player['team']}",
			file=out,
		)
	if replay["trailing"]:
		print(f"trailing bytes: {replay['trailing']}", file=out)

	shown = 0
	for tick, frame in enumerate(replay["frames"], start=1):
		lines = []
		for index, player_frame in enumerate(frame):
			state = player_frame["input"]
			if state is not None and not input_is_empty(state):
				lines.append(f"  player {index + 1} input {fmt_input(state)}")
			for action in player_frame["actions"]:
				lines.append(f"  player {index + 1} {action}")
		if not lines and not all_frames:
			continue
		shown += 1
		print(f"tick {tick}", file=out)
		for line in lines:
			print(line, file=out)
	if not all_frames:
		print(f"empty ticks skipped: {frame_count - shown}", file=out)


def main(argv):
	parser = argparse.ArgumentParser(description="Dump a MixRTS replay file as text.")
	parser.add_argument("replay", help="path to a .replay file")
	parser.add_argument("--all", action="store_true", help="also print ticks that contain no actions")
	args = parser.parse_args(argv)
	try:
		replay = load_replay(args.replay)
	except (OSError, ReplayError) as error:
		print(f"error: {error}", file=sys.stderr)
		return 1
	dump_replay(replay, args.all, sys.stdout)
	return 0


if __name__ == "__main__":
	sys.exit(main(sys.argv[1:]))
