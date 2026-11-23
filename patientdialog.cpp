#include "patientdialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

PatientDialog::PatientDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("患者信息"));
    setMinimumWidth(440);

    m_idCard = new QLineEdit(this);
    m_name = new QLineEdit(this);
    m_sex = new QComboBox(this);
    m_sex->addItems({QStringLiteral("男"), QStringLiteral("女"), QStringLiteral("其他")});
    m_dob = new QDateEdit(QDate(2000, 1, 1), this);
    m_dob->setCalendarPopup(true);
    m_dob->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_height = new QDoubleSpinBox(this);
    m_height->setRange(30, 250);
    m_height->setSuffix(QStringLiteral(" cm"));
    m_height->setValue(165);
    m_weight = new QDoubleSpinBox(this);
    m_weight->setRange(2, 300);
    m_weight->setSuffix(QStringLiteral(" kg"));
    m_weight->setValue(55);
    m_mobile = new QLineEdit(this);

    auto *title = new QLabel(QStringLiteral("编辑患者信息"), this);
    title->setObjectName(QStringLiteral("dialogTitle"));
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->addRow(QStringLiteral("身份证号 *"), m_idCard);
    form->addRow(QStringLiteral("姓名 *"), m_name);
    form->addRow(QStringLiteral("性别"), m_sex);
    form->addRow(QStringLiteral("出生日期 *"), m_dob);
    form->addRow(QStringLiteral("身高"), m_height);
    form->addRow(QStringLiteral("体重"), m_weight);
    form->addRow(QStringLiteral("手机号 *"), m_mobile);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void PatientDialog::setRecord(const PatientRecord &record) {
    m_id = record.id;
    m_idCard->setText(record.idCard);
    m_name->setText(record.name);
    m_sex->setCurrentText(record.sex);
    m_dob->setDate(record.dateOfBirth);
    m_height->setValue(record.height);
    m_weight->setValue(record.weight);
    m_mobile->setText(record.mobile);
}

PatientRecord PatientDialog::record() const {
    PatientRecord result;
    result.id = m_id;
    result.idCard = m_idCard->text();
    result.name = m_name->text();
    result.sex = m_sex->currentText();
    result.dateOfBirth = m_dob->date();
    result.height = m_height->value();
    result.weight = m_weight->value();
    result.mobile = m_mobile->text();
    return result;
}

