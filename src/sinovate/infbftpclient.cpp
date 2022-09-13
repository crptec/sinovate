// Copyright (c) 2018-2012 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/infbftpclient.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <logging.h>
#include <threadinterrupt.h>
#include <util/system.h>
#include <util/thread.h>
#include <algorithm>


static const unsigned int NETDB_MAX_HOST_NAME_LENGTH = 128;

/**
 * @brief: send data in buffer until nBufferSize value is met.
 * each time, the process send the min value between BFTP_CLIENT_BUFFER_LENGTH and the rest of buffer
 *
 * @param sockfd: socket description
 * @param buffer: pointer to data
 * @param nBufferSize: size of data
 *
 * @return:
 *  size_t : bytes were sent by socket.
 *  -1: socket error
 */
size_t SendBuffer(int sockfd, const char* buffer, uint64_t nBufferSize)
{
    uint64_t nSent = 0;
    size_t nSendOffset = 0;
    uint64_t nSendBytes = 0;
    while (nSent < nBufferSize)
    {
        int nCount = 0;
        nCount = send(sockfd, &buffer[nSendOffset], nBufferSize - nSendOffset, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (nCount > 0)
        {
            nSendBytes += nCount;
            nSendOffset += nCount;
            nSent += nCount;
            if (nSendOffset == nBufferSize)
            {
                nSendOffset = 0;
            }
        } else {
            if (nCount < 0)
            {
                close(sockfd);
                return -1;
            }
            // couldn't send anything more
            break;
        }
    }
    return nSent;
}

/**
 * @brief: the process read (max) BFTP_CLIENT_BUFFER_LENGTH bytes from file.
 * and try to send it to server.
 *
 * @param sockfd: socket description for communication.
 * @param path: full path of file which will be sent to server.
 *
 * @return
 *  -1: can not open file
 *  n : >= 0. Data size need to be sent. 0 if file was sent correctly.
 */
int64_t CBftpClient::Read_file(int sockfd, boost::filesystem::path path)
{
    int n;
    FILE *fp;
    char buffer[BFTP_CLIENT_BUFFER_LENGTH];

    fp = fopen(path.string().c_str(), "rb");
    if (fp == nullptr)
    {
        LogPrint(BCLog::BFTP, "%s: Failed to open file %s\n", __func__, path.string());
        return -1;
    }

    int64_t fileSize = boost::filesystem::file_size(path);
    size_t nRead = 0;
    LogPrint(BCLog::BFTP, "bftpclient: sending file size %d bytes to server.\n", fileSize);

    int64_t nBytesToSend = fileSize;
    bool errored = false;
    int64_t buffer_limit = BFTP_CLIENT_BUFFER_LENGTH;

    do {
        /*read nBytesRead from file*/
        int64_t nBytesRead = std::min(buffer_limit, nBytesToSend);
        /*read failed, return immediat, result = false */
        size_t nread = fread(buffer, 1, nBytesRead, fp);
        if ( nread != nBytesRead){
            LogPrint(BCLog::BFTP, "bftpclient: fread() failed, read: %d, require :%d\n", nread, nBytesRead);
            errored = true;
            break;
        }

        /*send buffer to server*/
        int sent = SendBuffer(sockfd, buffer, nread);
        if (sent < 0)
        {
            LogPrint(BCLog::BFTP, "bftpclient: send() failed\n");
            errored = true;
            break;
        } else {
            if (sent != nread)
            {
                LogPrint(BCLog::BFTP, "bftpclient: could not send all buffer read from file, read: %s, sent: %d\n", nread, sent);
                errored = true;
                break;
            }
            /*recalculated nBytesToSend need to send in next time*/
            nBytesToSend -= sent;
        }
    }
    while (nBytesToSend > 0);
    fclose(fp);
    //close socket if no error detected before.
    if (!errored)
    {
        close(sockfd);
    }

    return errored ? -1 : nBytesToSend;
}

void CBftpClient::Send(const std::string& address_server, uint16_t port, CKey privateKey, const std::string& filePath)
{
    int conn_sock_fd = -1, rc;
    struct in6_addr server_addr;
    struct addrinfo hints, *res = NULL;
    char server[NETDB_MAX_HOST_NAME_LENGTH];

    do {
        strcpy(server, address_server.c_str());

        memset(&hints, 0x00, sizeof(hints));
        hints.ai_flags    = AI_NUMERICSERV;
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        /*check provided address server*/
        rc = inet_pton(AF_INET, server, &server_addr);
        if (rc == 1) /* valid IPv4*/
        {
            hints.ai_family   = AF_INET;
            hints.ai_flags   |= AI_NUMERICHOST;
        }
        else
        {
            rc = inet_pton(AF_INET6, server, &server_addr);
            if (rc == 1) /* valid IPv6*/
            {
                hints.ai_family   = AF_INET;
                hints.ai_flags   |= AI_NUMERICHOST;
            }
        }

        /*get address info of server*/
        std::stringstream sPortNumber;
        sPortNumber << port;

        rc = getaddrinfo(server, sPortNumber.str().c_str(), &hints, &res);
        if (rc != 0)
        {
            LogPrintf("bftpclient not found server: %s, port: %d.\n", server, port);
            if (rc == EAI_SYSTEM) error("bftpclient getaddrinfo() failed\n");
            break;
        }

        /*create socket connection*/
        conn_sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (conn_sock_fd < 0)
        {
            LogPrint(BCLog::BFTP, "bftpclient socket() failed\n");
            break;
        }

        /*connect to server*/
        rc = connect(conn_sock_fd, res->ai_addr, res->ai_addrlen);
        if (rc < 0)
        {
            LogPrint(BCLog::BFTP, "bftpclient connect() failed\n");
            break;
        }

        /*read file and send to server*/
        LogPrint(BCLog::BFTP, "bftpclient: sending file :%s ....\n", filePath);
        int64_t result = Read_file(conn_sock_fd, boost::filesystem::path(filePath));
        if (result == 0)
        {
            LogPrint(BCLog::BFTP, "file was sent to server.\n");
        } else 
        {
            LogPrint(BCLog::BFTP, "got an error: %d when try to send file to server.\n", result);
        }
    } while (false);
    
}
