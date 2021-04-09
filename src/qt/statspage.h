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

class WalletModel;

class StatsPage :  public QWidget
{
    Q_OBJECT

public:
    explicit StatsPage(const PlatformStyle* platformStyle, QWidget* parent = 0);
    ~StatsPage();
    
public Q_SLOTS:
    void getStatistics();
    void onResult(QNetworkReply* reply);

private:
    Ui::StatsPage* m_ui;
    QTimer* m_timer;
    const PlatformStyle* m_platformStyle;
    QNetworkAccessManager* m_networkManager;
};

#endif // STATSPAGE_H
