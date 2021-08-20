// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <qt/forms/ui_overviewpage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactionoverviewwidget.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>
#include <interfaces/wallet.h>
#include <qt/transactiondescdialog.h>
#include <qt/styleSheet.h>

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QDateTime>
#include <QPainter>
#include <QStatusTipEvent>
#include <QMessageBox>
#include <QTimer>
#include <QToolButton>
#include <QDesktopServices>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#define NUM_ITEMS 5
#define MARGIN 5
#define SYMBOL_WIDTH 80

#define TX_SIZE 40
#define DECORATION_SIZE 20
#define DATE_WIDTH 110
#define TYPE_WIDTH 140
#define AMOUNT_WIDTH 205

#define BUTTON_ICON_SIZE 24

Q_DECLARE_METATYPE(interfaces::WalletBalances)

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(BitcoinUnits::BTC),
        platformStyle(_platformStyle)
    {
        background_color_selected = GetStringStyleValue("txviewdelegate/background-color-selected", "#009ee5");
        background_color = GetStringStyleValue("txviewdelegate/background-color", "#393939");
        alternate_background_color = GetStringStyleValue("txviewdelegate/alternate-background-color", "#2e2e2e");
        foreground_color = GetStringStyleValue("txviewdelegate/foreground-color", "#dedede");
        foreground_color_selected = GetStringStyleValue("txviewdelegate/foreground-color-selected", "#ffffff");
        amount_color = GetStringStyleValue("txviewdelegate/amount-color", "#ffffff");
        color_unconfirmed = GetColorStyleValue("guiconstants/color-unconfirmed", COLOR_UNCONFIRMED);
        color_negative = GetColorStyleValue("guiconstants/color-negative", COLOR_NEGATIVE);
    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();
        bool selected = option.state & QStyle::State_Selected;
        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        if(selected)
        {
            icon = PlatformStyle::SingleColorIcon(icon, foreground_color_selected);
        }
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();

        QModelIndex ind = index.model()->index(index.row(), TransactionTableModel::Type, index.parent());
        QString typeString = ind.data(Qt::DisplayRole).toString();

        QRect mainRect = option.rect;
        QColor txColor = index.row() % 2 ? background_color : alternate_background_color;
        painter->fillRect(mainRect, txColor);

        if(selected)
        {
            painter->fillRect(mainRect.x()+1, mainRect.y()+1, mainRect.width()-2, mainRect.height()-2, background_color_selected);
        }

        QColor foreground = foreground_color;
        if(selected)
        {
            foreground = foreground_color_selected;
        }
        painter->setPen(foreground);

        QRect dateRect(mainRect.left() + MARGIN, mainRect.top(), DATE_WIDTH, TX_SIZE);
        painter->drawText(dateRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        int topMargin = (TX_SIZE - DECORATION_SIZE) / 2;
        QRect decorationRect(dateRect.topRight() + QPoint(MARGIN, topMargin), QSize(DECORATION_SIZE, DECORATION_SIZE));
        icon.paint(painter, decorationRect);

        QRect typeRect(decorationRect.right() + MARGIN, mainRect.top(), TYPE_WIDTH, TX_SIZE);
        painter->drawText(typeRect, Qt::AlignLeft|Qt::AlignVCenter, typeString);

        bool watchOnly = index.data(TransactionTableModel::WatchonlyRole).toBool();

        if (watchOnly)
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            if(selected)
            {
                iconWatchonly = PlatformStyle::SingleColorIcon(iconWatchonly, foreground_color_selected);
            }
            QRect watchonlyRect(typeRect.right() + MARGIN, mainRect.top() + topMargin, DECORATION_SIZE, DECORATION_SIZE);
            iconWatchonly.paint(painter, watchonlyRect);
            //address_rect_min_width += 5 + watchonlyRect.width();
        }

        int addressMargin = watchOnly ? MARGIN + 20 : MARGIN;
        int addressWidth = mainRect.width() - DATE_WIDTH - DECORATION_SIZE - TYPE_WIDTH - AMOUNT_WIDTH - 5*MARGIN;
        addressWidth = watchOnly ? addressWidth - 20 : addressWidth;

        QFont addressFont = option.font;
        addressFont.setPointSizeF(addressFont.pointSizeF() * 0.95);
        painter->setFont(addressFont);

        QFontMetrics fmName(painter->font());
        QString clippedAddress = fmName.elidedText(address, Qt::ElideRight, addressWidth);

        QRect addressRect(typeRect.right() + addressMargin, mainRect.top(), addressWidth, TX_SIZE);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, clippedAddress);

        QFont amountFont = option.font;
        painter->setFont(amountFont);

        if(amount < 0)
        {
            foreground = color_negative;
        }
        else if(!confirmed)
        {
            foreground = color_unconfirmed;
        }
        else
        {
            foreground = amount_color;
        }

        if(selected)
        {
            foreground = foreground_color_selected;
        }
        painter->setPen(foreground);

        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::SeparatorStyle::ALWAYS);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }

        QRect amountRect(addressRect.right() + MARGIN, addressRect.top(), AMOUNT_WIDTH, TX_SIZE);
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(TX_SIZE, TX_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

private:
    QColor background_color_selected;
    QColor background_color;
    QColor alternate_background_color;
    QColor foreground_color;
    QColor foreground_color_selected;
    QColor amount_color;
    QColor color_unconfirmed;
    QColor color_negative;
};

