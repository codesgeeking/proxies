//
// Created by codesgeeking on 2020/10/5.
//

#include "SessionManager.h"
#include <mutex>

static mutex monitorLock;
static mutex statsLock;

Session *SessionManager::addNewSession(tcp::socket &socket, proxies::Config &config,
                                       StreamTunnel *tunnel) {
    Session *session = new Session(++id, socket, config, tunnel);
    auto theId = session->id;
    Logger::traceId = theId;
    {
        lock_guard<mutex> monitorLockGuard(monitorLock);
        connections.emplace(make_pair(theId, session));
    }
    session->start();
    return session;
}

SessionManager::SessionManager()
    : id(0), ioContextWork(new boost::asio::io_context::work(ioContext)),
      timerTh([&] { ioContext.run(); }), statTimer(ioContext), sessionTimer(ioContext) {
    scheduleStats();
    scheduleMonitor();
}

SessionManager *SessionManager::INSTANCE = new SessionManager();


void SessionManager::destroySession(uint64_t id) {
    monitorLock.lock();
    auto iterator = connections.find(id);
    if (iterator != connections.end()) {
        Session *session = iterator->second;
        if (session->stage == Session::STAGE::DETROYED) {
            delete session;
            connections.erase(iterator);
        }
    }
    monitorLock.unlock();
}

void SessionManager::stats() {
    if (id > 0) {
        Logger::INFO << "total sessions:" << connections.size() << END;
        lock_guard<mutex> statsLockGuard(statsLock);
        for (auto it = speeds.begin(); it != speeds.end(); it++) {
            Logger::traceId = 0;
            double speed = 0;
            if (it->second.first != 0) { speed = it->second.second * 1.0 / it->second.first; }
            Logger::INFO << "tunnel" << it->first << "speed" << speed << END;
        }
    }
}

void SessionManager::scheduleStats() {
    statTimer.expires_from_now(boost::posix_time::seconds(5));
    statTimer.async_wait([&](boost::system::error_code ec) {
        if (id > 0) { this->stats(); }
        this->scheduleStats();
    });
}

void SessionManager::scheduleMonitor() {
    sessionTimer.expires_from_now(boost::posix_time::seconds(1));
    sessionTimer.async_wait([&](boost::system::error_code ec) {
        if (id > 0) { this->monitorSession(); }
        this->scheduleMonitor();
    });
}

SessionManager::~SessionManager() {
    delete ioContextWork;
    statTimer.cancel();
    sessionTimer.cancel();
    timerTh.join();
}

void SessionManager::monitorSession() {
    auto now = proxies::utils::now();
    int soTimeout = proxies::Config::INSTANCE.soTimeout;
    int conTimeout = proxies::Config::INSTANCE.conTimeout;
    set<uint64_t> closedIds;
    {
        lock_guard<mutex> monitorLockGuard(monitorLock);
        lock_guard<mutex> statsLockGuard(statsLock);
        speeds.clear();
        for (auto it = connections.begin(); it != connections.end(); it++) {
            auto session = (*it).second;
            auto id = (*it).first;
            Logger::traceId = session->id;

            auto tunnelId = session->connectedTunnel->id();
            if (speeds.find(tunnelId) == speeds.end()) { speeds[tunnelId] = {0, 0}; }
            speeds[tunnelId].first += session->readTunnelTime;
            speeds[tunnelId].second += session->readTunnelSize + session->writeTunnelSize;
            bool noRead = session->lastReadTunnelTime == 0
                                  ? (now - session->begin >= soTimeout)
                                  : (now - session->lastReadTunnelTime >= soTimeout);
            bool noWrite = session->lastWriteTunnelTime == 0
                                   ? (now - session->begin >= soTimeout)
                                   : (now - session->lastWriteTunnelTime >= soTimeout);
            bool connectTimeout = session->stage == Session::STAGE::CONNECTING
                                          ? (now - session->begin >= conTimeout)
                                          : false;
            bool closed = session->stage == Session::STAGE::DETROYED;
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
}
