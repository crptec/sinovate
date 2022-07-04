// Copyright (c) 2018-2012 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFBFTPCLIENT_H
#define SIN_INFBFTPCLIENT_H

#include <cstring>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <key.h>
#include <boost/filesystem.hpp>

static const unsigned int BFTP_CLIENT_BUFFER_LENGTH = 4*1024;

using namespace std;

/**
 * @brief: open a TCP socket client with pre-defined privateKey.
 * Client upload only 1 big file size and close immediat the connection.
 * Nothing else.
 *
 * Using this client is:
 *    1. Send
 * 
 * CBftpClient Send(server_address, server_port, communication_private_key, file);
 */
class CBftpClient
{
public:
    bool stopBftpClient = false;
    /**
     * create TCP socket client with defined parameters
     */
    void Send(const std::string& address_server, uint16_t port, CKey privateKey, const std::string& filePath);
    /**
     * read filePath to buffer and send to server
     */
    int64_t Read_file(int sockfd, boost::filesystem::path path);
};

#endif // SIN_INFBFTPCLIENT_H