import os
import sys
import json
import random
from PIL import Image


GRIDMASK_RESOURCE          = 0xFF000000
GRIDMASK_HAS_CONCRETE      = 0x00100000
GRIDMASK_IS_FREE           = 0x00080000
GRIDMASK_BUILDABLE         = 0x00040000
GRIDMASK_CRATER_TYPE       = 0x00030000
GRIDMASK_TERRAIN_TYPE      = 0x0000F000
GRIDMASK_SHIP_CAN_PASS     = 0x00000008
GRIDMASK_WORM_CAN_PASS     = 0x00000004
GRIDMASK_VEHICLES_CAN_PASS = 0x00000002
GRIDMASK_INFANTRY_CAN_PASS = 0x00000001


def GRID_RESOURCE_TYPE(i: int):
	return ((i & GRIDMASK_RESOURCE) >> 24) & 0x1F


def HAS_RESOURCE(lmap, x: int, y: int):
	if x >= lmap["size"][0] or y >= lmap["size"][1]:
		return True
	return GRID_RESOURCE_TYPE(lmap["grid"][x + y * lmap["size"][0]]) != 0


def refreshTile(minimap: Image, lmap: dict, x: int, y: int):
	index = lmap["tiles"][x + y * lmap["size"][0]]
	attrs = lmap["grid"][x + y * lmap["size"][0]]
	res = GRID_RESOURCE_TYPE(attrs)

	if res != 0:
		r = lmap["tileset"]["resources"][res]
		if r["strategy"] == "random":
			index = random.choice(r["tile_pool"])
		elif r["strategy"] == "connected":
			index = 0
			index |= HAS_RESOURCE(lmap, x - 1, y + 1) << 0
			index |= HAS_RESOURCE(lmap, x,     y + 1) << 1
			index |= HAS_RESOURCE(lmap, x + 1, y + 1) << 2
			index |= HAS_RESOURCE(lmap, x - 1, y)     << 3
			index |= HAS_RESOURCE(lmap, x + 1, y)     << 4
			index |= HAS_RESOURCE(lmap, x - 1, y - 1) << 5
			index |= HAS_RESOURCE(lmap, x,     y - 1) << 6
			index |= HAS_RESOURCE(lmap, x + 1, y - 1) << 7
			index = r["tiles"][index]
	minimap.putpixel((x, y), lmap["tileset"]["colors"][index])


def parseColors(buffer):
	count = buffer[0]
	colors = [(buffer[3+i*3], buffer[2+i*3], buffer[1+i*3]) for i in range(count)]
	return [colors[t] for t in buffer[1+count*3:]]


def loadTileset(folder):
	with open(os.path.join(folder, "tileattrs.dat"), "rb") as fd, open(os.path.join(folder, "metadata.json")) as fd2:
		data = json.load(fd2)
		with open(os.path.join(folder, data["map_colors"]), "rb") as fd3:
			colors = fd3.read()
			for r in data["resources"]:
				if r["strategy"] == "connected":
					with open(os.path.join(folder, r["tiles"]), "rb") as fd4:
						d = fd4.read()
						r["tiles"] = [int.from_bytes(d[i*4:(i+1)*4], 'little') for i in range(len(d) // 4)]
			return {
				"folder": folder,
				"attrs": fd.read(),
				"colors": parseColors(colors),
				"size": data["size"],
				"meta": data,
				"resources": {r['id']: r for r in data["resources"]},
				"sheet": Image.open(os.path.join(folder, "tilesheet.png"))
			}


def loadMap(folder):
	with open(folder + "/map.json") as fd:
		jmap = json.load(fd)
	tileset = loadTileset(os.path.join(folder, jmap["tileset"]))
	with open(os.path.join(folder, "tiles.dat"), "rb") as fd:
		tiles = fd.read()
		tiles = [int.from_bytes(tiles[i * 4:i * 4+4], "little") for i in range(len(tiles) // 4)]
		return {
			"mapdata": jmap,
			"size": (jmap["size"]["x"], jmap["size"]["y"]),
			"tile_size": (tileset["size"]["x"], tileset["size"]["y"]),
			"preview": jmap["preview"],
			"pixel_size": (
				jmap["size"]["x"] * tileset["size"]["x"],
				jmap["size"]["y"] * tileset["size"]["y"]
			),
			"tiles": [t & 0xFFFFFF for t in tiles],
			"grid": [tileset["attrs"][t & 0xFFFFFF] | (t & 0xFF000000) for t in tiles],
			"tileset": tileset
		}


if __name__ == '__main__':
	lmap = loadMap(sys.argv[1])
	minimap = Image.new("RGB", lmap["size"])
	for y in range(minimap.size[1]):
		for x in range(minimap.size[0]):
			refreshTile(minimap, lmap, x, y)
	minimap.save(os.path.join(sys.argv[1], "preview.png"))
