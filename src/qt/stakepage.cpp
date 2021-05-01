#include "stakepage.h"
#include "forms/ui_stakepage.h"
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>





StakePage::StakePage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StakePage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
    



}


StakePage::~StakePage()
{

    delete ui;
}

