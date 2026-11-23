#include "mainwindow.h"
#include "patientdialog.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QStackedWidget>
#include <QTableView>
#include <QVBoxLayout>

MainWindow::MainWindow(DatabaseManager *database, const QString &fullName, QWidget *parent)
    : QMainWindow(parent), m_database(database) {
    setWindowTitle(QStringLiteral("诊疗信息管理系统"));
    resize(1180, 720);

    auto *root = new QWidget(this);
    auto *layout = new QHBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sidebar = new QWidget(root);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(220);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(24, 28, 24, 24);
    auto *logo = new QLabel(QStringLiteral("Clinic Desk"), sidebar);
    logo->setObjectName(QStringLiteral("sideTitle"));
    auto *user = new QLabel(QStringLiteral("当前用户：") + fullName, sidebar);
    user->setObjectName(QStringLiteral("sideUser"));
    sideLayout->addWidget(logo);
    sideLayout->addWidget(user);
    sideLayout->addSpacing(22);

    auto *stack = new QStackedWidget(root);
    stack->addWidget(buildPatientsPage());
    stack->addWidget(buildSimpleTablePage(QStringLiteral("医生信息"), QStringLiteral("doctors"),
                                          {QStringLiteral("编号"), QStringLiteral("工号"),
                                           QStringLiteral("姓名"), QStringLiteral("科室编号")}));
    stack->addWidget(buildSimpleTablePage(QStringLiteral("科室信息"), QStringLiteral("departments"),
                                          {QStringLiteral("编号"), QStringLiteral("科室名称")}));
    stack->addWidget(buildSimpleTablePage(QStringLiteral("操作记录"), QStringLiteral("histories"),
                                          {QStringLiteral("序号"), QStringLiteral("用户"),
                                           QStringLiteral("操作"), QStringLiteral("时间")}));

    const QStringList navNames = {QStringLiteral("患者管理"), QStringLiteral("医生管理"),
                                  QStringLiteral("科室管理"), QStringLiteral("操作记录")};
    auto *navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);
    for (int i = 0; i < navNames.size(); ++i) {
        auto *button = new QPushButton(navNames.at(i), sidebar);
        button->setCheckable(true);
        button->setObjectName(QStringLiteral("navButton"));
        if (i == 0) button->setChecked(true);
        navGroup->addButton(button, i);
        sideLayout->addWidget(button);
    }
    connect(navGroup, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);
    sideLayout->addStretch();
    auto *developer = new QLabel(QStringLiteral("开发者 李志鑫\n学号 2024414290217"), sidebar);
    developer->setObjectName(QStringLiteral("developerInfo"));
    sideLayout->addWidget(developer);

    layout->addWidget(sidebar);
    layout->addWidget(stack, 1);
    setCentralWidget(root);
    refreshModel();
}

QWidget *MainWindow::buildPatientsPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(34, 30, 34, 28);
    layout->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("患者信息管理"), page);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(QStringLiteral("通过 Model View 绑定 SQLite 数据，支持查询、新增、修改和删除"), page);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    auto *toolbar = new QHBoxLayout;
    m_search = new QLineEdit(page);
    m_search->setPlaceholderText(QStringLiteral("按姓名、身份证号或手机号查询"));
    auto *searchButton = new QPushButton(QStringLiteral("查询"), page);
    auto *clearButton = new QPushButton(QStringLiteral("清空"), page);
    auto *addButton = new QPushButton(QStringLiteral("新增患者"), page);
    addButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editButton = new QPushButton(QStringLiteral("修改"), page);
    auto *deleteButton = new QPushButton(QStringLiteral("删除"), page);
    deleteButton->setObjectName(QStringLiteral("dangerButton"));
    toolbar->addWidget(m_search, 1);
    toolbar->addWidget(searchButton);
    toolbar->addWidget(clearButton);
    toolbar->addSpacing(12);
    toolbar->addWidget(addButton);
    toolbar->addWidget(editButton);
    toolbar->addWidget(deleteButton);

    m_patientModel = new QSqlTableModel(this, m_database->database());
    m_patientModel->setTable(QStringLiteral("patients"));
    m_patientModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    const QStringList headers = {QStringLiteral("ID"), QStringLiteral("身份证号"), QStringLiteral("姓名"),
                                 QStringLiteral("性别"), QStringLiteral("出生日期"), QStringLiteral("身高"),
                                 QStringLiteral("体重"), QStringLiteral("手机号"), QStringLiteral("创建时间")};
    for (int i = 0; i < headers.size(); ++i) m_patientModel->setHeaderData(i, Qt::Horizontal, headers.at(i));
    m_patientModel->select();

    m_patientTable = new QTableView(page);
    m_patientTable->setModel(m_patientModel);
    m_patientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_patientTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_patientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_patientTable->horizontalHeader()->setStretchLastSection(true);
    m_patientTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_patientTable->verticalHeader()->setVisible(false);
    m_patientTable->setAlternatingRowColors(true);

    m_status = new QLabel(page);
    m_status->setObjectName(QStringLiteral("mutedText"));

    connect(searchButton, &QPushButton::clicked, this, &MainWindow::applyFilter);
    connect(m_search, &QLineEdit::returnPressed, this, &MainWindow::applyFilter);
    connect(clearButton, &QPushButton::clicked, this, [this] {
        m_search->clear();
        applyFilter();
    });
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addPatient);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editPatient);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deletePatient);
    connect(m_patientTable, &QTableView::doubleClicked, this, &MainWindow::editPatient);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(toolbar);
    layout->addWidget(m_patientTable, 1);
    layout->addWidget(m_status);
    return page;
}

