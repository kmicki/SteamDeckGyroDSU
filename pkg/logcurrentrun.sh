#!/bin/sh

journalctl --user -u sdgyrodsu -b | grep "\[$(systemctl --user status sdgyrodsu | grep 'Main PID' | sed 's/.*PID: \([0-9]\+\).*/\1/')\]" --color=never
