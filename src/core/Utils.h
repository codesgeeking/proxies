//
// Created by codesgeeking on 2020/5/19.
//

#ifndef PROXIES_UTILS
#define PROXIES_UTILS

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <set>
#include <string>

using namespace boost::asio::ip;
using namespace std;
using namespace boost::asio;
#define END Logger::MASK::ENDL;

static const char *const SPLIT = " ";
using namespace std;
namespace proxies {
    namespace utils {
        static void copyBytes(const uint8_t *from, uint8_t *to, int size) {
            for (int i = 0; i < size; i++) { *(to + i) = *(from + i); }
        }
        static void copyBytes(const uint8_t *from, int fromPos, uint8_t *to, int toPos, int size) {
            for (int i = 0; i < size; i++) { *(to + i + toPos) = from[i + fromPos]; }
        }
        static void copyBytes(const string str, int fromPos, uint8_t *to, int toPos, int size) {
            const char *from = str.c_str();
            for (int i = 0; i < size; i++) { *(to + i + toPos) = from[i + fromPos]; }
        }
        static bool equalBytes(const uint8_t *from, const uint8_t *to, int size) {
            for (int i = 0; i < size; i++) {
                if (*(to + i) != *(from + i)) { return false; };
            }
            return true;
        }
        namespace asio {
            static string addrStr(tcp::endpoint &endpoint) {
                return move(endpoint.address().to_string() + ":" + to_string(endpoint.port()));
            }
        }// namespace asio
        static inline long now() {
            auto time_now = std::chrono::system_clock::now();
            auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    time_now.time_since_epoch());
            return duration_in_ms.count();
        }
        namespace file {
            static inline bool exit(const string &path) {
                bool exits = false;
                fstream fileStream;
                fileStream.open(path, ios::in);
                if (fileStream) { exits = true; }
                fileStream.close();
                return exits;
            }
        }// namespace file

        static inline bool pid(const string &pidFile) {
            int pid = getpid();
            ofstream fileStream(pidFile);
            if (fileStream.is_open()) {
                fileStream << pid;
                fileStream.flush();
                fileStream.close();
                return true;
            }
            return false;
        }
        static bool exec(const string &command, string &resultStr, string &errorStr) {
            std::error_code ec;
            boost::process::ipstream is;
            boost::process::ipstream error;
            int result = boost::process::system(command, boost::process::std_out > is,
                                                boost::process::std_err > error, ec);
            bool success = false;
            if (!ec && result == 0) {
                string log;
                while (getline(is, log)) {
                    if (!resultStr.empty()) { resultStr += "\n"; }

                    resultStr += log;
                }
                success = true;
            }
            string log;
            while (getline(error, log)) {
                if (!errorStr.empty()) { errorStr += "\n"; }
                errorStr += log;
            }
            return success;
        }
        static bool exec(const string &command) {
            std::error_code ec;
            int result = boost::process::system(command, ec);
            bool success = false;
            if (!ec && result == 0) { success = true; }
            return success;
        }
        class Logger {
        private:
            uint32_t level = 0;
            string tag;
            string str;

            void appendStr(const string &info);

            static void getTime(string &timeStr);

        public:
            static thread_local uint64_t traceId;
            enum MASK { ENDL };
            static thread_local Logger TRACE;
            static thread_local Logger DEBUG;
            static thread_local Logger INFO;
            static thread_local Logger ERROR;
            static uint32_t LEVEL;

            explicit Logger(string tag, uint32_t level);

            void doLog();

            Logger &operator<<(const char *log);

            Logger &operator<<(char ch);

            Logger &operator<<(const string &string);

            Logger &operator<<(char *log);

            Logger &operator<<(const set<string> &strs);

            template<typename A>
            Logger &operator<<(const A &str1) {
                if (typeid(str1) == typeid(MASK) && str1 == ENDL) {
                    doLog();
                } else {
                    this->appendStr(to_string(str1));
                }
                return *this;
            }

            ostream *getStream();

            void doLog(const string &time, ostream &st, const string &line);
        };
    } // namespace utils
};    // namespace proxies
#endif// PROXIES_UTILS
