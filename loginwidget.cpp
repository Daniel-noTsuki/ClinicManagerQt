#include "loginwidget.h"
#include "databasemanager.h"

#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginWidget::LoginWidget(DatabaseManager *database, QWidget *parent)
    : QWidget(parent), m_database(database) {
    setWindowTitle(QStringLiteral("诊疗信息管理系统登录"));
    resize(760, 480);

    auto *brand = new QLabel(QStringLiteral("诊疗信息管理系统"), this);
    brand->setObjectName(QStringLiteral("brandTitle"));
    auto *subtitle = new QLabel(QStringLiteral("Qt SQL · Model View"), this);
    subtitle->setObjectName(QStringLiteral("brandSubtitle"));
    auto *hint = new QLabel(QStringLiteral("使用课程实验账号进入患者信息管理界面"), this);

    m_username = new QLineEdit(this);
    m_username->setPlaceholderText(QStringLiteral("用户名"));
    m_username->setText(QStringLiteral("admin"));
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(QStringLiteral("密码"));
    m_message = new QLabel(QStringLiteral("默认账号 admin / admin123"), this);
    m_message->setObjectName(QStringLiteral("mutedText"));

    auto *loginButton = new QPushButton(QStringLiteral("登录"), this);
    loginButton->setDefault(true);
    loginButton->setObjectName(QStringLiteral("primaryButton"));
    connect(loginButton, &QPushButton::clicked, this, &LoginWidget::attemptLogin);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginWidget::attemptLogin);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("用户名"), m_username);
    form->addRow(QStringLiteral("密码"), m_password);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("loginCard"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(42, 38, 42, 38);
    cardLayout->setSpacing(18);
    cardLayout->addWidget(brand);
    cardLayout->addWidget(subtitle);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(hint);
    cardLayout->addLayout(form);
    cardLayout->addWidget(m_message);
    cardLayout->addWidget(loginButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(120, 60, 120, 60);
    layout->addWidget(card);
}

void LoginWidget::setCredentials(const QString &username, const QString &password) {
    m_username->setText(username);
    m_password->setText(password);
}

void LoginWidget::attemptLogin() {
    QString fullName;
    if (!m_database->authenticate(m_username->text(), m_password->text(), &fullName)) {
        m_message->setText(QStringLiteral("用户名或密码错误，请重新输入"));
        m_message->setStyleSheet(QStringLiteral("color:#B42318"));
        m_password->selectAll();
        return;
    }
    m_database->logAction(QStringLiteral("用户登录成功"));
    emit loginSucceeded(fullName);
}

