#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QString>

struct PatientRecord {
    int id = 0;
    QString idCard;
    QString name;
    QString sex;
    QDate dateOfBirth;
    double height = 0.0;
    double weight = 0.0;
    QString mobile;
};

class DatabaseManager {
public:
    explicit DatabaseManager(const QString &connectionPrefix = QStringLiteral("clinic"));
    ~DatabaseManager();

    bool open(const QString &databasePath, QString *errorMessage = nullptr);
    bool initialize(QString *errorMessage = nullptr);
    bool authenticate(const QString &username, const QString &password,
                      QString *fullName = nullptr) const;
    bool addPatient(const PatientRecord &record, QString *errorMessage = nullptr);
    bool updatePatient(const PatientRecord &record, QString *errorMessage = nullptr);
    bool removePatient(int id, QString *errorMessage = nullptr);
    bool logAction(const QString &event, QString *errorMessage = nullptr) const;

    QSqlDatabase database() const;
    QString connectionName() const;

private:
    static QString passwordHash(const QString &plainText);
    static bool validatePatient(const PatientRecord &record, QString *errorMessage);
    bool execSql(const QString &sql, QString *errorMessage = nullptr) const;

    QString m_connectionName;
    QSqlDatabase m_database;
};

