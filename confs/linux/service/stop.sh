set -e
scriptDir=$(
  cd $(dirname $0)
  pwd
)
cp -f ${scriptDir}/proxies /etc/init.d/
chmod +x /etc/init.d/proxies
unamestr=$(uname -a | tr 'A-Z' 'a-z')
type=""
if [ "$(echo "$unamestr" | grep ubuntu)" != "" ]; then
  systemctl daemon-reload
  service proxies stop
  update-rc.d -f proxies remove
  systemctl daemon-reload
  type="ubuntu"
fi
sh ${scriptDir}/../nat/rule.sh clean
echo "proxies service stop success in $type!"