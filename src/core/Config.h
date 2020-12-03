//
// Created by codesgeeking on 2020/6/30.
//

#ifndef PROXIES_CONFIG_H
#define PROXIES_CONFIG_H

#include "StreamTunnel.h"
#include "Utils.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
using namespace proxies::utils;
using namespace boost::property_tree;
using namespace boost::asio;

namespace proxies {
    class Config {
    public:
        static Config INSTANCE;
        int soTimeout = 60000;
        int conTimeout = 10000;
        int logLevel = 2;
        int parallel = 16;
        vector<StreamTunnel *> tunnels;
        string baseConfDir = "/etc/trojan";

        Config() = default;

        void load(const string &configPathInput) {
            baseConfDir = boost::filesystem::absolute(configPathInput).normalize().string();
            string configPath = baseConfDir + "/config.json";
            if (proxies::utils::file::exit(configPath)) {
                ptree tree;
                try {
                    read_json(configPath, tree);
                } catch (json_parser_error e) {
                    Logger::ERROR << " parse config file " + configPath + " error!" << e.message()
                                  << END;
                    exit(1);
                }
                this->logLevel = stoi(tree.get("log", to_string(this->logLevel)));
                Logger::LEVEL = this->logLevel;
                this->soTimeout = stoi(tree.get("so_timeout", to_string(this->soTimeout)));
                this->conTimeout = stoi(tree.get("connect_timeout", to_string(this->conTimeout)));
                this->parallel = stoi(tree.get("parallel", to_string(this->parallel)));

                auto tunnelNodes = tree.get_child("tunnels");
                if (!tunnelNodes.empty()) {
                    for (auto it = tunnelNodes.begin(); it != tunnelNodes.end(); it++) {
                        StreamTunnel *streamTunnel = parseStreamTunnel(it->second);
                        tunnels.emplace_back(streamTunnel);
                    }
                }
            } else {
                Logger::INFO << "proxies config file not exit!" << configPath << END;
                exit(1);
            }
        }

        template<class K, class D, class C>
        StreamTunnel *parseStreamTunnel(basic_ptree<K, D, C> &tunnel) const {
            StreamTunnel *streamTunnel = new StreamTunnel();
            streamTunnel->type = tunnel.get("type", "trojan");
            streamTunnel->localAddr = tunnel.get("local_addr", streamTunnel->localAddr);
            streamTunnel->localPort = tunnel.get("local_port", streamTunnel->localPort);
            streamTunnel->remoteAddr = tunnel.get("remote_addr", streamTunnel->remoteAddr);
            streamTunnel->remotePort = tunnel.get("remote_port", streamTunnel->remotePort);
            streamTunnel->password = tunnel.get("password", streamTunnel->password);
            return streamTunnel;
        }
    };

}// namespace proxies


#endif//PROXIES_CONFIG_H
