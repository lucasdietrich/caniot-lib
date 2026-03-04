#!/bin/sh
set -eu

usage() {
  echo "Usage: $0 <vcan_ifname> <remote_host> <remote_port> [debug_flags]" >&2
  echo "Example: $0 vcan0 192.168.10.49 20000 cubt" >&2
  exit 1
}

[ "${1:-}" ] || usage
[ "${2:-}" ] || usage
[ "${3:-}" ] || usage

IFACE="$1"
REMOTE_HOST="$2"
REMOTE_PORT="$3"
DEBUG_FLAGS="${4:-}"

IP_BIN="$(command -v ip || true)"
CANNELLONI_BIN="$(command -v cannelloni || true)"
MODPROBE_BIN="$(command -v modprobe || true)"

[ -n "$IP_BIN" ] || { echo "Error: 'ip' command not found." >&2; exit 127; }
[ -n "$CANNELLONI_BIN" ] || { echo "Error: 'cannelloni' not found in PATH." >&2; exit 127; }

ensure_module() {
  if [ -n "$MODPROBE_BIN" ] && ! "$IP_BIN" link show type vcan >/dev/null 2>&1; then
    "$MODPROBE_BIN" vcan >/dev/null 2>&1 || true
  fi
}

ensure_vcan() {
  IF="$1"
  if ! "$IP_BIN" link show "$IF" >/dev/null 2>&1; then
    echo "Creating $IF (vcan)…"
    "$IP_BIN" link add dev "$IF" type vcan
  fi
  "$IP_BIN" link set "$IF" mtu 16 || true
  "$IP_BIN" link set "$IF" up
}

# --- Main ---
ensure_module
ensure_vcan "$IFACE"

echo
echo "Starting cannelloni on $IFACE → ${REMOTE_HOST}:${REMOTE_PORT}"
echo "Press Ctrl+C to stop."
echo

# run cannelloni in foreground, showing logs in the same shell
# shellcheck disable=SC2086
exec "$CANNELLONI_BIN" -I "$IFACE" -C c -R "$REMOTE_HOST" -r "$REMOTE_PORT" ${DEBUG_FLAGS:+-d "$DEBUG_FLAGS"}
