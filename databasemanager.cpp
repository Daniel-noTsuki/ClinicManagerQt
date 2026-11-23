#include "databasemanager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

DatabaseManager::DatabaseManager(const QString &connectionPrefix)
    : m_connectionName(connectionPrefix + QLatin1Char('_') +
                       QUuid::createUuid().toString(QUuid::WithoutBraces)),
      m_database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName)) {}

DatabaseManager::~DatabaseManager() {
    if (m_database.isOpen()) {
        m_database.close();
    }
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DatabaseManager::open(const QString &databasePath, QString *errorMessage) {
    m_database.setDatabaseName(databasePath);
    if (!m_database.open()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("数据库连接失败：") + m_database.lastError().text();
        }
        return false;
    }
    QSqlQuery pragma(m_database);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    return true;
}

bool DatabaseManager::execSql(const QString &sql, QString *errorMessage) const {
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool DatabaseManager::initialize(QString *errorMessage) {
    const QStringList schema = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS users ("
                       "id TEXT PRIMARY KEY, fullname TEXT NOT NULL, username TEXT NOT NULL UNIQUE, "
                       "password_hash TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS departments ("
                       "id TEXT PRIMARY KEY, name TEXT NOT NULL UNIQUE)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS doctors ("
                       "id TEXT PRIMARY KEY, employee_no TEXT NOT NULL UNIQUE, name TEXT NOT NULL, "
                       "department_id TEXT NOT NULL REFERENCES departments(id))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS patients ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, id_card TEXT NOT NULL UNIQUE, "
                       "name TEXT NOT NULL, sex TEXT NOT NULL CHECK(sex IN ('男','女','其他')), "
                       "dob TEXT NOT NULL, height REAL CHECK(height > 0), weight REAL CHECK(weight > 0), "
                       "mobile TEXT NOT NULL, created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS histories ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, user_id TEXT NOT NULL, event TEXT NOT NULL, "
                       "timestamp TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                       "FOREIGN KEY(user_id) REFERENCES users(id))")
    };

    for (const QString &statement : schema) {
        if (!execSql(statement, errorMessage)) {
            return false;
        }
    }

    QSqlQuery userCount(m_database);
    if (!userCount.exec(QStringLiteral("SELECT COUNT(*) FROM users")) || !userCount.next()) {
        if (errorMessage) *errorMessage = userCount.lastError().text();
        return false;
    }
    if (userCount.value(0).toInt() == 0) {
        QSqlQuery user(m_database);
        user.prepare(QStringLiteral("INSERT INTO users(id, fullname, username, password_hash) "
                                    "VALUES('U001', :fullname, 'admin', :hash)"));
        user.bindValue(QStringLiteral(":fullname"), QStringLiteral("李志鑫"));
        user.bindValue(QStringLiteral(":hash"), passwordHash(QStringLiteral("admin123")));
        if (!user.exec()) {
            if (errorMessage) *errorMessage = user.lastError().text();
            return false;
        }
    }

    if (!execSql(QStringLiteral("INSERT OR IGNORE INTO departments(id,name) VALUES"
                                "('D001','门诊部'),('D002','内科'),('D003','外科')"), errorMessage)) {
        return false;
    }
    if (!execSql(QStringLiteral("INSERT OR IGNORE INTO doctors(id,employee_no,name,department_id) VALUES"
                                "('DR001','2026001','陈医生','D001'),"
                                "('DR002','2026002','周医生','D002')"), errorMessage)) {
        return false;
    }

    QSqlQuery patientCount(m_database);
    if (!patientCount.exec(QStringLiteral("SELECT COUNT(*) FROM patients")) || !patientCount.next()) {
        if (errorMessage) *errorMessage = patientCount.lastError().text();
        return false;
    }
    if (patientCount.value(0).toInt() == 0) {
        const QString seed = QStringLiteral(
            "INSERT INTO patients(id_card,name,sex,dob,height,weight,mobile) VALUES"
            "('441900200205180021','梁晓宁','女','2002-05-18',163.0,51.5,'13800138021'),"
            "('441900199912030018','何致远','男','1999-12-03',176.0,68.0,'13800138018'),"
            "('441900200109220035','谢雨桐','女','2001-09-22',160.5,49.0,'13800138035')");
        if (!execSql(seed, errorMessage)) {
            return false;
        }
    }
    return true;
}

