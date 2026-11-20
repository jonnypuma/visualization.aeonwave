#!/bin/sh
# Builds Aeon Fuse Waveform Visualizer for every platform inside debian:bookworm (see README.md):
#   linux-arm64   aarch64, OpenGL ES   (CoreELEC / LibreELEC 64-bit, Raspberry Pi 4/5, other ARM64 boxes)
#   linux-armv7   armhf,   OpenGL ES   (32-bit LibreELEC / CoreELEC, older Raspberry Pi images)
#   linux-x86_64  x86-64,  OpenGL      (desktop Linux, LibreELEC Generic)
#   windows-x64   x86-64,  DirectX 11  (Kodi for Windows; MinGW, static, shaders compiled at start-up)
#   docker run --rm -v "<this folder>:/src" -v "<kodi headers>:/src/kodi" debian:bookworm sh /src/build.sh
# Out: dist/<platform>/visualization.aeonwave.<platform>/ and dist/visualization.aeonwave.<platform>-<version>.zip
#      One add-on per build since 1.5.2: visualization.aeonwave.linux-arm64 / .linux-armv7 / .linux-x86_64 / .windows-x64
#      (the Skippy Repo serves one zip per id, and the three Linux builds all say <platform>linux</platform>).
set -e
export DEBIAN_FRONTEND=noninteractive
VERSION=1.5.2
if ! command -v x86_64-w64-mingw32-g++ >/dev/null; then
  apt-get update -qq
  apt-get install -y -qq --no-install-recommends g++ g++-aarch64-linux-gnu g++-arm-linux-gnueabihf \
    g++-mingw-w64-x86-64 libglm-dev libgles-dev libgl-dev mesa-common-dev zip >/dev/null
fi
cd /src
rm -rf dist work && mkdir -p dist work
KODI=/src/kodi/xbmc/addons/kodi-dev-kit/include
COMMON="-std=c++17 -O2 -Wall"

# addon.xml of one platform from the template (the template has the Linux line): the id, the name, the summary and the
# description carry the platform (visualization.aeonwave.<platform>, "Aeon Fuse Waveform Visualizer (<label>)")
addon_xml() { # $1 dir, $2 library attribute, $3 library file, $4 platform, $5 id suffix, $6 label, $7 build sentence
  sed -e "s|id=\"visualization.aeonwave\" version=\"[0-9.]*\" name=\"Aeon Fuse Waveform Visualizer\"|id=\"visualization.aeonwave.$5\" version=\"$VERSION\" name=\"Aeon Fuse Waveform Visualizer ($6)\"|" \
      -e "s|library_linux=\"[^\"]*\"|$2=\"$3\"|" \
      -e "s|<platform>linux</platform>|<platform>$4</platform>|" \
      -e "s|\(<summary lang=\"en_GB\">[^<]*\)</summary>|\1 ($6)</summary>|" \
      -e "s|<description lang=\"en_GB\">\([^<]*\)</description>|<description lang=\"en_GB\">$7 \1 Install the build for your device: the same add-on is published for Linux ARM64, Linux ARM 32-bit, Linux x86-64 and Windows x64.</description>|" \
      visualization.aeonwave/addon.xml > "$1/addon.xml"
}

package() { # $1 platform, $2 library file (built in work/$1), $3 library attribute, $4 kodi platform, $5 shaders,
           # $6 label, $7 build sentence (see addon_xml)
  d=dist/$1/visualization.aeonwave.$1
  mkdir -p "$d/resources"
  cp visualization.aeonwave/LICENSE.md "$d/"
  cp -r visualization.aeonwave/resources/language visualization.aeonwave/resources/settings.xml \
        visualization.aeonwave/resources/icon.png visualization.aeonwave/resources/fanart.jpg "$d/resources/"
  if [ -n "$5" ]; then mkdir -p "$d/resources/shaders" && cp -r "visualization.aeonwave/resources/shaders/$5" "$d/resources/shaders/"; fi
  cp "work/$1/$2" "$d/"
  addon_xml "$d" "$3" "$2" "$4" "$1" "$6" "$7"
  (cd "dist/$1" && zip -qr "../visualization.aeonwave.$1-$VERSION.zip" "visualization.aeonwave.$1")
  echo "== $1: $(ls -l "$d/$2" | awk '{print $5}') bytes"
}

linux() { # $1 platform, $2 compiler prefix, $3 GL flag, $4 GL library soname, $5 shaders folder, $6 label, $7 build sentence
  w=work/$1 && mkdir -p "$w"
  ${2}g++ $COMMON -fPIC -fvisibility=hidden $3 -DTARGET_POSIX -DTARGET_LINUX -I"$KODI" -c src/Main_gl.cpp -o "$w/Main_gl.o"
  # a stand-in GL library exporting the gl* functions the add-on calls: the add-on gets DT_NEEDED on the system's
  # GL library (by its soname) and the real one answers at run time
  ${2}nm -u "$w/Main_gl.o" | awk '{print $NF}' | grep '^gl' | sort -u > "$w/glsyms.txt"
  for s in $(cat "$w/glsyms.txt"); do echo "void $s(void){}"; done > "$w/stub.c"
  ${2}gcc -shared -fPIC -Wl,-soname,"$4" -o "$w/libstubgl.so" "$w/stub.c"
  ${2}g++ -shared -Wl,-z,defs -Wl,--as-needed -o "$w/visualization.aeonwave.so.$VERSION" "$w/Main_gl.o" \
    -L"$w" -lstubgl -lm
  ${2}strip --strip-unneeded "$w/visualization.aeonwave.so.$VERSION"
  ${2}readelf -d "$w/visualization.aeonwave.so.$VERSION" | grep NEEDED | sed 's/^/   /'
  package "$1" "visualization.aeonwave.so.$VERSION" library_linux linux "$5" "$6" "$7"
}

linux linux-arm64 aarch64-linux-gnu- -DHAS_GLES=2 libGLESv2.so.2 GLES "Linux ARM64" \
  "Build for Linux ARM64 (64-bit ARM, OpenGL ES): CoreELEC / LibreELEC 64-bit, Raspberry Pi 4 and 5."
linux linux-armv7 arm-linux-gnueabihf- -DHAS_GLES=2 libGLESv2.so.2 GLES "Linux ARM 32-bit" \
  "Build for Linux ARM 32-bit (OpenGL ES): 32-bit LibreELEC / CoreELEC images."
linux linux-x86_64 "" -DHAS_GL libGL.so.1 GL "Linux x86-64" \
  "Build for Linux x86-64 (OpenGL): desktop Linux, LibreELEC Generic."

# Windows x64, DirectX 11
w=work/windows-x64 && mkdir -p "$w"
x86_64-w64-mingw32-g++ $COMMON -DTARGET_WINDOWS -DHAS_DX -I"$KODI" -shared -static -static-libgcc -static-libstdc++ \
  -o "$w/visualization.aeonwave.dll" src/Main_dx.cpp
x86_64-w64-mingw32-strip --strip-unneeded "$w/visualization.aeonwave.dll"
x86_64-w64-mingw32-objdump -p "$w/visualization.aeonwave.dll" | grep -i "DLL Name" | sed 's/^/   /'
package windows-x64 visualization.aeonwave.dll library_windx windx "" "Windows x64" \
  "Build for Windows x64 (DirectX 11): Kodi for Windows."

rm -rf work
ls -l dist/*.zip
