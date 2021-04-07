#ifndef SINPUSHBUTTON_H
#define SINPUSHBUTTON_H
#include <QPushButton>
#include <QStyleOptionButton>
#include <QIcon>

class SinPushButton : public QPushButton
{
public:
    explicit SinPushButton(QWidget * parent = Q_NULLPTR);
    explicit SinPushButton(const QString &text, QWidget *parent = Q_NULLPTR);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    void updateIcon(QStyleOptionButton &pushbutton);

private:
    bool m_iconCached;
    QIcon m_downIcon;
};

#endif // SINPUSHBUTTON_H
