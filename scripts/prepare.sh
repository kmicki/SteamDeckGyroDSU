# Required packages
dependencies=( "${@:2:$1}" ); shift "$(( $1 + 1 ))"

# Some header file for each package.
# SteamOS has all required packages installed by default but for some reason header files are missing.
# This will be checked before building.
checks=( "${@:2:$1}" ); shift "$(( $1 + 1 ))"

declare -a reinstall
common_reinstall=false

for i in ${!checks[@]}; do
    if [ -f ${checks[$i]} ]; then
        reinstall[$i]=false
    else
        # header file is missing
	    reinstall[$i]=true
        common_reinstall=true
    fi
done

if [ "$common_reinstall" == true ]; then
    echo "Some dependencies' header files missing."
    # Check if read only filesystem is enabled
    rofs=false
    if sudo steamos-readonly status | grep -q 'enabled'; then
        rofs=true
	echo "Read only filesystem enabled. Disabling..."
	sudo steamos-readonly disable &>/dev/null
    fi
    echo "Initializing package manager..."
    sudo pacman-key --init &>/dev/null
    sudo pacman-key --populate &>/dev/null
    for i in ${!reinstall[@]}; do
        if [ "${reinstall[$i]}" == true ]; then
            echo -e "Reinstalling \e[1m${dependencies[$i]}\e[0m..."
            sudo pacman -S --noconfirm ${dependencies[$i]} &>/dev/null
        fi
    done
    if [ "$rofs" == true ]; then
        echo "Reenabling read only filesystem..."     
	sudo steamos-readonly enable &>/dev/null
    fi
    echo "Required dependencies reinstalled."
else
    echo "All dependencies' header files already installed."
fi

exit 0

