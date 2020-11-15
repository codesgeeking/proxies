//
// Created by codesgeeking on 2020/10/5.
//

#ifndef PROXIES_SESSIONMANAGER_H
#define PROXIES_SESSIONMANAGER_H

#include "Common.h"
#include "Config.h"
#include "StreamTunnel.h"
#include "Session.h"
#include <random>
#include <unordered_map>
#include <unordered_set>
using std::default_random_engine;

using namespace std;

class SessionManager {
public:
    Session *addNewSession(tcp::socket &socket, proxies::Config &config, StreamTunnel *tunnel);

    void stats();
    SessionManager();
    static SessionManager *INSTANCE;
    virtual ~SessionManager();

private:
    unordered_map<string, pair<uint64_t, uint64_t>> speeds;
    void destroySession(uint64_t id);
    unordered_map<uint64_t, Session *> connections;
    std::atomic_uint64_t id;
    boost::asio::io_context ioContext;
    boost::asio::io_context::work *ioContextWork;
    std::thread timerTh;
    boost::asio::deadline_timer statTimer;
    boost::asio::deadline_timer sessionTimer;
    void scheduleStats();
    void scheduleMonitor();
    void monitorSession();
};


#endif//PROXIES_SESSIONMANAGER_H
