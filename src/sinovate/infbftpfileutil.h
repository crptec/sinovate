// Copyright (c) 2018-2012 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFBFTPFILEUTIL_H
#define SIN_INFBFTPFILEUTIL_H

#include <logging.h>
#include <protocol.h>
#include <net.h>
#include <serialize.h>

using namespace std;

/**
 * @brief: use this class to analyse the local file and get metadata.
 * 
 * Metadata infos:
 *    1. filename
 *    2. filesize
 *    3. hash
 *    4. read to DataStream
 *
 * @param _filePath: full path of reading file
 */
class CBftpFileUtil
{
private:
    bool Read(CDataStream& msg)
    {
        msg.clear();

        int64_t nStart = GetTimeMillis();
        // open input file, and associate with CAutoFile
        FILE *file = fopen(filePath.string().c_str(), "rb");
        CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
        if (filein.IsNull())
        {
            error("%s: Failed to open file %s", __func__, filePath.string());
            return false;
        }

        int fileSize = boost::filesystem::file_size(filePath);
        std::vector<unsigned char> vchData;
        vchData.resize(fileSize);

        try {
            filein.read((char *)&vchData[0], fileSize);
        }
        catch (std::exception &e) {
            error("%s: Deserialize or I/O error - %s", __func__, e.what());
            return false;
        }
        filein.fclose();

        CDataStream obj(vchData, SER_DISK, CLIENT_VERSION);
        msg << obj;
        return true;
    }

public:
    boost::filesystem::path filePath;

    CBftpFileUtil(boost::filesystem::path _filePath)
    {
        filePath = _filePath;
    }

    bool LoadToNetMsg(CDataStream& msg)
    {
        return Read(msg);
    }
};

#endif // SIN_INFBFTPFILEUTIL_H