./build.sh

systemctl --user stop sdgyrodsu.service
rm $HOME/sdgyrodsu/sdgyrodsu
cp bin/sdgyrodsu $HOME/sdgyrodsu/
systemctl --user enable --now sdgyrodsu.service

