#include <qt/passwordlineedit.h>
#include <qt/styleSheet.h>

PasswordLineEdit::PasswordLineEdit(QWidget *parent) : QLineEdit(parent) {
  setEchoMode(QLineEdit::Password);
  eyeOnIcon = GetStringStyleValue("appstyle/eyeOn-icon", ":/styles/theme1/app-icons/eyeOn");
  eyeOffIcon = GetStringStyleValue("appstyle/eyeOff-icon", ":/styles/theme1/app-icons/eyeOff");
  QAction *action = addAction(QIcon(eyeOffIcon), QLineEdit::TrailingPosition);
  button = qobject_cast<QToolButton *>(action->associatedWidgets().last());
  button->hide();
  button->setCursor(QCursor(Qt::PointingHandCursor));
  connect(button, &QToolButton::pressed, this, &PasswordLineEdit::onPressed);
  connect(button, &QToolButton::released, this, &PasswordLineEdit::onReleased);
}

void PasswordLineEdit::onPressed() {
  QToolButton *button = qobject_cast<QToolButton *>(sender());
  button->setIcon(QIcon(eyeOnIcon));
  //button->setIcon(platformStyle->MultiStatesIcon(":/icons/eyeOn",PlatformStyle::NavBar));
  setEchoMode(QLineEdit::Normal);
}

void PasswordLineEdit::onReleased() {
  QToolButton *button = qobject_cast<QToolButton *>(sender());
  button->setIcon(QIcon(eyeOffIcon));
  //button->setIcon(platformStyle->MultiStatesIcon(":/icons/eyeOff",PlatformStyle::NavBar));
  setEchoMode(QLineEdit::Password);
}

void PasswordLineEdit::enterEvent(QEvent *event) {
  button->show();
  QLineEdit::enterEvent(event);
}

void PasswordLineEdit::leaveEvent(QEvent *event) {
  button->hide();
  QLineEdit::leaveEvent(event);
}

void PasswordLineEdit::focusInEvent(QFocusEvent *event) {
  button->show();
  QLineEdit::focusInEvent(event);
}

void PasswordLineEdit::focusOutEvent(QFocusEvent *event) {
  button->hide();
  QLineEdit::focusOutEvent(event);
}
