//
// Created by codesgeeking on 2020/9/17.
//

#ifdef linux

#include <sys/socket.h>

#endif


#include "Session.h"
#include <vector>
static const uint8_t PACKAT_01[3] = {0x05, 0x01, 0x00};
static const uint8_t PACKAT_02[3] = {0x05, 0x01, 0x00};
static const uint8_t SEND_PACKAT_01[2] = {0x05, 0x00};
static const uint8_t SEND_PACKAT_02[10] = {0x05, 0x00, 0x00, 0x01, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00};

string Session::parseDist(int addrTotalSize, bool domain) {
    string dist = "";
    for (int i = 0; i < addrTotalSize - 2; i++) {
        if (domain) {
            dist += ((char) readClientBuffer[i]);
        } else {
            dist += to_string(readClientBuffer[i]) + ".";
        }
    };
    dist += ":";
    unsigned short port = 0;
    port |= readClientBuffer[addrTotalSize - 1];
    port |= readClientBuffer[addrTotalSize - 2] << 8;
    dist += to_string(port);
    return dist;
}
Session::Session(uint64_t id, tcp::socket &sock, proxies::Config &config, StreamTunnel *tunnel)
    : id(id), begin(proxies::utils::now()), connectedTunnel(tunnel), clientSock(std::move(sock)),
      config(config), sslCtx(boost::asio::ssl::context::sslv23_client),
      proxySock((io_context &) clientSock.get_executor().context(), sslCtx),
      downStrand((io_context &) clientSock.get_executor().context()),
      upStrand((io_context &) clientSock.get_executor().context()) {}

void Session::start() {
    boost::system::error_code error;
    clientEnd = clientSock.remote_endpoint(error);
    copyBytes(connectedTunnel->passwordSHA224 + "\r\n", 0, writeProxyBuffer, 0, 58);
    readClient("sock5 client 01", 3, [=]() {
        if (!proxies::utils::equalBytes(readClientBuffer, PACKAT_01, 3)) {
            shutdown();
        } else {
            copyBytes(SEND_PACKAT_01, writeClientBuffer, 2);
            writeClient("sock5 server answer 01", 2, [=]() {
                readClient("sock5 client 02", 3, [=]() {
                    if (!proxies::utils::equalBytes(readClientBuffer, PACKAT_02, 3)) {
                        shutdown();
                    } else {
                        copyBytes(readClientBuffer, 1, writeProxyBuffer, 58, 1);
                        readClient("sock5 client addr type", 1, [=]() {
                            copyBytes(readClientBuffer, 0, writeProxyBuffer, 59, 1);
                            auto readAddrSzie = [=](int size, bool isDomain) {
                                readClient("sock5 client addr data", size, [=]() {
                                    int addrTotalSize = size;
                                    distEnd = parseDist(size, isDomain);
                                    copyBytes(readClientBuffer, 0, writeProxyBuffer,
                                              60 + (isDomain ? 1 : 0), addrTotalSize);
                                    addrTotalSize += (isDomain ? 1 : 0);
                                    copyBytes("\r\n", 0, writeProxyBuffer, 60 + addrTotalSize, 2);
                                    copyBytes(SEND_PACKAT_02, 0, writeClientBuffer, 0, 10);
                                    writeClient("sock5 server answer 02", 10, [=]() {
                                        this->readClientMax(
                                                "sock5 client first packet", 128,
                                                [=](int firstSize) {
                                                    copyBytes(readClientBuffer, 0, writeProxyBuffer,
                                                              62 + addrTotalSize, firstSize);
                                                    this->connectProxy([=]() {
                                                        this->handshakeProxy([=]() {
                                                            writeProxy("fisst auth trojan",
                                                                       62 + addrTotalSize +
                                                                               firstSize,
                                                                       [=]() {
                                                                           stageLock.lock();
                                                                           stage = CONNECTED;
                                                                           stageLock.unlock();

                                                                           readProxy();
                                                                           readClient();
                                                                       });
                                                        });
                                                    });
                                                });
                                    });
                                });
                            };
                            int lens = 2;
                            if (readClientBuffer[0] == 1) {
                                readAddrSzie(4 + lens, false);
                                distEnd += "v4:";
                            } else if (readClientBuffer[0] == 3) {
                                distEnd += "domain:";
                                readClient("sock5 client domain len", 1, [=]() {
                                    copyBytes(readClientBuffer, 0, writeProxyBuffer, 60, 1);
                                    readAddrSzie(readClientBuffer[0] + lens, true);
                                });
                            } else if (readClientBuffer[0] == 4) {
                                distEnd += "v6:";
                                readAddrSzie(16 + lens, false);
                            } else {
                                Logger::ERROR << "unkown sock5 client domain type" << END;
                                shutdown();
                            }
                        });
                    }
                });
            });
        }
    });
}


void Session::closeClient(std::function<void()> completeHandler) {
    boost::system::error_code ec;
    clientSock.shutdown(boost::asio::socket_base::shutdown_both, ec);
    clientSock.cancel(ec);
    clientSock.close(ec);
    completeHandler();
}
void Session::closeServer(std::function<void()> completeHandler) {
    boost::system::error_code ec;
    proxySock.next_layer().shutdown(boost::asio::socket_base::shutdown_both, ec);
    proxySock.async_shutdown([=](const boost::system::error_code &error) {
        boost::system::error_code ec;
        Logger::traceId = this->id;
        proxySock.next_layer().shutdown(boost::asio::socket_base::shutdown_both, ec);
        proxySock.next_layer().cancel(ec);
        proxySock.next_layer().close(ec);
        completeHandler();
    });
}

