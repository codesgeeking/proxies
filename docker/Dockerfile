FROM codesgeeking/boost

USER root

RUN echo "/usr/local/lib" >> /etc/ld.so.conf && ldconfig

ADD release/etc/proxies /etc/proxies

ADD release/bin/proxies /bin/proxies

CMD [ "/bin/proxies"]

