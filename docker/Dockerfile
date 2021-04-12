FROM codesgeeking/boost

USER root

ADD release/etc/proxies /etc/proxies

ADD release/bin/proxies /bin/proxies

RUN echo "/usr/local/lib" >> /etc/ld.so.conf

CMD [ "/bin/proxies"]

