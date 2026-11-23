#include "databasemanager.h"
#include "loginwidget.h"
#include "mainwindow.h"
#include "patientdialog.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTextStream>

static void applyStyle(QApplication &app) {
    app.setStyle(QStringLiteral("Fusion"));
    app.setFont(QFont(QStringLiteral("PingFang SC"), 12));
    app.setStyleSheet(QStringLiteral(R"(
        QWidget { color:#1D2939; background:#F7F9FC; }
        QLabel { background:transparent; }
        QFrame#loginCard { background:white; border:1px solid #E4E7EC; border-radius:16px; }
        QLabel#brandTitle, QLabel#pageTitle { font-size:26px; font-weight:700; color:#101828; }
        QLabel#brandSubtitle { font-size:15px; color:#175CD3; font-weight:600; }
        QLabel#pageSubtitle, QLabel#mutedText { color:#667085; }
        QLabel#dialogTitle { font-size:20px; font-weight:700; margin-bottom:8px; }
        QWidget#sidebar { background:#163A5F; color:white; }
        QLabel#sideTitle { color:white; font-size:23px; font-weight:700; }
        QLabel#sideUser, QLabel#developerInfo { color:#D7E8F7; line-height:1.4; }
        QPushButton#navButton { color:#EAF2F8; background:transparent; text-align:left;
            padding:12px 14px; border:none; border-radius:8px; }
        QPushButton#navButton:hover, QPushButton#navButton:checked { background:#245B8C; color:white; }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox { background:white; border:1px solid #D0D5DD;
            border-radius:7px; padding:8px 10px; min-height:20px; }
        QPushButton { background:white; border:1px solid #D0D5DD; border-radius:7px; padding:8px 14px; }
        QPushButton:hover { background:#F2F4F7; }
        QPushButton#primaryButton { background:#175CD3; color:white; border-color:#175CD3; }
        QPushButton#dangerButton { color:#B42318; border-color:#FDA29B; }
        QTableView { background:white; alternate-background-color:#F8FAFC; border:1px solid #E4E7EC;
            border-radius:8px; gridline-color:#EAECF0; selection-background-color:#D1E9FF;
            selection-color:#101828; }
        QHeaderView::section { background:#EEF4FA; color:#344054; font-weight:600; border:none;
            border-bottom:1px solid #D0D5DD; padding:9px; }
    )"));
}

static int runSelfTest(const QString &databasePath) {
    QTextStream out(stdout);
    QFile::remove(databasePath);
    QDir(databasePath + QStringLiteral(".missing")).removeRecursively();

    DatabaseManager invalid(QStringLiteral("invalid"));
    QString error;
    const bool connectionFailureDetected = !invalid.open(databasePath + QStringLiteral(".missing/clinic.db"), &error);
    out << "[PASS] 数据库连接失败提示: " << (connectionFailureDetected ? "已检测" : "未检测") << '\n';
    if (!connectionFailureDetected) return 1;

    DatabaseManager db(QStringLiteral("selftest"));
    if (!db.open(databasePath, &error) || !db.initialize(&error)) {
        out << "[FAIL] 初始化数据库: " << error << '\n';
        return 2;
    }
    out << "[PASS] 数据库初始化\n";

    if (db.authenticate(QStringLiteral("admin"), QStringLiteral("wrong"))) return 3;
    out << "[PASS] 错误凭据被拒绝\n";
    if (!db.authenticate(QStringLiteral("admin"), QStringLiteral("admin123"))) return 4;
    out << "[PASS] 正确凭据登录成功\n";

    PatientRecord missing;
    missing.idCard = QStringLiteral("TEST-MISSING");
    missing.sex = QStringLiteral("女");
    missing.dateOfBirth = QDate(2001, 1, 1);
    missing.height = 160;
    missing.weight = 50;
    missing.mobile = QStringLiteral("13900000000");
    if (db.addPatient(missing, &error)) return 5;
    out << "[PASS] 缺失姓名被拦截: " << error << '\n';

    PatientRecord record;
    record.idCard = QStringLiteral("TEST-2026-001");
    record.name = QStringLiteral("测试患者甲");
    record.sex = QStringLiteral("女");
    record.dateOfBirth = QDate(2001, 6, 12);
    record.height = 162.5;
    record.weight = 52;
    record.mobile = QStringLiteral("13900000001");
    if (!db.addPatient(record, &error)) return 6;
    out << "[PASS] 新增记录成功\n";
    if (db.addPatient(record, &error)) return 7;
    out << "[PASS] 重复身份证被拒绝: " << error << '\n';

    QSqlQuery query(db.database());
    query.prepare(QStringLiteral("SELECT id FROM patients WHERE id_card=:id_card"));
    query.bindValue(QStringLiteral(":id_card"), record.idCard);
    if (!query.exec() || !query.next()) return 8;
    record.id = query.value(0).toInt();
    record.mobile = QStringLiteral("13900000999");
    if (!db.updatePatient(record, &error)) return 9;
    QSqlQuery verify(db.database());
    verify.prepare(QStringLiteral("SELECT mobile FROM patients WHERE id=:id"));
    verify.bindValue(QStringLiteral(":id"), record.id);
    if (!verify.exec() || !verify.next() || verify.value(0).toString() != record.mobile) return 10;
    out << "[PASS] 修改后数据库值与模型数据一致\n";

    QSqlQuery empty(db.database());
    if (!empty.exec(QStringLiteral("SELECT COUNT(*) FROM patients WHERE name='不存在的患者名'")) ||
        !empty.next() || empty.value(0).toInt() != 0) return 11;
    out << "[PASS] 无结果查询返回 0 行\n";

    if (!db.removePatient(record.id, &error)) return 12;
    QSqlQuery removed(db.database());
    removed.prepare(QStringLiteral("SELECT COUNT(*) FROM patients WHERE id=:id"));
    removed.bindValue(QStringLiteral(":id"), record.id);
    if (!removed.exec() || !removed.next() || removed.value(0).toInt() != 0) return 13;
    out << "[PASS] 删除后记录不存在\n";
    out << "SELF_TEST_RESULT=PASS\n";
    return 0;
}

static int captureScreens(DatabaseManager &database, const QString &outputDirectory) {
    QDir().mkpath(outputDirectory);
    LoginWidget login(&database);
    login.setCredentials(QStringLiteral("admin"), QStringLiteral("admin123"));
    login.show();
    QApplication::processEvents();
    if (!login.grab().save(outputDirectory + QStringLiteral("/01-login.png"))) return 21;
    login.setCredentials(QStringLiteral("admin"), QStringLiteral("wrong-password"));
    QMetaObject::invokeMethod(&login, "attemptLogin");
    QApplication::processEvents();
    if (!login.grab().save(outputDirectory + QStringLiteral("/02-login-rejected.png"))) return 22;
    login.hide();

    MainWindow main(&database, QStringLiteral("李志鑫"));
    main.show();
    QApplication::processEvents();
    if (!main.grab().save(outputDirectory + QStringLiteral("/03-patient-management.png"))) return 23;
    main.setSearchKeyword(QStringLiteral("不存在的患者名"));
    QApplication::processEvents();
    if (!main.grab().save(outputDirectory + QStringLiteral("/04-empty-query.png"))) return 24;
    main.hide();

    PatientDialog dialog;
    PatientRecord sample;
    sample.idCard = QStringLiteral("441900200205180021");
    sample.name = QStringLiteral("梁晓宁");
    sample.sex = QStringLiteral("女");
    sample.dateOfBirth = QDate(2002, 5, 18);
    sample.height = 163;
    sample.weight = 51.5;
    sample.mobile = QStringLiteral("13800138021");
    dialog.setRecord(sample);
    dialog.show();
    QApplication::processEvents();
    if (!dialog.grab().save(outputDirectory + QStringLiteral("/05-patient-editor.png"))) return 25;
    return 0;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    applyStyle(app);
    const QStringList arguments = app.arguments();

    if (arguments.contains(QStringLiteral("--self-test"))) {
        const int i = arguments.indexOf(QStringLiteral("--self-test"));
        const QString path = i + 1 < arguments.size() ? arguments.at(i + 1)
                                                       : QDir::tempPath() + QStringLiteral("/clinic-selftest.db");
        return runSelfTest(path);
    }

    const QString dataDirectory = arguments.contains(QStringLiteral("--capture"))
        ? QDir::tempPath() + QStringLiteral("/ClinicManagerCapture")
        : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDirectory);
    DatabaseManager database;
    QString error;
    if (!database.open(dataDirectory + QStringLiteral("/clinic.db"), &error) ||
        !database.initialize(&error)) {
        QTextStream(stderr) << error << '\n';
        return 1;
    }

    if (arguments.contains(QStringLiteral("--capture"))) {
        const int i = arguments.indexOf(QStringLiteral("--capture"));
        if (i + 1 >= arguments.size()) return 20;
        return captureScreens(database, arguments.at(i + 1));
    }

    auto *login = new LoginWidget(&database);
    MainWindow *mainWindow = nullptr;
    QObject::connect(login, &LoginWidget::loginSucceeded, &app, [&](const QString &fullName) {
        mainWindow = new MainWindow(&database, fullName);
        mainWindow->show();
        login->close();
    });
    login->show();
    return app.exec();
}
