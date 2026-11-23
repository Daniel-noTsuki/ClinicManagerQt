#pragma once

#include "databasemanager.h"

#include <QDialog>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;

class PatientDialog : public QDialog {
    Q_OBJECT
public:
    explicit PatientDialog(QWidget *parent = nullptr);
    void setRecord(const PatientRecord &record);
    PatientRecord record() const;

private:
    int m_id = 0;
    QLineEdit *m_idCard;
    QLineEdit *m_name;
    QComboBox *m_sex;
    QDateEdit *m_dob;
    QDoubleSpinBox *m_height;
    QDoubleSpinBox *m_weight;
    QLineEdit *m_mobile;
};

