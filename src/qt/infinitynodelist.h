// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2018-2021 SIN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FXTC_QT_INFINITYNODELIST_H
#define FXTC_QT_INFINITYNODELIST_H

#include <primitives/transaction.h>
#include <qt/platformstyle.h>
//#include <sync.h>
//#include <util.h>
#include <wallet/wallet.h>
#include <qt/walletmodel.h>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QWidget>
#include <QJsonObject>
#include <univalue.h>
#include <QNetworkReply>

#define MY_INFINITYNODELIST_UPDATE_SECONDS                 60
#define INFINITYNODELIST_UPDATE_SECONDS                    15
#define INFINITYNODELIST_FILTER_COOLDOWN_SECONDS            3

namespace Ui {
    class InfinitynodeList;
}

class ClientModel;
class OptionsModel;
class QNetworkAccessManager;
class UniValue;
class QEvent;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

typedef std::pair<unsigned int, std::string> pair_burntx;
typedef std::pair<QString, QString> pair_nodestatus;

struct vectorBurnTxCompare {
    bool operator()(std::pair<std::string, pair_burntx> const&left,
      std::pair<std::string, pair_burntx> const&right) {
        return left.second.first > right.second.first;
    }
};

struct infinitynode_conf_t
{
    std::string IPaddress = "";
    int port = 20970;
    std::string infinitynodePrivateKey;
    std::string collateralHash = "";
    int collateralIndex = 0;
    std::string burnFundHash = "";
    int burnFundIndex = 0;
    CTxDestination collateralAddress{};
};

class DINColumnsEventHandler : public QObject  {

protected:
   virtual bool eventFilter(QObject *pQObj, QEvent *pQEvent) override;
};

