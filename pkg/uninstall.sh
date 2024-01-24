#!/bin/sh

echo "Uninstalling the service"
systemctl --user -q stop sdgyrodsu.service >/dev/null 2>&1
systemctl --user -q disable sdgyrodsu.service >/dev/null 2>&1
rm $HOME/.config/systemd/user/sdgyrodsu.service >/dev/null 2>&1

echo "Removing files"
rm $HOME/sdgyrodsu/sdgyrodsu >/dev/null 2>&1
rm $HOME/sdgyrodsu/update.sh >/dev/null 2>&1
rm $HOME/sdgyrodsu/uninstall.sh >/dev/null 2>&1
rm $HOME/sdgyrodsu/v1cleanup.sh >/dev/null 2>&1
rm $HOME/sdgyrodsu/logcurrentrun.sh >/dev/null 2>&1
rm $HOME/Desktop/uninstall-sdgyrodsu.desktop >/dev/null 2>&1
rm -d $HOME/sdgyrodsu >/dev/null 2>&1

echo "Uninstalling complete."

read -n 1 -s -r -p "Finished. Press any key to exit."
echo " "
