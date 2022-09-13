// Copyright (c) 2018-2012 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/infbftpserver.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <logging.h>
#include <threadinterrupt.h>
#include <util/system.h>
#include <util/thread.h>

/**
 * @brief: save all data receive from socket to a path on server.
 * if client send more then required. data file will be deleted.
 *
 * @param sockfd:         socket description for communication
 * @param path:           full path for data file on server
 * @param nClientMsgsize: exact file size will be checked.
 *
 *return:
 *  -1:     if can not open file to write (system error)
 *  -2:     client's authentification is incorrect
 *  size_t: bytes are saved in data file on server
 */
size_t CBftpServer::Write_file(int sockfd, boost::filesystem::path path, int nClientMsgsize)
{
    int n;
    FILE *fp;
    char buffer[BFTP_SERVER_BUFFER_LENGTH];
    size_t recvBytes = 0;
    std::atomic<int64_t> nLastRecv{0};
    nLastRecv = GetTimeSeconds();
    static const int BFTP_SERVER_TIMEOUT_INTERVAL = 1 * 60;

    fp = fopen(path.string().c_str(), "wb");
    if (fp == nullptr)
    {
        LogPrint(BCLog::BFTP, "Failed to open file %s", path.string().c_str());
        close(sockfd);
        return -1;
    }

    while (recvBytes < nClientMsgsize)
    {
        int64_t now = GetTimeSeconds();
        if (now > nLastRecv + BFTP_SERVER_TIMEOUT_INTERVAL)
        {
            LogPrint(BCLog::BFTP, "socket sending timeout: %is\n", now - nLastRecv);
            break;
        }

        n = recv(sockfd, buffer, BFTP_SERVER_BUFFER_LENGTH, 0);
        if (n < 0)
        {
            LogPrint(BCLog::BFTP, "socket error. Client closed connection.\n");
            break;
        }

        if (n == 0 && recvBytes < nClientMsgsize)
        {
            LogPrint(BCLog::BFTP, "Client closed the connection before all of the data was sent.\n");
            break;
        }

        if (fwrite(buffer, 1, n, fp) != n)
        {
            LogPrint(BCLog::BFTP, "fwrite() failed. Data is not write correctly in server file.\n");
            break;
        } else {
            recvBytes += n;
        }
        bzero(buffer, BFTP_SERVER_BUFFER_LENGTH);
    }
    close(sockfd);
    fclose(fp);

    if (recvBytes != nClientMsgsize)
    {
        LogPrint(BCLog::BFTP, "Client sent different bytes than required. Delete data file.\n");
        boost::filesystem::remove(path);
    }

    return recvBytes;
}

/**
 * @brief: open a FTP server in a new thread, and wait for client connection.
 * server will write the communication data if client's authentification is correct.
 * authentification is a signed message (nonce) with communication privKey
 *
 * @param address: wait for communication at address
 * @param port: wait for communication at port
 * @param nonce, privKey: get from bftp protocol communication. Use to check the client's authentification
 * @param nClientMsgsize: get from bftp protocol communication. Exact size will be sent from client.
 * @param filePath: Data file will be stored on server
 */
void CBftpServer::Create(std::string address, uint16_t port, CKey privKey, int nClientMsgsize, std::string filePath)
{
    int listen_sock_fd, conn_sock_fd = -1;
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char str_add[INET6_ADDRSTRLEN];
    int ret, flag = 1;

    do {
        /**
         * create a socket description to prepare to accept incoming connection
         */
        if ((listen_sock_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        {
            LogPrint(BCLog::BFTP, "socket() failed\n");
            break;
        }

        /**
         * allow the local address to be reused when the server os restarted before the required with time expires.
         */
        if (setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0)
        {
            LogPrint(BCLog::BFTP, "setsockopt(SO_REUSEADDR) failed\n");
            break;
        }

        /**
         * set socket param for connection and gets unique name for the socket.
         * in6addr_any allows connections to be establised from any IPv6 or IPv6 client.
         */
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_port = htons(port);
        server_addr.sin6_addr = in6addr_any;

        if (bind(listen_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            LogPrint(BCLog::BFTP, "bind() failed\n");
            break;
        }

        /**
         * set server accept only 1 incoming connection request.
         * server will reject if it detects many incomings connections.
         */
        if (listen(listen_sock_fd, 1) < 0)
        {
            LogPrint(BCLog::BFTP, "listen() failed\n");
            break;
        }

        /**
         * server will block indefinitely waiting for an unique connection
         */
        if((conn_sock_fd = accept(listen_sock_fd, NULL, NULL)) < 0)
        {
            LogPrint(BCLog::BFTP, "accept() failed\n");
            break;
        }
        else
        {
            getpeername(conn_sock_fd, (struct sockaddr *)&client_addr, &addrlen);
            if (inet_ntop(AF_INET6, &client_addr.sin6_addr, str_add, sizeof(str_add)))
            {
                LogPrint(BCLog::BFTP, "bftpserver: Client address is: %s.\n",str_add);
                LogPrint(BCLog::BFTP, "bftpserver: Client port is: %d.\n",ntohs(client_addr.sin6_port));
            }
        }

        /**
         * write received data to file
         */
        size_t received = Write_file(conn_sock_fd, boost::filesystem::path(filePath), nClientMsgsize);
        if (nClientMsgsize == received)
        {
            LogPrint(BCLog::BFTP, "bftpserver received: %d bytes from client and saved.\n", received);
        } else {
            LogPrint(BCLog::BFTP, "bftpserver received: %d bytes from client, different with communicated infos. Data file is deleted.\n", received);
        }
    } while (false);

    if (listen_sock_fd != -1) close(listen_sock_fd);
    if (conn_sock_fd != -1) close(conn_sock_fd);
    LogPrint(BCLog::BFTP, "bftpserver closed communication.\n");
}

/**
 * @brief: called from net_processing. when client want to send a big file and the payement is correct.
 * the process will open a communication chanel, which allow the client to updaload the data file > 4Mb.
 * the thread (server) will close:
 *   - when it receive more then nClientMsgsize bytes from client
 *   - technical error
 *   - client's authentification is failed.
 */
void CBftpServer::Start()
{
    bftpServer = std::thread([this] { Create(address, port, privKey, nClientMsgsize, filePath); });
    bftpServer.detach();
}
