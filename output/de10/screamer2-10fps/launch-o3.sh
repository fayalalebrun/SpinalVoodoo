set -euo pipefail
r=/dev/shm/screamer2-50mhz
test "$(cat /proc/sys/kernel/random/boot_id)" = 32bbda2e-4586-4ff7-b759-bff08e8cd7d7
test ! -S /tmp/.X11-unix/X1
test -x "$r/dosbox-x-o3"
if pgrep -f '^/home/fpga/dosbox-x-2025.10.07-gcc-armv7|^/dev/shm/screamer2-50mhz/dosbox-x-o3' >/dev/null; then exit 1; fi
printf '%s  %s\n' a003437b5e7d02f78289b27591170de030cf399257c4663d7e8d63c0ba7e8b28 "$r/lib/libglide2x.so.2" | sha256sum -c -
nohup Xvfb :1 -screen 0 800x600x16 -nolisten tcp > "$r/xvfb.log" 2>&1 < /dev/null &
echo $! > "$r/xvfb.pid"
sleep 2
sudo -n env HOME="$r/home" XDG_CONFIG_HOME="$r/home" XDG_CACHE_HOME="$r/home" TMPDIR="$r/tmp" DISPLAY=:1 LD_LIBRARY_PATH="$r/lib" SDL_AUDIODRIVER=dummy "$r/dosbox-x-o3" -conf "$r/dosbox.conf" -c exit > "$r/o3-startup.log" 2>&1
if grep -Ei 'selftest.*fail|bitfield.*fail' "$r/o3-startup.log"; then exit 1; fi
sudo -n sh -c 'r=/dev/shm/screamer2-50mhz; nohup env HOME="$r/home" XDG_CONFIG_HOME="$r/home" XDG_CACHE_HOME="$r/home" TMPDIR="$r/tmp" DISPLAY=:1 LD_LIBRARY_PATH="$r/lib" DE10_BATCH=0 DE10_GLIDE_X11=0 GLIDE_X11_ENABLE=0 DE10_INIT_STAGE_FILE=/dev/null SDL_AUDIODRIVER=dummy "$r/dosbox-x-o3" -conf "$r/dosbox.conf" -c "mount c $r/game" -c "imgmount d $r/game/disc.iso -t iso" -c "c:" -c S2_3DFX.EXE > /dev/null 2>&1 < /dev/null & echo $! > "$r/dosbox.pid"'
printf 'DOSBOX '; cat "$r/dosbox.pid"
printf 'XVFB '; cat "$r/xvfb.pid"
