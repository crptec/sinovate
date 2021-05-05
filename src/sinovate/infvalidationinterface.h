// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFVALIDATIONINTERFACE_H
#define SIN_INFVALIDATIONINTERFACE_H
#include <validationinterface.h>

class InfValidationInterface : public CValidationInterface
{
public:
    InfValidationInterface(CConnman& connmanIn);
    virtual ~InfValidationInterface();

    bool registerStatus(){return fRegister;}

protected:
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;

private:
    bool fRegister = false;
    CConnman& connman;
};

extern InfValidationInterface* g_inf_validation_interface;

#endif // SIN_INFVALIDATIONINTERFACE_H
