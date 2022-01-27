#ifndef PASSWORDLINEEDIT_H
#define PASSWORDLINEEDIT_H

#include <QAction>
#include <QLineEdit>
#include <QToolButton>

class PasswordLineEdit : public QLineEdit {
public:
  PasswordLineEdit(QWidget *parent = nullptr);
private Q_SLOTS:
  void onPressed();
  void onReleased();

protected:
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);

private:
  QToolButton *button;
  QString eyeOffIcon;
  QString eyeOnIcon;
};

#endif // PASSWORDLINEEDIT_H