#include <qt/overviewpage.moc>

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    // ++ Price Stats
    m_timer(nullptr),
    // --
    ui(new Ui::OverviewPage),
    clientModel(nullptr),
    walletModel(nullptr),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    ui->toolButtonBlog->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    //ui->toolButtonBlog->setIcon(QIcon(":/icons/blog_blue"));
    ui->toolButtonBlog->setIconSize(QSize(64, 64));
    ui->toolButtonBlog->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/blog_white",PlatformStyle::NavBar));
   
    ui->toolButtonBlog->setStatusTip(tr("Visit Sinovate Blog"));

    ui->toolButtonDocs->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonDocs->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/docs_white",PlatformStyle::NavBar));
    ui->toolButtonDocs->setIconSize(QSize(64, 64));
    ui->toolButtonDocs->setStatusTip(tr("Visit Sinovate Docs"));

    
    ui->toolButtonExchanges->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonExchanges->setIconSize(QSize(64, 64));
    ui->toolButtonExchanges->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/cmc_white",PlatformStyle::NavBar));
    ui->toolButtonExchanges->setStatusTip(tr("Buy Sinovate Coin"));

    ui->toolButtonExplorer->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonExplorer->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/explorer_white",PlatformStyle::NavBar));
    ui->toolButtonExplorer->setIconSize(QSize(64, 64));
    ui->toolButtonExplorer->setStatusTip(tr("Visit Sinovate Block Explorer"));

    ui->toolButtonDiscord->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonDiscord->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/discord_white",PlatformStyle::NavBar));
    ui->toolButtonDiscord->setIconSize(QSize(64, 64));
    ui->toolButtonDiscord->setStatusTip(tr("Visit Sinovate Discord Channel"));

    ui->toolButtonRoadmap->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonRoadmap->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/roadmap_white",PlatformStyle::NavBar));
    ui->toolButtonRoadmap->setIconSize(QSize(64, 64));
    ui->toolButtonRoadmap->setStatusTip(tr("Sinovate Roadmap"));

    ui->toolButtonWebTool->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonWebTool->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/webtool_white",PlatformStyle::NavBar));
    ui->toolButtonWebTool->setIconSize(QSize(64, 64));
    ui->toolButtonWebTool->setStatusTip(tr("Visit Sinovate WebTool"));

    ui->toolButtonWallet->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonWallet->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/download_white",PlatformStyle::NavBar));
    ui->toolButtonWallet->setIconSize(QSize(64, 64));
    ui->toolButtonWallet->setStatusTip(tr("Download Latest Sinovate Wallets"));

    ui->toolButtonWhitePaper->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonWhitePaper->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/whitepaper_white",PlatformStyle::NavBar));
    ui->toolButtonWhitePaper->setIconSize(QSize(64, 64));
    ui->toolButtonWhitePaper->setStatusTip(tr("Sinovate LitePaper"));

    ui->toolButtonFaq->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui->toolButtonFaq->setIcon(platformStyle->MultiStatesIcon(":/styles/theme2/app-icons/faq_white",PlatformStyle::NavBar));
    ui->toolButtonFaq->setIconSize(QSize(64, 64));
    ui->toolButtonFaq->setStatusTip(tr("Open FAQ Page"));

    // Set stylesheet
    SetObjectStyleSheet(ui->labelWalletStatus, StyleSheetNames::ButtonTransparent);
    SetObjectStyleSheet(ui->labelTransactionsStatus, StyleSheetNames::ButtonTransparent);

    m_balances.balance = -1;


    // use a MultiStatesIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->MultiStatesIcon(":/icons/warning", PlatformStyle::PushButton);

    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);

    ui->labelDate->setFixedWidth(DATE_WIDTH);
    ui->labelType->setFixedWidth(TYPE_WIDTH);
    ui->labelAmount->setFixedWidth(AMOUNT_WIDTH);

    // Set send/receive icons
    ui->buttonSend->setIcon(platformStyle->MultiStatesIcon(":/icons/send", PlatformStyle::PushButton));
    ui->buttonSend->setIconSize(QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE));
    ui->buttonReceive->setIcon(platformStyle->MultiStatesIcon(":/icons/receiving_addresses", PlatformStyle::PushButton));
    ui->buttonReceive->setIconSize(QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE));

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (TX_SIZE + 2));
    ui->listTransactions->setMinimumWidth(590);
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listTransactions->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->listTransactions, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDetails()));

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);
    connect(ui->labelTransactionsStatus, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);

    // Price Stats
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(getStatistics()));
    m_timer->start(30000);
    getStatistics();
    // --
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

