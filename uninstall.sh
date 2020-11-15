if [ -f "/usr/bin/proxies" ]; then
  sudo /usr/bin/proxies -d stop
fi
if [ -f "/usr/local/bin/proxies" ]; then
  sudo /usr/local/bin/proxies -d stop
fi
rm -rf /usr/local/bin/proxies
rm -rf /usr/bin/proxies
rm -rf /usr/local/etc/st/dns
rm -rf /etc/st/dns
