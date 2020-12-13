#!/bin/sh
scriptDir=$(
  cd $(dirname $0)
  pwd
)
cp -f ${scriptDir}/proxies.service /usr/lib/systemd/system/proxies.service
systemctl daemon-reload  
systemctl stop proxies  
systemctl disable proxies  
sleep 3s
systemctl status proxies 