void OverviewPage::setPrivacy(bool privacy)
{
    m_privacy = privacy;
    if (m_balances.balance != -1) {
        setBalance(m_balances);
    }

    ui->listTransactions->setVisible(!m_privacy);

    const QString status_tip = m_privacy ? tr("Privacy mode activated for the Overview tab. To unmask the values, uncheck Settings->Mask values.") : "";
    setStatusTip(status_tip);
    QStatusTipEvent event(status_tip);
    QApplication::sendEvent(this, &event);
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

// ++ Price Stats
void OverviewPage::getStatistics()
{
    if (this->clientModel==nullptr) {
        return;
    }
    SINStatsStruct s = this->clientModel->getStats();

    if (s.lastPrice!=0.0)    {
        QLocale l = QLocale(QLocale::English);

        // Set BTC price strings
        ui->labelCurrentPriceBTC->setText(QString::number(s.lastPrice, 'f', 8));

        double currentBTC = s.lastPrice;
        double availableBTC = (currentBTC * totalBalance / 100000000);
        ui->labelBTCTotal->setText(QString::number(availableBTC, 'f', 8) + " BTC");

        // Set UDS price strings
        ui->labelCurrentPriceUSD->setText(QString::number(s.usdPrice, 'f', 8));

        double currentUSD = s.usdPrice;
        double availableUSD = (currentUSD * totalBalance / 100000000);
        ui->labelUSDTotal->setText("$" + QString::number(availableUSD, 'f', 2) + " USD");

        // Set EUR price strings
        ui->labelCurrentPriceEUR->setText(QString::number(s.eurPrice, 'f', 8));

        double currentEUR = s.eurPrice;
        double availableEUR = (currentEUR * totalBalance / 100000000);
        ui->labelEURTotal->setText(QString::number(availableEUR, 'f', 2) + " EUR");
    }
    else
    {
        const QString noValue = "NaN";

        ui->labelCurrentPriceBTC->setText(noValue);
        ui->labelCurrentPriceUSD->setText(noValue);
        ui->labelCurrentPriceEUR->setText(noValue);
    }
}
// --

void OverviewPage::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;

// ++ Price Stats
    totalBalance = balances.balance + balances.unconfirmed_balance + balances.immature_balance;
// --
    
    if (walletModel->wallet().isLegacy()) {
        if (walletModel->wallet().privateKeysDisabled()) {
            ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.unconfirmed_watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.immature_watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            //ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        } else {
            ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.unconfirmed_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.immature_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            //ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.balance + balances.unconfirmed_balance + balances.immature_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.unconfirmed_watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.immature_watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
            ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        }
    } else {
        ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.unconfirmed_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.immature_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        //ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.balance + balances.unconfirmed_balance + balances.immature_balance, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    }
    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = balances.immature_balance != 0;
    bool showWatchOnlyImmature = balances.immature_watch_only_balance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    //ui->widgetImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->widgetWatchImmature->setVisible(!walletModel->wallet().privateKeysDisabled() && showWatchOnlyImmature); // show watch-only immature balance
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    //ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->widgetWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->widgetWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->widgetWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly)
    {
        ui->widgetWatchImmature->hide();
    }
    
    ui->widgetWatch->setVisible(showWatchOnly);
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model) {
        // Show warning, for example if this is a prerelease version
        connect(model, &ClientModel::alertsChanged, this, &OverviewPage::updateAlerts);
        updateAlerts(model->getStatusBarWarnings());

        //connect(model->getOptionsModel(), &OptionsModel::useEmbeddedMonospacedFontChanged, this, &OverviewPage::setMonospacedFont);
        //setMonospacedFont(model->getOptionsModel()->getUseEmbeddedMonospacedFont());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, &WalletModel::balanceChanged, this, &OverviewPage::setBalance);

        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &OverviewPage::updateDisplayUnit);

        updateWatchOnlyLabels(wallet.haveWatchOnly() && !model->wallet().privateKeysDisabled());
        connect(model, &WalletModel::notifyWatchonlyChanged, [this](bool showWatchOnly) {
            updateWatchOnlyLabels(showWatchOnly && !walletModel->wallet().privateKeysDisabled());
        });
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::on_showMoreButton_clicked()
{
    Q_EMIT showMoreClicked();
}

void OverviewPage::showDetails()
{
    if(!ui->listTransactions->selectionModel())
        return;
    QModelIndexList selection = ui->listTransactions->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog *dlg = new TransactionDescDialog(selection.at(0));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}

void OverviewPage::on_buttonSend_clicked()
{
    Q_EMIT sendCoinsClicked();
}

void OverviewPage::on_buttonReceive_clicked()
{
    Q_EMIT receiveCoinsClicked();
}

void OverviewPage::showTransactionWidget(bool bShow)   {
    if (bShow)  {
        ui->widgetTransaction->show();
        ui->widgetRecent->show();
        ui->hLine->show();
    }
    else {        
        ui->widgetTransaction->hide();
        ui->widgetRecent->hide();
        ui->hLine->hide();
    }
}

void OverviewPage::showToolBoxWidget(bool bShow)   {
    if (bShow) {  
        ui->toolBoxWidget->show();
        ui->widgetTransaction->hide();
        ui->widgetRecent->hide();
        ui->hLine->hide();
    }
    else {         
        ui->toolBoxWidget->hide();
        ui->widgetTransaction->show();
        ui->widgetRecent->show();
        ui->hLine->show();
    }
}

void OverviewPage::on_toolButtonDiscord_clicked() {
    QDesktopServices::openUrl(QUrl("https://sinovate.io/links/discord/", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonBlog_clicked() {
    QDesktopServices::openUrl(QUrl("https://sinovate.io/blog/", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonDocs_clicked() {
    QDesktopServices::openUrl(QUrl("https://docs.sinovate.io/", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonExchanges_clicked() {
    QDesktopServices::openUrl(QUrl("https://coinmarketcap.com/currencies/sinovate/markets/", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonExplorer_clicked() {
    QDesktopServices::openUrl(QUrl("https://explorer.sinovate.io/", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonRoadmap_clicked() {
    QDesktopServices::openUrl(QUrl("https://sinovate.io/roadmap/", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonWallet_clicked() {
    QDesktopServices::openUrl(QUrl("https://github.com/SINOVATEblockchain/SIN-core/releases", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonWebTool_clicked() {
    QDesktopServices::openUrl(QUrl("https://docs.sinovate.io/#/sin_webtool_guide", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonWhitePaper_clicked() {
    QDesktopServices::openUrl(QUrl("https://sinovate.io/SINOVATE_litepaper.pdf", QUrl::TolerantMode));
}

void OverviewPage::on_toolButtonFaq_clicked() {
     Q_EMIT toolButtonFaqClicked();
}
