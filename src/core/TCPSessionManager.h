//
// Created by codesgeeking on 2020/10/5.
//

#ifndef TROJANX_TCPSESSIONMANAGER_H
#define TROJANX_TCPSESSIONMANAGER_H

#include "Common.h"
#include "Config.h"
#include "StreamTunnel.h"
#include "TCPSession.h"
#include <random>
#include <unordered_map>
#include <unordered_set>
using std::default_random_engine;

using namespace std;

class TCPSessionManager {
public:
    TCPSession *addNewSession(tcp::socket &socket, proxies::Config &config, StreamTunnel *tunnel);

    void stats();
    uint16_t guessUnusedSafePort();
    TCPSessionManager();
    static TCPSessionManager *INSTANCE;

    virtual ~TCPSessionManager();

private:
    void destroySession(uint64_t id);

    unordered_map<uint64_t, TCPSession *> connections;
    std::atomic_uint64_t id;
    default_random_engine randomEngine;
    uniform_int_distribution<unsigned short> randomRange;//随机数分布对象
    boost::asio::io_context ioContext;
    boost::asio::io_context::work *ioContextWork;

    boost::asio::deadline_timer timer;
    std::thread timerTh;

    void schedule();
    void scheduleCheckSession();
};


#endif//TROJANX_TCPSESSIONMANAGER_H
