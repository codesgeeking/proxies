#!/bin/sh
set -e
scriptDir=$(
  cd $(dirname $0)
  pwd
)
cp -f "$scriptDir"/proxies /etc/init.d/proxies
/etc/init.d/proxies  disable
/etc/init.d/proxies  stop
echo "proxies service stop success!"