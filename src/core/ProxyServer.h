//
// Created by codesgeeking on 2020/6/30.
//

#ifndef PROXIES_PROXYSERVER_H
#define PROXIES_PROXYSERVER_H

#include "Config.h"
#include "Utils.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <map>
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace proxies;

class ProxyServer {
public:
    ProxyServer(Config &config);

    void start();

    static ProxyServer INSTANCE;

private:
    io_context ioContext;
    thread_pool pool;
    Config &config;
    std::map<string, ip::tcp::acceptor *> serverAcceptors;
    void accept(StreamTunnel *tunnel);
};


#endif//PROXIES_PROXYSERVER_H
