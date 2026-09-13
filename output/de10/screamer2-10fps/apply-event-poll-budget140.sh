set -euo pipefail
r=/dev/shm/screamer2-50mhz
p=$(cat "$r/dosbox.pid")
case "$p" in ''|*[!0-9]*) exit 1;; esac
test "$p" -gt 1
test "$(cat /proc/sys/kernel/random/boot_id)" = 32bbda2e-4586-4ff7-b759-bff08e8cd7d7
test "$(sudo -n readlink /proc/"$p"/exe)" = "$r/dosbox-x-o3"
grep -Eq '^State:[[:space:]]+T[[:space:]]' "/proc/$p/status"
printf '%s  %s\n' fb9db5530b4cb499aacfb3463232e2dca96d5162290ea505d2601350d8b38fc0 "$r/dosbox-x-o3" 603a9e606f59e51cf560a28b2c71c98af37c17345190db64cfa210a1110eab36 "$r/dosbox.conf" 7be828f5380912fd5f818b978f0d6e3a06ed5dd4b9ebe0c774de387f66c475d4 "$r/game/SCREAMER.CFG" a003437b5e7d02f78289b27591170de030cf399257c4663d7e8d63c0ba7e8b28 "$r/lib/libglide2x.so.2" | sha256sum -c -
trap 'sudo -n kill -STOP "$p"' EXIT
sudo -n timeout -s INT -k 5 45 gdb -nx -q -batch -p "$p" -ex 'p *(int *)&CPU_CyclePercUsed' -ex 'p *(unsigned char *)&CPU_CycleAutoAdjust' -ex 'p *(unsigned char *)&CPU_SkipCycleAutoAdjust' -ex 'python assert int(gdb.parse_and_eval("*(unsigned char *)&CPU_CycleAutoAdjust")) == 1; assert int(gdb.parse_and_eval("*(unsigned char *)&CPU_SkipCycleAutoAdjust")) == 0; gdb.execute("set *(int *)&CPU_CyclePercUsed = 140"); assert int(gdb.parse_and_eval("*(int *)&CPU_CyclePercUsed")) == 140' -ex 'p *(int *)&CPU_CyclePercUsed' -ex detach
