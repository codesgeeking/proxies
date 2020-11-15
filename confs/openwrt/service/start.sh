#!/bin/sh
set -e
scriptDir=$(
  cd $(dirname $0)
  pwd
)
cp -f "$scriptDir"/proxies /etc/init.d/proxies
/etc/init.d/proxies  enable
/etc/init.d/proxies  start
echo "proxies service start success!"
