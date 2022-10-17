FROM codesgeeking/boost

USER root

ADD release/etc/proxies /etc/proxies

ADD release/bin/proxies /bin/proxies

RUN apt update 
RUN apt install -y net-tools
CMD [ "/bin/proxies"]

