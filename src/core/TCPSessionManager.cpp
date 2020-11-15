//
// Created by codesgeeking on 2020/10/5.
//

#include "TCPSessionManager.h"
#include <mutex>

static mutex wLock;

TCPSession *TCPSessionManager::addNewSession(tcp::socket &socket, proxies::Config &config,
                                             StreamTunnel *tunnel) {
    TCPSession *tcpSession = new TCPSession(++id, socket, config, tunnel);
    auto theId = tcpSession->id;
    Logger::traceId = theId;
    {
        lock_guard<mutex> lockGuard(wLock);
        connections.emplace(make_pair(theId, tcpSession));
    }
    tcpSession->start();
    return tcpSession;
}

TCPSessionManager::TCPSessionManager()
    : id(0), randomRange(1024, 12000), ioContextWork(new boost::asio::io_context::work(ioContext)),
      timerTh([&] { ioContext.run(); }), timer(ioContext) {
    //    srand(time::now());
    //    id = new std::atomic_uint64_t(rand() % 0xFFFFFFFFFFFF);
    randomEngine.seed(proxies::utils::now());
    schedule();
}

TCPSessionManager *TCPSessionManager::INSTANCE = new TCPSessionManager();


void TCPSessionManager::destroySession(uint64_t id) {
    wLock.lock();
    auto iterator = connections.find(id);
    if (iterator != connections.end()) {
        TCPSession *session = iterator->second;
        if (session->stage == TCPSession::STAGE::DETROYED) {
            delete session;
            connections.erase(iterator);
        }
    }
    wLock.unlock();
}

void TCPSessionManager::stats() { Logger::INFO << "total sessions:" << connections.size() << END; }

uint16_t TCPSessionManager::guessUnusedSafePort() { return randomRange(randomEngine); }

void TCPSessionManager::schedule() {
    timer.expires_from_now(boost::posix_time::seconds(5));
    timer.async_wait([&](boost::system::error_code ec) {
        this->stats();
        this->scheduleCheckSession();
        this->schedule();
    });
}

TCPSessionManager::~TCPSessionManager() {
    delete ioContextWork;
    timer.cancel();
    timerTh.join();
}

void TCPSessionManager::scheduleCheckSession() {
    auto now = proxies::utils::now();
    set<uint64_t> closedIds;
    unordered_map<string, pair<uint64_t, uint64_t>> speeds;
    {
        lock_guard<mutex> lockGuard(wLock);
        for (auto it = connections.begin(); it != connections.end(); it++) {
            auto session = (*it).second;
            auto id = (*it).first;
            Logger::traceId = session->id;

            if (session->connectedTunnel != nullptr) {
                auto tunnelId = session->connectedTunnel->id();
                if (speeds.find(tunnelId) == speeds.end()) { speeds[tunnelId] = {0, 0}; }
                speeds[tunnelId].first += session->readTunnelTime;
                speeds[tunnelId].second += session->readTunnelSize + session->writeTunnelSize;
            }
            Logger::INFO << session->toString() << session->transmit() << END;
            bool noRead = session->lastReadTunnelTime == 0
                                  ? (now - session->begin >= proxies::Config::INSTANCE.soTimeout)
                                  : (now - session->lastReadTunnelTime >=
                                     proxies::Config::INSTANCE.soTimeout);
            bool noWrite = session->lastWriteTunnelTime == 0
                                   ? (now - session->begin >= proxies::Config::INSTANCE.soTimeout)
                                   : (now - session->lastWriteTunnelTime >=
                                      proxies::Config::INSTANCE.soTimeout);
            bool connectTimeout =
                    session->stage == TCPSession::STAGE::CONNECTING
                            ? (now - session->begin >= proxies::Config::INSTANCE.soTimeout)
                            : false;
            bool closed = session->stage == TCPSession::STAGE::DETROYED;
            if (closed) {
                closedIds.emplace(id);
            } else if (connectTimeout) {
                session->shutdown();
                Logger::ERROR << "session manager shutdown connect timeout session" << id << END;
            } else if (noRead && noWrite) {
                session->shutdown();
                Logger::ERROR << "session manager shutdown no read no write session" << id << END;
            }
        }
    }
    for (auto it = closedIds.begin(); it != closedIds.end(); it++) { destroySession(*it); }

    for (auto it = speeds.begin(); it != speeds.end(); it++) {
        Logger::traceId = 0;
        double speed = 0;
        if (it->second.first != 0) { speed = it->second.second * 1.0 / it->second.first; }
        Logger::DEBUG << "tunnel" << it->first << "speed" << speed << END;
    }
}
