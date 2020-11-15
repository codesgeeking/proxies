#!/bin/sh
set -e
scriptDir=$(
  cd $(dirname $0)
  pwd
)
cp -f "$scriptDir"/proxies /etc/init.d/proxies
/etc/init.d/proxies  disable
/etc/init.d/proxies  stop
sh ${scriptDir}/../nat/rule.sh clean
echo "proxies service stop success!"