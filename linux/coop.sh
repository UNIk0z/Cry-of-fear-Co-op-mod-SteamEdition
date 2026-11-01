#!/usr/bin/env bash
# Cry of Fear Co-op for Linux (Steam + Proton). Run: bash coop.sh
# or straight away: bash coop.sh install | run | uninstall | check
cd "$(dirname "$(readlink -f "$0")")" || exit 1
if ! command -v python3 >/dev/null 2>&1; then
  echo "Python 3 is needed (it comes with almost every Linux). Install the python3 package, then try again."
  exit 1
fi
if [ $# -gt 0 ]; then exec python3 files/coop-linux.py "$@"; fi
echo "Cry of Fear Co-op (Linux)"
echo "  1) Install or update   (close the game first)"
echo "  2) Run Co-op Mod"
echo "  3) Uninstall"
echo "  4) Quit"
read -r -p "Choose 1-4: " choice
case "$choice" in
  1) python3 files/coop-linux.py install ;;
  2) python3 files/coop-linux.py run ;;
  3) python3 files/coop-linux.py uninstall ;;
  *) exit 0 ;;
esac
status=$?
read -r -p "Press Enter to close." _
exit $status
