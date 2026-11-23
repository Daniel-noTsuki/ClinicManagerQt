#pragma once

#include "databasemanager.h"

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QSqlTableModel;
class QTableView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DatabaseManager *database, const QString &fullName,
                        QWidget *parent = nullptr);
    void setSearchKeyword(const QString &keyword);

private slots:
    void applyFilter();
    void addPatient();
    void editPatient();
    void deletePatient();

private:
    PatientRecord selectedRecord() const;
    void refreshModel(const QString &status = QString());
    QWidget *buildPatientsPage();
    QWidget *buildSimpleTablePage(const QString &title, const QString &tableName,
                                  const QStringList &headers);

    DatabaseManager *m_database;
    QSqlTableModel *m_patientModel = nullptr;
    QTableView *m_patientTable = nullptr;
    QLineEdit *m_search = nullptr;
    QLabel *m_status = nullptr;
};
