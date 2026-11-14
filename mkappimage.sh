BUILD_DIR="$1"
OUTPUT="$2"

if [ -z "$BUILD_DIR" ]; then
	BUILD_DIR=cmake-build-release
fi
if [ -z "$OUTPUT" ]; then
	OUTPUT=appimage_folder
fi

cmake --build "$BUILD_DIR" --target MixRTS -- -j $(nproc)
mkdir -p "$OUTPUT/usr/bin" "$OUTPUT/usr/lib" "$OUTPUT/usr/share/mixrts" "$OUTPUT/usr/share/applications" "$OUTPUT/usr/share/icons/hicolor/64x64/apps"
cp "$BUILD_DIR/MixRTS" "$OUTPUT/usr/bin"
cp "$BUILD_DIR/libMixRTS_Core.so" "$OUTPUT/usr/lib"

cp -r assets "$OUTPUT/usr/share/mixrts"
cp -r "$BUILD_DIR/mods" "$OUTPUT/usr/share/mixrts"
rm -rf "$OUTPUT/usr/share/mixrts/mods/"*/*.cmake
rm -rf "$OUTPUT/usr/share/mixrts/mods/"*/Makefile
rm -rf "$OUTPUT/usr/share/mixrts/mods/"*/CMakeFiles
rm -rf "$OUTPUT/usr/share/mixrts/mods/"*/*.so

cp assets/icon.png "$OUTPUT/usr/share/icons/hicolor/64x64/apps/mixrts.png"
echo "[Desktop Entry]
Type=Application
Name=MixRTS
Exec=MixRTS
Comment=Open source RTS engine, written in C++
Icon=mixrts
Categories=Game" > "$OUTPUT/usr/share/applications/mixrts.desktop"
echo '#!/bin/sh
HERE="$(dirname -- "$(readlink -f -- "$0")")"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${HERE}/usr/lib"
cd "$HERE"/usr/share/mixrts
exec "$HERE"/usr/bin/MixRTS "$@"' > "$OUTPUT/AppRun"
chmod +x "$OUTPUT/AppRun"
ln -s usr/share/icons/hicolor/64x64/apps/mixrts.png "$OUTPUT/.DirIcon"
ln -s usr/share/icons/hicolor/64x64/apps/mixrts.png "$OUTPUT/mixrts.png"
ln -s usr/share/applications/mixrts.desktop "$OUTPUT/mixrts.desktop"
ldd "$OUTPUT/usr/bin/MixRTS" | tail -n +2 | sort | tail -n +2 | cut -d '>' -f 2 | cut -d '(' -f 1 | cut -d ' ' -f 2 | while read line; do
	if printf %s "$line" | grep -v libMixRTS_Core.so; then
		cat "$line" > "$OUTPUT/usr/lib/$(basename "$line")"
	fi
done
ldd "$OUTPUT/usr/lib/libMixRTS_Core.so" | tail -n +2 | sort | tail -n +2 | cut -d '>' -f 2 | cut -d '(' -f 1 | cut -d ' ' -f 2 | while read line; do
	if ! [ -z "$line" ] && printf "%s\n" "$line"; then
		cat "$line" > "$OUTPUT/usr/lib/$(basename "$line")"
	fi
done
ARCH=x86_64 appimagetool "$OUTPUT" mixrts.AppImage
rm -rf "$OUTPUT"