#!/bin/sh

VCAN="vcan1"

case "$1" in
    add)
        sudo ip link add dev $VCAN type vcan
        sudo ip link set $VCAN mtu 16
        sudo ip link set up $VCAN
        ;;
    remove)
        sudo ip link delete $VCAN type vcan
        ;;
    *)
        echo "Usage: $0 {add|remove}"
        exit 1
        ;;
esac
