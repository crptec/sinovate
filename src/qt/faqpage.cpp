#include "faqpage.h"
#include "forms/ui_faqpage.h"
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <QScrollBar>
#include <QMetaObject>
#include <QPushButton>





FaqPage::FaqPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FaqPage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

	ui->pushButtonClose->setIcon(platformStyle->MultiStatesIcon(":/icons/close", PlatformStyle::PushButton));
    ui->labelContent3->setOpenExternalLinks(true);
    ui->labelContent6->setOpenExternalLinks(true);
        

    // Buttons
    connect(ui->pushButtonFaq1, &QPushButton::clicked, this, &FaqPage::onFaq1Clicked);
    connect(ui->pushButtonFaq2, &QPushButton::clicked, this, &FaqPage::onFaq2Clicked);
    connect(ui->pushButtonFaq3, &QPushButton::clicked, this, &FaqPage::onFaq3Clicked);
    connect(ui->pushButtonFaq4, &QPushButton::clicked, this, &FaqPage::onFaq4Clicked);
    connect(ui->pushButtonFaq5, &QPushButton::clicked, this, &FaqPage::onFaq5Clicked);
    connect(ui->pushButtonFaq6, &QPushButton::clicked, this, &FaqPage::onFaq6Clicked);
    
}
    
void FaqPage::showEvent(QShowEvent *event){
    if(pos != 0){
        QPushButton* btn = getButtons()[pos - 1];
        QMetaObject::invokeMethod(btn, "setChecked", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(btn, "clicked", Qt::QueuedConnection);
    }
}




void FaqPage::setSection(int num){
    if (num < 1 || num > 10)
        return;
    pos = num;
}

void FaqPage::onFaq1Clicked(){
    ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq1->y());
}

void FaqPage::onFaq2Clicked(){
   ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq2->y());
}

void FaqPage::onFaq3Clicked(){
   ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq3->y());
}

void FaqPage::onFaq4Clicked(){
    ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq4->y());
}

void FaqPage::onFaq5Clicked(){
    ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq5->y());
}

void FaqPage::onFaq6Clicked(){
    ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq6->y());
}


std::vector<QPushButton*> FaqPage::getButtons(){
    return {
            ui->pushButtonFaq1,
            ui->pushButtonFaq2,
            ui->pushButtonFaq3,
            ui->pushButtonFaq4,
            ui->pushButtonFaq5,
            ui->pushButtonFaq6,
    };
 	

}


FaqPage::~FaqPage()
{
    
    delete ui;
}

void FaqPage::on_pushButtonClose_clicked() {
     Q_EMIT pushButtonCloseClicked();
}