void Session::shutdown() {
    stageLock.lock();
    if (stage == CONNECTING || stage == CONNECTED) {
        long begin = proxies::utils::now();
        stage = DETROYING;
        stageLock.unlock();
        closeClient([=] {
            closeServer([=] {
                stageLock.lock();
                stage = DETROYED;
                stageLock.unlock();
            });
        });
    } else {
        stageLock.unlock();
    }
}

void Session::readClient() {
    readClientMax("readClient", bufferSize, [=](size_t size) {
        copyBytes(this->readClientBuffer, this->writeProxyBuffer, size);
        writeProxy(size);
    });
}
void Session::readClientMax(const string &tag, size_t maxSize,
                            std::function<void(size_t size)> completeHandler) {
    clientSock.async_read_some(buffer(readClientBuffer, sizeof(uint8_t) * maxSize),
                               upStrand.wrap([=](boost::system::error_code error, size_t size) {
                                   Logger::traceId = this->id;
                                   if (!error) {
                                       completeHandler(size);
                                   } else {
                                       processError(error, tag);
                                   }
                               }));
}
void Session::readClient(const string &tag, size_t size, std::function<void()> completeHandler) {
    clientSock.async_receive(buffer(readClientBuffer, sizeof(uint8_t) * size),
                             upStrand.wrap([=](boost::system::error_code error, size_t size) {
                                 Logger::traceId = this->id;
                                 if (!error) {
                                     completeHandler();
                                 } else {
                                     processError(error, tag);
                                 }
                             }));
}
void Session::readProxy() {
    long begin = proxies::utils::now();
    proxySock.async_read_some(buffer(readProxyBuffer, sizeof(uint8_t) * bufferSize),
                              downStrand.wrap([=](boost::system::error_code error, size_t size) {
                                  Logger::traceId = this->id;
                                  if (!error) {
                                      lastReadTunnelTime = proxies::utils::now();
                                      readTunnelTime += proxies::utils::now() - begin;
                                      readTunnelSize += size;
                                      proxies::utils::copyBytes(readProxyBuffer, writeClientBuffer,
                                                                size);
                                      writeClient(size);
                                  } else {
                                      processError(error, "readProxy");
                                  }
                              }));
}

void Session::processError(const boost::system::error_code &error, const string &TAG) {
    bool isEOF = error.category() == error::misc_category && error == error::misc_errors::eof;
    bool isCancled = error == error::operation_aborted;
    if (!isCancled && !isEOF) {
        Logger::ERROR << TAG << error.message() << END;
    }
    shutdown();
}

void Session::writeProxy(size_t writeSize) {
    writeProxy("writeProxy", writeSize, [=]() { readClient(); });
}

void Session::writeProxy(const string &tag, size_t writeSize,
                         std::function<void()> completeHandler) {
    long begin = proxies::utils::now();
    size_t len = sizeof(uint8_t) * writeSize;
    boost::asio::async_write(proxySock, buffer(writeProxyBuffer, len),
                             boost::asio::transfer_at_least(len),
                             upStrand.wrap([=](boost::system::error_code error, size_t size) {
                                 Logger::traceId = this->id;
                                 if (!error) {
                                     lastWriteTunnelTime = proxies::utils::now();
                                     writeTunnelTime += proxies::utils::now() - begin;
                                     writeTunnelSize += size;
                                     // Logger::TRACE << "writeProxy" << writeSize << size << END;
                                     completeHandler();
                                 } else {
                                     processError(error, tag);
                                 }
                             }));
}

void Session::connectProxy(std::function<void()> completeHandler) {
    tcp::endpoint serverEndpoint(make_address_v4(connectedTunnel->remoteIP),
                                 connectedTunnel->remotePort);
    proxySock.lowest_layer().async_connect(serverEndpoint, [=](boost::system::error_code error) {
        if (!error) {
            completeHandler();
        } else {
            processError(error, "connectProxy");
        }
    });
}
void Session::handshakeProxy(std::function<void()> completeHandler) {
    proxySock.async_handshake(boost::asio::ssl::stream_base::client,
                              [=](boost::system::error_code error) {
                                  if (!error) {
                                      completeHandler();
                                  } else {
                                      processError(error, "handshakeProxy");
                                  }
                              });
}

void Session::writeClient(size_t writeSize) {
    writeClient(__PRETTY_FUNCTION__, writeSize, [=]() { readProxy(); });
}

void Session::writeClient(const string &tag, size_t writeSize,
                          std::function<void()> completeHandler) {
    size_t len = sizeof(uint8_t) * writeSize;
    boost::asio::async_write(clientSock, buffer(writeClientBuffer, len),
                             boost::asio::transfer_at_least(len),
                             downStrand.wrap([=](boost::system::error_code error, size_t size) {
                                 Logger::traceId = this->id;
                                 if (error) {
                                     processError(error, tag);
                                 } else {
                                     // Logger::TRACE << "writeClient" << writeSize << size << END;
                                     completeHandler();
                                 }
                             }));
}

Session::~Session() {
    Logger::traceId = id;
    Logger::INFO << "disconnect" << toString() << transmit() << END;
}

string Session::toString() {
    return proxies::utils::asio::addrStr(clientEnd) + " -> " + distEnd + " " +
           connectedTunnel->remoteAddr;
}

string Session::transmit() const {
    const uint64_t val = proxies::utils::now() - this->begin;
    return to_string(id) + " live:" + to_string(live()) +
           ", read:" + to_string(this->readTunnelSize) +
           ", write:" + to_string(this->writeTunnelSize);
}
int Session::live() const { return proxies::utils::now() - this->begin; }
