//
// Created by codesgeeking on 2020/5/19.
//

#include "Utils.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>
#include <utility>

using namespace std;
static std::mutex logMutex;

void proxies::utils::Logger::getTime(std::string &timeStr) {
    char buff[20];
    time_t now = time(nullptr);
    tm *time = localtime(&now);
    strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", time);
    timeStr = buff;
}


void proxies::utils::Logger::doLog() {
    std::lock_guard<std::mutex> lg(logMutex);
    string time;
    getTime(time);
    int pos = 0;
    int lastPos = 0;
    ostream &st = *(getStream());

    while ((pos = this->str.find("\n", pos)) != this->str.npos) {
        auto line = this->str.substr(lastPos, (pos - lastPos));
        doLog(time, st, line);
        pos += 1;
        lastPos = pos;
    }

    auto line = this->str.substr(lastPos, (this->str.length() - lastPos));
    doLog(time, st, line);
    this->str.clear();
}

void proxies::utils::Logger::doLog(const string &time, ostream &st, const string &line) {
    if (this->level >= LEVEL) {
        st << "[" << this_thread::get_id();
        if (traceId != 0) {
            st << "-" << traceId;
        }
        st << "]" << SPLIT << "[" << tag << "]" << SPLIT << time << SPLIT;
        st << line << endl;
    }
}

ostream *proxies::utils::Logger::getStream() {
    ostream *stream = &cout;
    if (tag == "ERROR") {
        stream = &cerr;
    }
    return stream;
}

thread_local proxies::utils::Logger proxies::utils::Logger::TRACE("TRACE", 0);
thread_local proxies::utils::Logger proxies::utils::Logger::DEBUG("DEBUG", 1);
thread_local proxies::utils::Logger proxies::utils::Logger::INFO("INFO", 2);
thread_local proxies::utils::Logger proxies::utils::Logger::ERROR("ERROR", 3);

uint32_t proxies::utils::Logger::LEVEL = 2;

proxies::utils::Logger &proxies::utils::Logger::operator<<(const char *log) {
    appendStr(log);
    return *this;
}

proxies::utils::Logger &proxies::utils::Logger::operator<<(char *log) {
    appendStr(log);
    return *this;
}

proxies::utils::Logger &proxies::utils::Logger::operator<<(char ch) {
    this->str.push_back(ch);
    this->str.append(SPLIT);
    return *this;
}

proxies::utils::Logger::Logger(string tag, uint32_t level) : tag(tag), level(level) {
}

proxies::utils::Logger &proxies::utils::Logger::operator<<(const string &string) {
    appendStr(string);
    return *this;
}


void proxies::utils::Logger::appendStr(const string &info) {
    this->str.append(info).append(SPLIT);
}

proxies::utils::Logger &proxies::utils::Logger::operator<<(const set<string> &strs) {
    for (auto str : strs) {
        appendStr(str);
    }
    return *this;
}


thread_local uint64_t proxies::utils::Logger::traceId = 0;
