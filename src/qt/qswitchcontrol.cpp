#include "qswitchcontrol.h"
#include <qt/styleSheet.h>

#include <QPushButton>
#include <QPropertyAnimation>
#include <QStyleOption>
#include <QPainter>
#include <QMessageBox>
#include <QCheckBox>
#include <QSettings>

static const QSize FrameSize = QSize(68, 30);
static const QSize SwitchSize = QSize (26, 26);
static const int SwitchOffset = (FrameSize.height() - SwitchSize.height()) / 2;

static const QString CustomFrameOnStlye = QString("QAbstractButton { border: none; border-radius: %1; background-color: #4697D9;}").arg(FrameSize.height() / 2);
static const QString CustomFrameOffStlye = QString("QAbstractButton { border: none; border-radius: %1; background-color: #6f80ab;}").arg(FrameSize.height() / 2);
static const QString CustomButtonStlye = QString("QPushButton { min-width: 0em; border-radius: %1; background-color: white;}").arg(SwitchSize.height() / 2);

QSwitchControl::QSwitchControl(QWidget *parent):
    QAbstractButton(parent)
{
    this->setFixedSize(FrameSize);

    pbSwitch = new QPushButton(this);
    pbSwitch->setFixedSize(SwitchSize);
    pbSwitch->setStyleSheet(CustomButtonStlye);

    animation = new QPropertyAnimation(pbSwitch, "geometry", this);
    animation->setDuration(200);

    connect(this, &QSwitchControl::mouseClicked, this, &QSwitchControl::onStatusChanged);
    connect(pbSwitch, &QPushButton::clicked, this, &QSwitchControl::onStatusChanged);
    setCheckable(true);
    setChecked(false);
}

void QSwitchControl::setChecked(bool checked)
{
    if(checked)
    {
        pbSwitch->move(this->width() - pbSwitch->width() - SwitchOffset, this->y() + SwitchOffset);
        this->setStyleSheet(CustomFrameOnStlye);
    }
    else
    {
        pbSwitch->move(this->x() + SwitchOffset, this->y() + SwitchOffset);
        this->setStyleSheet(CustomFrameOffStlye);
    }

    QAbstractButton::setChecked(checked);
}

void QSwitchControl::onStatusChanged()
{
    bool checked = !isChecked();

    QRect currentGeometry(pbSwitch->x(), pbSwitch->y(), pbSwitch->width(), pbSwitch->height());

    if(animation->state() == QAbstractAnimation::Running)
        animation->stop();

    if(checked)
    {
        //++
        QSettings settings;
        QCheckBox *warnStakeCheckBoxShowHide = new QCheckBox(tr("Don't show this again"));
        bool warnStakeShowStatus;
        QVariant IsHidden = settings.value("warnStakeShowStatus", warnStakeShowStatus);

            if (IsHidden == false){
            QMessageBox *stakeWarnDialog = new QMessageBox(this);
            stakeWarnDialog->setWindowModality(Qt::WindowModal);
            stakeWarnDialog->setWindowTitle(tr("Confirm Staking!"));
            stakeWarnDialog->setCheckBox(warnStakeCheckBoxShowHide);
            stakeWarnDialog->setText(tr("<h2>WARNING!</h2>")  
                    + tr("When the <b>Staking</b> button is turned on, all the available coins will start staking and will not be available for 14400 blocks (~10 days)")
                    + "<br><br>" + tr("If some coins are not intended for staking, <b>first go to INPUTS in SEND Tab and lock some of the inputs!</b>")
                    +"<br><br>"+ tr("Are you sure?")+"<br><br><br>");
            stakeWarnDialog->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            stakeWarnDialog->setIcon(QMessageBox::Warning);
            stakeWarnDialog->button(QMessageBox::Yes)->setObjectName("Yes");
            stakeWarnDialog->button(QMessageBox::No)->setObjectName("No");
            stakeWarnDialog->setButtonText(QMessageBox::Yes, tr("Yes"));
            stakeWarnDialog->setButtonText(QMessageBox::No, tr("No"));
            SetObjectStyleSheet(stakeWarnDialog->button(QMessageBox::Yes), StyleSheetNames::ButtonCustom);
            SetObjectStyleSheet(stakeWarnDialog->button(QMessageBox::No), StyleSheetNames::ButtonCustom);
            stakeWarnDialog->setStyleSheet("color: #6F80AB");
            warnStakeCheckBoxShowHide->setStyleSheet("background-color: transparent; font: 9px;");
            int click = stakeWarnDialog->exec();
            warnStakeShowStatus = warnStakeCheckBoxShowHide->isChecked();        
            settings.setValue("warnStakeShowStatus", warnStakeShowStatus);
                if (click == QMessageBox::No) {
                return;

                }
            }
        //--
        this->setStyleSheet(CustomFrameOnStlye);

        animation->setStartValue(currentGeometry);
        animation->setEndValue(QRect(this->width() - pbSwitch->width() - SwitchOffset, pbSwitch->y(), pbSwitch->width(), pbSwitch->height()));
    }
    else
    {
        this->setStyleSheet(CustomFrameOffStlye);

        animation->setStartValue(currentGeometry);
        animation->setEndValue(QRect(SwitchOffset, pbSwitch->y(), pbSwitch->width(), pbSwitch->height()));
    }
    animation->start();

    setChecked(checked);
    Q_EMIT clicked(checked);
}

void QSwitchControl::mousePressEvent(QMouseEvent *)
{
    Q_EMIT mouseClicked();
}

void QSwitchControl::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
