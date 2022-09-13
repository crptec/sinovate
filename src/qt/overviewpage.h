// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OVERVIEWPAGE_H
#define BITCOIN_QT_OVERVIEWPAGE_H

#include <interfaces/wallet.h>

#include <QWidget>
#include <memory>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;

namespace Ui {
    class OverviewPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);

    void showTransactionWidget(bool bShow);
    void showToolBoxWidget(bool bShow);

public Q_SLOTS:
    void setBalance(const interfaces::WalletBalances& balances);
    void setPrivacy(bool privacy);

Q_SIGNALS:
    void showMoreClicked();
    void transactionClicked(const QModelIndex &index);
    void outOfSyncWarningClicked();
    void sendCoinsClicked(QString addr = "");
    void receiveCoinsClicked();
    void toolButtonFaqClicked();

protected:
    void changeEvent(QEvent* e) override;

private:
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    interfaces::WalletBalances m_balances;
    QTimer* m_timer;
    qint64 totalBalance;
    bool m_privacy{false};

    const PlatformStyle* m_platform_style;

    TxViewDelegate *txdelegate;
    std::unique_ptr<TransactionFilterProxy> filter;

private Q_SLOTS:
    void updateDisplayUnit();
    void updateAlerts(const QString &warnings);
    void updateWatchOnlyLabels(bool showWatchOnly);

    void on_showMoreButton_clicked();
    void on_buttonSend_clicked();
    void on_buttonReceive_clicked();
    void showDetails();
    void getStatistics();
    void on_toolButtonBlog_clicked();
    void on_toolButtonDocs_clicked();
    void on_toolButtonExchanges_clicked();
    void on_toolButtonExplorer_clicked();
    void on_toolButtonRoadmap_clicked();
    void on_toolButtonWallet_clicked();
    void on_toolButtonWebTool_clicked();
    void on_toolButtonWhitePaper_clicked();
    void on_toolButtonDiscord_clicked();
    void on_toolButtonFaq_clicked();
};

#endif // BITCOIN_QT_OVERVIEWPAGE_H
