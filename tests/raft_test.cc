#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <filesystem>
#include <hiredis/hiredis.h>
#include <unistd.h>

void setupDirectoryAndCopyConfig(const std::string& directory, 
        const std::string& configFile, int port) {
    std::filesystem::create_directory(directory);
    std::string newConfigPath = directory + "/pikiwidb.conf";
    std::ifstream src(configFile, std::ios::binary);
    std::ofstream dst(newConfigPath, std::ios::binary);

    std::string line;
    while (std::getline(src, line)) {
        // 查找并替换端口号
        if (line.find("port") != std::string::npos && line.substr(0, 4) == "port") {
            line = "port " + std::to_string(port);
        }
        dst << line << "\n";
    }

    src.close();
    dst.close();
}

redisContext* redisConnectPikiwidb(int port) {
    redisContext* client = redisConnect("127.0.0.1", port);
    if (client == nullptr || client->err) {
        if (client) {
            std::cerr << "Error: " << client->errstr << std::endl;
        } else {
            std::cerr << "Can't allocate redis context" << std::endl;
        }
        exit(1);
    }

    return client;
}

int main() {
    std::string originalConfigFile = "../pikiwidb.conf";

    setupDirectoryAndCopyConfig("../node_1", originalConfigFile, 9221);
    setupDirectoryAndCopyConfig("../node_2", originalConfigFile, 9222);
    setupDirectoryAndCopyConfig("../node_3", originalConfigFile, 9223);

    system("../bin/pikiwidb ../node_1/pikiwidb.conf > ../node_1/output.txt &");
    system("../bin/pikiwidb ../node_2/pikiwidb.conf > ../node_2/output.txt &");
    system("../bin/pikiwidb ../node_3/pikiwidb.conf > ../node_3/output.txt &");

    std::cout << "Waiting for pikiwidb servers to start...\n";
    sleep(2);

    redisContext* client_1 = redisConnectPikiwidb(9221);
    redisContext* client_2 = redisConnectPikiwidb(9222);
    redisContext* client_3 = redisConnectPikiwidb(9223);    

    std::cout << "Initializing pikiwidb cluster..." << std::endl;
    redisReply* reply = nullptr;
    reply = (redisReply*)redisCommand(client_1, "raft.cluster init");
    if (reply != nullptr) {
        std::cout << "[pikiwidb-9221] raft cluster init: " << reply->str << std::endl;
        freeReplyObject(reply);
    } else {
        std::cerr << "[pikiwidb-9221] raft cluster init: failed" << std::endl;
    }

    reply = (redisReply*)redisCommand(client_2, "raft.cluster join 127.0.0.1:9221");
    if (reply != nullptr) {
        std::cout << "[pikiwidb-9222] raft.cluster join 127.0.0.1:9221: " << reply->str << std::endl;
        freeReplyObject(reply);
    } else {
        std::cerr << "[pikiwidb-9222] raft.cluster join 127.0.0.1:9221: failed" << std::endl;
    }

    reply = (redisReply*)redisCommand(client_3, "raft.cluster join 127.0.0.1:9221");
    if (reply != nullptr) {
        std::cout << "[pikiwidb-9223] raft.cluster join 127.0.0.1:9221: " << reply->str << std::endl;
        freeReplyObject(reply);
    } else {
        std::cerr << "[pikiwidb-9223] raft.cluster join 127.0.0.1:9221: failed" << std::endl;
    }

    std::cout << "Remove pikiwidb node 9222..." << std::endl;
    reply = (redisReply*)redisCommand(client_2, "info raft");
    std::string peer_id = "";
    if (reply != nullptr) {
        std::cout << "[pikiwidb-9222] info raft: " << reply->str << std::endl;
        std::string prefix = "peer_id:";
        std::string reply_str(reply->str);
        std::string::size_type prefix_length = prefix.length();
        std::string::size_type peer_id_start = reply_str.find(prefix);
        peer_id_start += prefix_length;  // 定位到raft_group_id的起始位置
        std::string::size_type peer_id_end = reply_str.find("\r\n", peer_id_start);
        if (peer_id_end != std::string::npos) {
            peer_id = reply_str.substr(peer_id_start, peer_id_end - peer_id_start);
        } else {
            std::cerr << "[pikiwidb-9222] get peer id failed" << std::endl;
        }
        freeReplyObject(reply);
    } else {
        std::cerr << "[pikiwidb-9222] info raft: failed" << std::endl;
    }

    int count = 0;
    while (true) {
        reply = (redisReply*)redisCommand(client_2, std::string("raft.node remove " + peer_id).c_str());
        if (reply != nullptr) {
            std::cout << "[pikiwidb-9223] raft.node remove " + peer_id + ": " << reply->str << std::endl;
            std::string reply_str(reply->str);
            if (reply_str.find("OK") != std::string::npos) {
                break;
            } else if (reply_str.find("The leader address of the") != std::string::npos || 
                reply_str.find("ERR failed to connect to cluster for join or remove") != std::string::npos) {
                count++;
            } 
            
            if (count == 3) {
                std::cerr << "[pikiwidb-9222] raft.node remove " + peer_id + " failed after three retries" << std::endl;
                break;
            }
            freeReplyObject(reply);
        } else {
            std::cerr << "[pikiwidb-9222] raft.node remove " + peer_id + " failed" << std::endl;
        }
    }    

    system("pgrep -f pikiwidb | xargs kill");
    
    return 0;
}
