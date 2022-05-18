./prepare.sh

echo "Building source..."
g++ -std=c++2a $(find inc -type d -printf '-I %p\n') -g $(find src -type f -iregex '.*\.cpp' -printf '%p\n') -pthread -lncurses -o bin/sdgyrodsu

result=$?

if [ "$result" -ne 0 ]; then
    echo -e "\e[1mError: source was not built.\e[0m"
    exit $result
else
    echo -e "\e[1mBuilding complete.\e[0m"
fi
