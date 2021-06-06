#include <qt/clientmodel.h>
#include "statspage.h"
#include "forms/ui_statspage.h"
#include <qt/platformstyle.h>

#include <QTimer>

StatsPage::StatsPage(const PlatformStyle* platformStyle, QWidget *parent) :
    QWidget(parent),
    m_timer(nullptr),
    m_ui(new Ui::StatsPage),
    clientModel(nullptr),
    m_platformStyle(platformStyle)
{
    m_ui->setupUi(this);

 	m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(getStatistics()));
    m_timer->start(30000);
    getStatistics();
}

StatsPage::~StatsPage()
{
    delete m_ui;
    
}

void StatsPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void StatsPage::getStatistics()
{
    if (this->clientModel==nullptr) {
        return;
    }
    SINStatsStruct s = this->clientModel->getStats();

    if (s.lastPrice!=0.0)    {
        QLocale l = QLocale(QLocale::English);
        // Set NETWORK strings
        QString heightValue(s.blockcount);
        QString knownHashrateString = QString::number(s.known_hashrate/1000000, 'f', 2);
        QString hashrateString = knownHashrateString;
        m_ui->hashrateValueLabel->setText(hashrateString +" MH/s");
        m_ui->difficultyValueLabel->setText(s.difficulty);
        m_ui->lastPriceValueLabel->setText(QString::number(s.lastPrice, 'f', 8) + QString(" BTC"));
        m_ui->heightValueLabel->setText(heightValue);


        // Set ADDRESS STATS strings
        int top10 = s.explorerTop10;
        int top50 = s.explorerTop50;
        m_ui->addressesValueLabel->setText(s.explorerAddresses);
        m_ui->activeValueLabel->setText(s.explorerActiveAddresses);
        m_ui->top10ValueLabel->setText(l.toString(top10) + " SIN");
        m_ui->top50ValueLabel->setText(l.toString(top50) + " SIN");

        // Set BURNT COIN STATS strings
        int supplyNumber = s.supply - s.burnFee;
        int feeNumber = s.burnFee - s.burnNode;
        int burntNumber = s.burnFee;

        m_ui->feeValueLabel->setText(l.toString(feeNumber)+ " SIN");
        m_ui->nodesValueLabel->setText(l.toString(s.burnNode)+ " SIN");
        m_ui->totalBurntValueLabel->setText(l.toString(burntNumber)+ " SIN");
        m_ui->totalSupplyValueLabel->setText(l.toString(supplyNumber)+ " SIN");
        m_ui->circulationSupplyValueLabel->setText(l.toString(supplyNumber)+ " SIN");

        // Set INFINITY NODE STATS strings
        int bigRoiDays = 1000000/((720/s.inf_online_big)*1752);
        int midRoiDays = 500000/((720/s.inf_online_mid)*838);
        int lilRoiDays = 100000/((720/s.inf_online_lil)*560);

        //++
        int bigRoiDaysPercent = (365/(1000000/((720/s.inf_online_big)*1752)))*100-100;
        int midRoiDaysPercent = (365/(500000/((720/s.inf_online_mid)*838)))*100-100;
        int lilRoiDaysPercent = (365/(100000/((720/s.inf_online_lil)*560)))*100-100;

        QString bigROIStringPercent = QString::number(bigRoiDaysPercent, 'f', 0);
        QString midROIStringPercent = QString::number(midRoiDaysPercent, 'f', 0);
        QString lilROIStringPercent = QString::number(lilRoiDaysPercent, 'f', 0);
        //--

        QString bigROIString = "ROI: " + QString::number(bigRoiDays) + " days" ;
        QString midROIString = "ROI: " + QString::number(midRoiDays) + " days";
        QString lilROIString = "ROI: " + QString::number(lilRoiDays) + " days";
        QString totalNodesString = QString::number(s.inf_online_big + s.inf_online_mid + s.inf_online_lil);

        QString bigString = QString::number(s.inf_online_big);
        QString midString = QString::number(s.inf_online_mid);
        QString lilString = QString::number(s.inf_online_lil);
        m_ui->bigValueLabel->setText(bigString);
        m_ui->midValueLabel->setText(midString);
        m_ui->lilValueLabel->setText(lilString);
        m_ui->totalValueLabel->setText(totalNodesString);
        m_ui->bigRoiLabel->setText(bigROIStringPercent + "%");
        m_ui->midRoiLabel->setText(midROIStringPercent + "%");
        m_ui->miniRoiLabel->setText(lilROIStringPercent + "%");
    }
    else
    {
        const QString noValue = "NaN";
        m_ui->hashrateValueLabel->setText(noValue);
        m_ui->difficultyValueLabel->setText(noValue);
        m_ui->lastPriceValueLabel->setText(noValue);
        m_ui->heightValueLabel->setText(noValue);

        m_ui->addressesValueLabel->setText(noValue);
        m_ui->activeValueLabel->setText(noValue);
        m_ui->top10ValueLabel->setText(noValue);
        m_ui->top50ValueLabel->setText(noValue);

        m_ui->feeValueLabel->setText(noValue);
        m_ui->nodesValueLabel->setText(noValue);
        m_ui->totalBurntValueLabel->setText(noValue);
        m_ui->totalSupplyValueLabel->setText(noValue);

        m_ui->bigValueLabel->setText(noValue);
        m_ui->midValueLabel->setText(noValue);
        m_ui->lilValueLabel->setText(noValue);
        m_ui->totalValueLabel->setText(noValue);
    }
}
