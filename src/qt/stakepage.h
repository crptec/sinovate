#ifndef STAKEPAGE_H
#define STAKEPAGE_H
#include "walletmodel.h"
#include <QWidget>

namespace Ui {
class StakePage;
}

class WalletModel;




class StakePage :  public QWidget

{
    Q_OBJECT

public:
    explicit StakePage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~StakePage();

public Q_SLOTS:


private:
    Ui::StakePage *ui;
    const PlatformStyle *platformStyle;



};

#endif // STAKEPAGE_H 