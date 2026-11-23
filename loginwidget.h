#pragma once

#include <QWidget>

class DatabaseManager;
class QLabel;
class QLineEdit;

class LoginWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoginWidget(DatabaseManager *database, QWidget *parent = nullptr);
    void setCredentials(const QString &username, const QString &password);

signals:
    void loginSucceeded(const QString &fullName);

private slots:
    void attemptLogin();

private:
    DatabaseManager *m_database;
    QLineEdit *m_username;
    QLineEdit *m_password;
    QLabel *m_message;
};

