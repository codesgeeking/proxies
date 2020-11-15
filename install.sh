set -e
if [ ! -d "/tmp/proxies-build" ]; then
  mkdir /tmp/proxies-build
fi
CMAKE_INSTALL_PREFIX="/usr/local"
if [ "" != "$1" ]; then
  CMAKE_INSTALL_PREFIX=$1
fi
baseDir=$(pwd)
cd /tmp/proxies-build
cmake -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} ${baseDir}
make -j8
make install
