//
// Created by codesgeeking on 2020/10/8.
//

#ifndef PROXIES_STSTREAMTUNNEL_H
#define PROXIES_STSTREAMTUNNEL_H

#include "Utils.h"
#include <set>

class StreamTunnel {
public:
    string type = "trojan";
    string localAddr = "";
    string remoteAddr = "";
    int localPort = 0;
    int remotePort = 0;
    string password = "";
    string remoteIP = "";
    string passwordSHA224 = "";

    string id();
};


#endif//PROXIES_STSTREAMTUNNEL_H
