#include "renamedialog.h"
#include "ui_renamedialog.h"
#include "debughelper.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>

RenameDialog::RenameDialog(QFileInfo selectedFile, QWidget *parent) :
    selectedFile(selectedFile),
    QDialog(parent),
    ui(new Ui::RenameDialog)
{
    ui->setupUi(this);
    ui->currentNameLabel->setText(selectedFile.absoluteFilePath());
}

RenameDialog::~RenameDialog()
{
    delete ui;
}

void RenameDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton clickedStandardButton = ui->buttonBox->standardButton(button);
    if(clickedStandardButton == QDialogButtonBox::Ok){
        dbg() << " ok button pressed";
    }else if(clickedStandardButton == QDialogButtonBox::Cancel){
        dbg() << " cancel button pressed";
    }else{
        dbg() << " pressed a button, which is not handled see QDialogButtonBox::StandardButton .octalvalue:" << QString::number(clickedStandardButton, 16);
    }
}

void RenameDialog::on_newNameLineEdit_textChanged(const QString &newValue)
{
    // reset label
    ui->errorLabel->setText("");
    ui->errorLabel->setStyleSheet("QLabel {color : black;}");

    if(newValue == ""){
        // lineedit is empty
        return;
    }
    QString newFilename = selectedFile.dir().filePath(newValue);
    if( QFileInfo(newFilename).exists()){
        ui->errorLabel->setText(tr("a file with this name already exists"));
        ui->errorLabel->setStyleSheet("QLabel {color : red;}");
    }
}
