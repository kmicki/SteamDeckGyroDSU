./build.sh

result=$?

if [ "$result" -ne 0 ]; then
    exit $result
fi

mkdir -p bin/pkg
rm -rf bin/pkg/*

cp bin/release/* bin/pkg/
cp pkg/* bin/pkg/

mkdir -p pkgbin/
rm -rf pkgbin/*
mkdir -p pkgbin/SteamDeckGyroDSUSetup
cp bin/pkg/* pkgbin/SteamDeckGyroDSUSetup/
rm -rf bin/pkg

cd pkgbin
zip -r SteamDeckGyroDSUSetup.zip SteamDeckGyroDSUSetup
rm -rf SteamDeckGyroDSUSetup

