set -e
scriptDir=$(
  cd $(dirname $0)
  pwd
)
ulimit -n 65000
cp -f ${scriptDir}/proxies /etc/init.d/
chmod +x /etc/init.d/proxies
unamestr=$(uname -a | tr 'A-Z' 'a-z')
type="unkown"
if [ "$(echo "$unamestr" | grep ubuntu)" != "" ]; then
  update-rc.d -f proxies defaults 99
  systemctl daemon-reload
  service proxies start
  type="ubuntu"
fi
echo "proxies service start success in $type!"
