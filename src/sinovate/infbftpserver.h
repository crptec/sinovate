// Copyright (c) 2018-2012 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFBFTPSERVER_H
#define SIN_INFBFTPSERVER_H

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

static const unsigned int BFTP_SERVER_BUFFER_LENGTH = 4*1024;

using namespace std;

/**
 * @brief: open a TCP socket server for identified client with pre-defined privateKey.
 * Allow client upload only 1 big file size and close immediat the connection.
 * Server do nothing else. it is likely one direction communication chanel.
 *
 * Using this server is:
 *    1. Create in instance
 *    2. Start the server
 * 
 * CBftpServer server(address, port, privateKey, filesize);
 * server.start();
 */
class CBftpServer
{
private:
    const std::string address;
    const uint16_t port;
    CKey privKey;
    const int nClientMsgsize;
    std::string filePath;

    std::thread bftpServer;
public:
    bool stopBftpServer = false;
    /**
     * @param address: The host to accept incoming connections from.
     * @param port: The port to start the server on. Must > 1024.
     * @param privateKey: Client privateKey. The server will accept only 1 connection from client which has this key.
     * @param nClientMsgsize: The server will allow to upload only 1 file with exact file's size
     */
    CBftpServer(const std::string& _address, uint16_t _port, CKey _privateKey, int _nClientMsgsize, std::string  _datafile):
        address(_address),
        port(_port),
        privKey(_privateKey),
        nClientMsgsize(_nClientMsgsize),
        filePath(_datafile)
    {}
    /**
     * create TCP socket server with defined parameters
     */
    void Create(std::string address, uint16_t port, CKey privKey, int nClientMsgsize, std::string filePath);
    /**
     * @brief: Start the server
     *
     * @return True if the server has been started successfully
     */
    void Start();
    /**
     * write reveiced buffer to file filePath
     */
    size_t Write_file(int sockfd, boost::filesystem::path path, int nClientMsgsize);
};

#endif // SIN_INFBFTPFILEUTIL_H