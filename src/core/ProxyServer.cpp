//
// Created by codesgeeking on 2020/6/30.
//
#include "ProxyServer.h"

#include <boost/process.hpp>

#include "SessionManager.h"
#include "Utils.h"
#include <openssl/evp.h>

using namespace std;

string SHA224(const string &message) {
    uint8_t digest[EVP_MAX_MD_SIZE];
    char mdString[(EVP_MAX_MD_SIZE << 1) + 1];
    unsigned int digest_len;
    EVP_MD_CTX *ctx;
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
    ctx = EVP_MD_CTX_create();
#else
    ctx = EVP_MD_CTX_new();
#endif
    if (ctx != nullptr) {
        if (EVP_DigestInit_ex(ctx, EVP_sha224(), nullptr)) {
            if (EVP_DigestUpdate(ctx, message.c_str(), message.length())) {
                if (EVP_DigestFinal_ex(ctx, digest, &digest_len)) {
                    for (unsigned int i = 0; i < digest_len; ++i) {
                        sprintf(mdString + (i << 1), "%02x", (unsigned int) digest[i]);
                    }
                    mdString[digest_len << 1] = '\0';
#if !defined(OPENSSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L
                    EVP_MD_CTX_destroy(ctx);
#else
                    EVP_MD_CTX_free(ctx);
#endif
                    return string(mdString);
                } else {
                    throw runtime_error("could not output hash");
                }
            } else {
                throw runtime_error("could not update hash");
            }

        } else {
            throw runtime_error("could not initialize hash context");
        }
    } else {
        throw runtime_error("could not create hash context");
    }
}
bool resolveRemoteAddr(StreamTunnel *tunnel) {
    io_context ioContext;
    tcp::resolver slv(ioContext);
    auto remoteAddr = tunnel->remoteAddr;
    ip::tcp::resolver::query qry(remoteAddr, "");
    bool success = false;
    ip::tcp::resolver::iterator ipIt;
    int count = 0;
    while (!success && count < 3) {
        count++;
        boost::system::error_code error;
        ipIt = slv.resolve(qry, error);
        success = !error;
        if (!success) {
            Logger::ERROR << "resolve remoteAddr" << remoteAddr << "failed, try time" << count++
                          << error.message() << END;
            this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    if (!success) {
        Logger::ERROR << "resolve remoteAddr" << remoteAddr << "final failed!" << END;
        return false;
    }
    ip::tcp::resolver::iterator end;
    if (ipIt != end) {
        tunnel->remoteIP = (*ipIt).endpoint().address().to_v4().to_string();
        return true;
    }
    return false;
}
ProxyServer::ProxyServer(Config &config) : config(config) {
    try {

        for (auto it = config.tunnels.begin(); it != config.tunnels.end(); it++) {
            auto tunnel = *it;
            if (!resolveRemoteAddr(tunnel)) {
                exit(1);
            }
            tunnel->passwordSHA224 = SHA224(tunnel->password);
            ip::tcp::acceptor *serverAcceptor = new ip::tcp::acceptor(
                    ioContext, tcp::endpoint(boost::asio::ip::make_address_v4(tunnel->localAddr),
                                             tunnel->localPort));
            boost::asio::ip::tcp::acceptor::keep_alive option(true);
            serverAcceptor->set_option(option);
            Logger::INFO << tunnel->remoteAddr << tunnel->remoteIP << "listen at"
                         << tunnel->localPort << tunnel->passwordSHA224 << END;

            this->serverAcceptors.emplace(make_pair(tunnel->id(), serverAcceptor));
        }

    } catch (const boost::system::system_error &e) {
        Logger::ERROR << "bind address error" << e.what() << END;
        exit(1);
    }
}


void ProxyServer::start() {

    boost::asio::io_context::work ioContextWork(ioContext);
    vector<thread> threads;
    for (int i = 0; i < config.parallel; i++) {
        threads.emplace_back([&]() {
            for (auto it = config.tunnels.begin(); it != config.tunnels.end(); it++) {
                accept(*it);
            }
            ioContext.run();
        });
    }
    Logger::INFO << "proxies server started" << END;
    for (auto &th : threads) {
        th.join();
    }
    Logger::INFO << "proxies end" << END;
}

void ProxyServer::accept(StreamTunnel *tunnel) {
    tcp::socket rSocket(ioContext);
    serverAcceptors[tunnel->id()]->async_accept(
            [=](boost::system::error_code error, boost::asio::ip::tcp::socket socket) {
                // Check whether the server was stopped by a signal before this
                // completion handler had a chance to run.
                if (!serverAcceptors[tunnel->id()]->is_open()) {
                    return;
                }
                if (!error) {
                    SessionManager::INSTANCE->addNewSession(socket, config, tunnel);
                }
                accept(tunnel);
            });
}
