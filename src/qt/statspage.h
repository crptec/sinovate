#ifndef STATSPAGE_H
#define STATSPAGE_H
#include "walletmodel.h"
#include <QWidget>
#include <QtNetwork/QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

namespace Ui {
class StatsPage;
}

class ClientModel;
class WalletModel;

class StatsPage :  public QWidget
{
    Q_OBJECT

public:
    explicit StatsPage(const PlatformStyle* platformStyle, QWidget* parent = 0);
    ~StatsPage();
    void setClientModel(ClientModel *clientModel);
    
public Q_SLOTS:
    void getStatistics();

private:
    Ui::StatsPage* m_ui;
    ClientModel *clientModel;
    QTimer* m_timer;
    const PlatformStyle* m_platformStyle;
};

#endif // STATSPAGE_H
