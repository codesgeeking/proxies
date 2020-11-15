set -e
scriptDir=$(cd $(dirname $0); pwd)
if [ -f "/Library/LaunchDaemons/com.codesgeeking.proxies.plist" ];then
  launchctl unload /Library/LaunchDaemons/com.codesgeeking.proxies.plist
  rm -rf  /Library/LaunchDaemons/com.codesgeeking.proxies.plist
fi
if [ -f "${scriptDir}/com.codesgeeking.proxies.plist" ];then
  launchctl unload ${scriptDir}/com.codesgeeking.proxies.plist
fi
sh ${scriptDir}/../nat/rule.sh clean
echo "proxies service stop success!"
