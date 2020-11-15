set -e
scriptDir=$(
  cd $(dirname $0)
  pwd
)
cp -f ${scriptDir}/com.codesgeeking.proxies.plist /Library/LaunchDaemons/com.codesgeeking.proxies.plist
launchctl unload /Library/LaunchDaemons/com.codesgeeking.proxies.plist
rm -rf /tmp/proxies.log
rm -rf /tmp/proxies.error
launchctl load /Library/LaunchDaemons/com.codesgeeking.proxies.plist
echo "proxies service start success!"
