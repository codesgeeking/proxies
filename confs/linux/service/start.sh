scriptDir=$(
  cd $(dirname $0)
  pwd
)
cp -f ${scriptDir}/proxies.service /usr/lib/systemd/system/proxies.service
systemctl daemon-reload 
systemctl enable proxies 
systemctl reset-failed proxies.service  
systemctl start proxies 
sleep 3s
systemctl status proxies
