//
// Created by codesgeeking on 2020/9/17.
//

#ifndef PROXIES_SESSION_H
#define PROXIES_SESSION_H

#include "Common.h"

class Session {
public:
    enum STAGE { CONNECTING, CONNECTED, DETROYING, DETROYED };
    static const uint32_t bufferSize = 1024;

    Session(uint64_t id, tcp::socket &sock, proxies::Config &config, StreamTunnel *tunnel);

    virtual ~Session();

    uint64_t id;
    uint16_t port = 0;
    uint64_t begin = 0L;
    uint64_t lastReadTunnelTime = 0;
    uint64_t readTunnelTime = 0;
    uint64_t readTunnelSize = 0;
    uint64_t lastWriteTunnelTime = 0;
    uint64_t writeTunnelTime = 0;
    uint64_t writeTunnelSize = 0;
    StreamTunnel *connectedTunnel = nullptr;
    STAGE stage = CONNECTING;
    string distEnd;

    void start();
    void shutdown();

    string toString();
    string transmit() const;
    int live() const;

private:
    mutex stageLock;
    tcp::socket clientSock;
    proxies::Config &config;
    boost::asio::ssl::context sslCtx;
    boost::asio::ssl::stream<tcp::socket> proxySock;
    tcp::endpoint clientEnd;
    uint8_t readClientBuffer[bufferSize];
    uint8_t writeProxyBuffer[bufferSize];
    uint8_t writeClientBuffer[bufferSize];
    uint8_t readProxyBuffer[bufferSize];
    io_context::strand downStrand;
    io_context::strand upStrand;
    void readClient();
    void readClientMax(const string &tag, size_t maxSize,
                       std::function<void(size_t size)> completeHandler);
    void readClient(const string &tag, size_t size, std::function<void()> completeHandler);

    void writeClient(size_t size);
    void writeClient(const string &tag, size_t size, std::function<void()> completeHandler);

    void readProxy();
    void writeProxy(size_t size);
    void writeProxy(const string &tag, size_t size, std::function<void()> completeHandler);

    void connectProxy(std::function<void()> completeHandler);
    void handshakeProxy(std::function<void()> completeHandler);

    void closeClient(std::function<void()> completeHandler);
    void closeServer(std::function<void()> completeHandler);

    string parseDist(int addrTotalSize, bool domain);


    void bindLocalPort(basic_endpoint<tcp> &endpoint, boost::system::error_code &error);

    void processError(const boost::system::error_code &error, const string &TAG);

    void copyOption();

    bool initProxySocks();

#ifdef linux

    void setMark();

#endif
};

#endif// PROXIES_SESSION_H
