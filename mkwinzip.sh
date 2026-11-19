#!/bin/sh
BUILD_DIR="$1"
OUTPUT="$2"

if [ -z "$BUILD_DIR" ]; then
	BUILD_DIR=cmake-build-release-mingw-64
fi
if [ -z "$OUTPUT" ]; then
	OUTPUT=winzip_folder
fi

cmake --build "$BUILD_DIR" --target MixRTS -- -j $(nproc) || exit
mkdir "$OUTPUT"
cp "$BUILD_DIR/MixRTS.exe" "$OUTPUT"
cp "$BUILD_DIR/libMixRTS_Core.dll" "$OUTPUT"
cp -r documentation README.md "$OUTPUT"
cp -r assets "$OUTPUT"
cp -r "$BUILD_DIR/mods" "$OUTPUT"
rm -rf "$OUTPUT/mods/"*/*.cmake
rm -rf "$OUTPUT/mods/"*/Makefile
rm -rf "$OUTPUT/mods/"*/CMakeFiles
rm -rf "$OUTPUT/mods/"*/*.dll

(strings "$OUTPUT/"MixRTS.exe; strings "$OUTPUT/"libMixRTS_Core.dll) | grep -i '\.dll$' | sort | uniq | while read line; do
	if
		printf %s "$line" |
			grep -iv '^DBGHELP\.DLL$' |
			grep -iv '^NTDLL\.DLL$' |
			grep -iv '^KERNEL32\.DLL$' |
			grep -iv '^USER32\.DLL$' |
			grep -v '^api-ms-win-' |
			grep -v '^libMixRTS_Core\.dll$' &&
		[ -f /usr/x86_64-w64-mingw32/bin/"$line" ];
	then
		cat /usr/x86_64-w64-mingw32/bin/"$line" > "$OUTPUT/$(basename "$line")"
	fi
done
cd "$OUTPUT"
zip -r ../mixrts.zip *
cd -
rm -rf "$OUTPUT"