QString DatabaseManager::passwordHash(const QString &plainText) {
    return QString::fromLatin1(QCryptographicHash::hash(plainText.toUtf8(),
                                                        QCryptographicHash::Sha256).toHex());
}

bool DatabaseManager::authenticate(const QString &username, const QString &password,
                                   QString *fullName) const {
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT fullname FROM users "
                                 "WHERE username=:username AND password_hash=:password"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());
    query.bindValue(QStringLiteral(":password"), passwordHash(password));
    if (!query.exec() || !query.next()) {
        return false;
    }
    if (fullName) *fullName = query.value(0).toString();
    return true;
}

bool DatabaseManager::validatePatient(const PatientRecord &record, QString *errorMessage) {
    if (record.name.trimmed().isEmpty() || record.idCard.trimmed().isEmpty() ||
        record.mobile.trimmed().isEmpty() || !record.dateOfBirth.isValid()) {
        if (errorMessage) *errorMessage = QStringLiteral("姓名、身份证、出生日期和手机号为必填项");
        return false;
    }
    if (record.height <= 0 || record.weight <= 0) {
        if (errorMessage) *errorMessage = QStringLiteral("身高和体重必须大于 0");
        return false;
    }
    return true;
}

bool DatabaseManager::addPatient(const PatientRecord &record, QString *errorMessage) {
    if (!validatePatient(record, errorMessage)) return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT INTO patients(id_card,name,sex,dob,height,weight,mobile) "
                                 "VALUES(:id_card,:name,:sex,:dob,:height,:weight,:mobile)"));
    query.bindValue(QStringLiteral(":id_card"), record.idCard.trimmed());
    query.bindValue(QStringLiteral(":name"), record.name.trimmed());
    query.bindValue(QStringLiteral(":sex"), record.sex);
    query.bindValue(QStringLiteral(":dob"), record.dateOfBirth.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":height"), record.height);
    query.bindValue(QStringLiteral(":weight"), record.weight);
    query.bindValue(QStringLiteral(":mobile"), record.mobile.trimmed());
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().nativeErrorCode() == QStringLiteral("2067")
                ? QStringLiteral("身份证号已存在，不能重复新增")
                : query.lastError().text();
        }
        return false;
    }
    logAction(QStringLiteral("新增患者：") + record.name);
    return true;
}

bool DatabaseManager::updatePatient(const PatientRecord &record, QString *errorMessage) {
    if (record.id <= 0 || !validatePatient(record, errorMessage)) return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("UPDATE patients SET id_card=:id_card,name=:name,sex=:sex,dob=:dob,"
                                 "height=:height,weight=:weight,mobile=:mobile WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), record.id);
    query.bindValue(QStringLiteral(":id_card"), record.idCard.trimmed());
    query.bindValue(QStringLiteral(":name"), record.name.trimmed());
    query.bindValue(QStringLiteral(":sex"), record.sex);
    query.bindValue(QStringLiteral(":dob"), record.dateOfBirth.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":height"), record.height);
    query.bindValue(QStringLiteral(":weight"), record.weight);
    query.bindValue(QStringLiteral(":mobile"), record.mobile.trimmed());
    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    logAction(QStringLiteral("修改患者：") + record.name);
    return query.numRowsAffected() == 1;
}

bool DatabaseManager::removePatient(int id, QString *errorMessage) {
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM patients WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 1) {
        logAction(QStringLiteral("删除患者记录，ID=%1").arg(id));
        return true;
    }
    if (errorMessage) *errorMessage = QStringLiteral("记录不存在或已被删除");
    return false;
}

bool DatabaseManager::logAction(const QString &event, QString *errorMessage) const {
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT INTO histories(user_id,event,timestamp) "
                                 "VALUES('U001',:event,:timestamp)"));
    query.bindValue(QStringLiteral(":event"), event);
    query.bindValue(QStringLiteral(":timestamp"), QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}

QSqlDatabase DatabaseManager::database() const { return m_database; }
QString DatabaseManager::connectionName() const { return m_connectionName; }

