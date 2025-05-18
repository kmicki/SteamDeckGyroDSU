#!/bin/sh

journalctl --user -u sdgyrodsu -b | grep "\[$(systemctl --user show -P MainPID sdgyrodsu)\]" --color=never
