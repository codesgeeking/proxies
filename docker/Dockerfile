FROM codesgeeking/boost

USER root

ADD ld.so.conf /etc/ld.so.conf

ADD release/etc/proxies /etc/proxies

ADD release/bin/proxies /bin/proxies

CMD [ "/bin/proxies"]