QWidget *MainWindow::buildSimpleTablePage(const QString &titleText, const QString &tableName,
                                          const QStringList &headers) {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(34, 30, 34, 28);
    auto *title = new QLabel(titleText, page);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *model = new QSqlTableModel(page, m_database->database());
    model->setTable(tableName);
    for (int i = 0; i < headers.size(); ++i) model->setHeaderData(i, Qt::Horizontal, headers.at(i));
    model->select();
    auto *table = new QTableView(page);
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(table, 1);
    return page;
}

void MainWindow::applyFilter() {
    QString keyword = m_search->text().trimmed();
    keyword.replace(QLatin1Char('\''), QStringLiteral("''"));
    if (keyword.isEmpty()) {
        m_patientModel->setFilter(QString());
    } else {
        m_patientModel->setFilter(QStringLiteral("name LIKE '%%1%' OR id_card LIKE '%%1%' OR mobile LIKE '%%1%'")
                                      .arg(keyword));
    }
    refreshModel(m_patientModel->rowCount() == 0
                     ? QStringLiteral("没有找到匹配的患者记录")
                     : QStringLiteral("查询完成"));
}

PatientRecord MainWindow::selectedRecord() const {
    PatientRecord record;
    const QModelIndex current = m_patientTable->currentIndex();
    if (!current.isValid()) return record;
    const QSqlRecord row = m_patientModel->record(current.row());
    record.id = row.value(QStringLiteral("id")).toInt();
    record.idCard = row.value(QStringLiteral("id_card")).toString();
    record.name = row.value(QStringLiteral("name")).toString();
    record.sex = row.value(QStringLiteral("sex")).toString();
    record.dateOfBirth = QDate::fromString(row.value(QStringLiteral("dob")).toString(), Qt::ISODate);
    record.height = row.value(QStringLiteral("height")).toDouble();
    record.weight = row.value(QStringLiteral("weight")).toDouble();
    record.mobile = row.value(QStringLiteral("mobile")).toString();
    return record;
}

void MainWindow::refreshModel(const QString &status) {
    m_patientModel->select();
    const QString text = status.isEmpty()
        ? QStringLiteral("当前显示 %1 条患者记录").arg(m_patientModel->rowCount())
        : status + QStringLiteral("，当前 %1 条").arg(m_patientModel->rowCount());
    m_status->setText(text);
}

void MainWindow::addPatient() {
    PatientDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    QString error;
    if (!m_database->addPatient(dialog.record(), &error)) {
        QMessageBox::warning(this, QStringLiteral("新增失败"), error);
        return;
    }
    refreshModel(QStringLiteral("患者记录已新增"));
}

void MainWindow::editPatient() {
    const PatientRecord current = selectedRecord();
    if (current.id <= 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一条患者记录"));
        return;
    }
    PatientDialog dialog(this);
    dialog.setRecord(current);
    if (dialog.exec() != QDialog::Accepted) return;
    QString error;
    if (!m_database->updatePatient(dialog.record(), &error)) {
        QMessageBox::warning(this, QStringLiteral("修改失败"), error);
        return;
    }
    refreshModel(QStringLiteral("患者记录已修改并刷新"));
}

void MainWindow::deletePatient() {
    const PatientRecord current = selectedRecord();
    if (current.id <= 0) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择一条患者记录"));
        return;
    }
    const auto answer = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确定删除患者“%1”吗？此操作不可撤销。").arg(current.name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    QString error;
    if (!m_database->removePatient(current.id, &error)) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), error);
        return;
    }
    refreshModel(QStringLiteral("患者记录已删除"));
}

