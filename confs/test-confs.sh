if [ "$1" != "" ]; then
  rm -rf service
  rm -rf config.json
  ln -s $1/service service
  ln -s $1/config.json config.json
fi