/** Masternode Manager page widget */
class InfinitynodeList : public QWidget
{
    Q_OBJECT

public:
    explicit InfinitynodeList(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~InfinitynodeList();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showTab_setUP(bool fShow);

private:
    QMenu *contextDINMenu;
    QMenu *contextDINColumnsMenu;
    DINColumnsEventHandler *menuEventHandler;
    std::vector<std::pair<int, QAction*>> contextDINColumnsActions;
    int64_t nTimeFilterUpdated;
    bool fFilterUpdated;
    interfaces::WalletBalances m_balances;

    QLabel* labelPic[8];
    QLabel* labelTxt[8];
    int currentStep = 0;

public Q_SLOTS:
    bool isSynced();
    void updateDINList();
    bool filterNodeRow( int nRow );
    void setBalance(const interfaces::WalletBalances& balances);

    // node setup functions
    void nodeSetupInitialize();
    void nodeSetupCleanProgress();
    UniValue nodeSetupCallRPC(std::string args);
    void nodeSetupEnableOrderUI( bool bEnable, int orderID = 0, int invoiceID = 0 );
    int nodeSetupAPIAddClient( QString firstName, QString lastName, QString email, QString password, QString& strError );
    int nodeSetupAPIAddOrder( int clientid, QString billingCycle, QString& productids, int& invoiceid, QString email, QString password, QString& strError );
    bool nodeSetupAPIGetInvoice( int invoiceid, QString& strAmount, QString& strStatus, QString& paymentAddress, QString email, QString password, QString& strError );
    std::map<int,std::string> nodeSetupAPIListInvoices( QString email, QString password, QString& strError );
    QJsonObject nodeSetupAPIInfo( int serviceid, int clientid, QString email, QString password, QString& strError );
    QJsonObject nodeSetupAPINodeInfo( int serviceid, int clientId, QString email, QString pass, QString& strError  );
    bool nodeSetupAPINodeList( QString email, QString pass, QString& strError  );
    bool nodeSetupCheckFunds( CAmount invoiceAmount = 0 );
    void nodeSetupStep( std::string icon , std::string text );
    int  nodeSetupGetClientId( QString& email, QString& pass, bool bSilent = false);
    void nodeSetupSetClientId( int clientId, QString email, QString pass );
    void nodeSetupEnableClientId( int clientId );
    int  nodeSetupGetOrderId( int& invoiceid, QString& mProductIds );
    void nodeSetupSetOrderId( int orderid , int invoiceid, QString strProductIds );
    QString  nodeSetupGetBurnTx( );
    void nodeSetupSetBurnTx( QString strBurnTx );
    QString nodeSetupGetPaymentTx( );
    void nodeSetupSetPaymentTx( QString txHash );
    int nodeSetupGetServiceForNodeAddress( QString nodeAdress );
    void nodeSetupSetServiceForNodeAddress( QString nodeAdress, int serviceId );
    void nodeSetupResetClientId( );
    void nodeSetupResetOrderId( );
    void nodeSetupPopulateInvoicesCombo( );
    void nodeSetupPopulateBurnTxCombo( );

    QString nodeSetupCheckInvoiceStatus();
    void nodeSetupCheckPendingPayments();
    QString nodeSetupRPCBurnFund( QString collateralAddress, CAmount amount, QString backupAddress );
    int nodeSetupGetBurnAmount();
    QString nodeSetupGetNewAddress();
    UniValue nodeSetupGetTxInfo( QString txHash, std::string attribute);
    QString nodeSetupSendToAddress( QString strAddress, int amount, QTimer* timerConfirms );
    void nodeSetupCheckBurnPrepareConfirmations();
    void nodeSetupCheckBurnSendConfirmations();
    std::map<std::string, pair_burntx> nodeSetupGetUnusedBurnTxs( );;    
    QString nodeSetupGetOwnerAddressFromBurnTx( QString burnTx );
    bool nodeSetupUnlockWallet();
    void nodeSetupLockWallet();
    QString nodeSetupGetRPCErrorMessage( UniValue objError );
    QString nodeSetupGetNodeType(CAmount amount);
    void nodeSetupCheckDINNode(int nSelectedRow, bool bShowMsg = false );
    void nodeSetupCheckAllDINNodes();
    void nodeSetupCheckDINNodeTimer();

Q_SIGNALS:

private:
    QTimer *motdTimer;
    QTimer* m_timer;
    Ui::InfinitynodeList *ui;
    QNetworkAccessManager* motd_networkManager;
    QNetworkRequest* motd_request;
    QTimer *timer;
    QTimer* timerSingleShot;
    ClientModel *clientModel;
    WalletModel *walletModel;

    QString strCurrentFilter;
    bool bDINNodeAPIUpdate = false;

    // nodeSetup
    QTimer *invoiceTimer;
    QTimer *burnPrepareTimer;
    QTimer *burnSendTimer;
    QTimer *pendingPaymentsTimer;
    QTimer *checkAllNodesTimer;

    int nCheckAllNodesCurrentRow;
    QNetworkAccessManager *ConnectionManager;
    QString NODESETUP_ENDPOINT_BASIC;
    QString NODESETUP_ENDPOINT_NODE;
    QString NODESETUP_RESTORE_URL;
    QString NODESETUP_SUPPORT_URL;
    QString NODESETUP_PID;
    int NODESETUP_UPDATEMETA_AMOUNT;
    int NODESETUP_CONFIRMS;
    int NODESETUP_REFRESHCOMBOS;    // every N updateDINList cycles
    int nodeSetup_RefreshCounter;
    int mClientid, mOrderid, mInvoiceid, mServiceId;
    bool bNodeSetupLogged = false;
    std::map<std::string, int> nodeSetupUsedBurnTxs;
    std::map<std::string, int> nodeSetupPendingPayments;
    std::map<QString, QString> nodeSetupTempIPInfo;     // burntx -> IP from API
    std::map<QString, pair_nodestatus> nodeSetupNodeInfoCache;     // address ->  pair(blockcount, status)
    QString mPaymentTx;
    QString mBurnPrepareTx;
    QString mBurnAddress;
    QString mBurnTx;
    QString mMetaTx;
    QString mProductIds;
    std::string billingOptions[3] = {tr("Monthly").toStdString(), tr("Semiannually").toStdString(), tr("Annually").toStdString()};
    QAction *mCheckNodeAction;
    QAction *mCheckAllNodesAction;
    WalletModel::UnlockContext *pUnlockCtx = NULL;

private Q_SLOTS:
    void showContextDINMenu(const QPoint &);
    void showContextDINColumnsMenu(const QPoint &);
    void nodeSetupDINColumnToggle(int nColumn );
    void on_checkDINNode();
    void on_payButton_clicked();
    void on_setupSinovateButton_clicked();

    // node setup
    void on_btnSetup_clicked();
    void on_btnLogin_clicked();
    void on_btnSetupReset_clicked();
    void on_btnRestore_clicked();
    void getStatistics();
    void loadMotd();
    void updateDisplayUnit();
};

#endif // FXTC_QT_INFINITYNODELIST_H
