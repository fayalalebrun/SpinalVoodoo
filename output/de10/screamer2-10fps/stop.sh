set -euo pipefail
r=/dev/shm/screamer2-50mhz
test "$(cat /proc/sys/kernel/random/boot_id)" = 32bbda2e-4586-4ff7-b759-bff08e8cd7d7
p=$(cat "$r/dosbox.pid")
x=$(cat "$r/xvfb.pid")
case "$(sudo -n readlink /proc/$p/exe)" in
    /home/fpga/dosbox-x-2025.10.07-gcc-armv7|/dev/shm/screamer2-50mhz/dosbox-x-o3) ;;
    *) exit 1 ;;
esac
test "$(cat /proc/$x/comm)" = Xvfb
sudo -n kill -CONT "$p"
sudo -n kill -TERM "$p"
sleep 2
test ! -e "/proc/$p"
kill -TERM "$x"
sleep 1
test ! -e "/proc/$x"
printf 'STOPPED %s %s\n' "$p" "$x"
