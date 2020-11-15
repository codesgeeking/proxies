//
// Created by codesgeeking on 2020/10/8.
//

#include "StreamTunnel.h"

string StreamTunnel::id() { return type + "_" + this->remoteAddr + "_" + to_string(this->remotePort); }
