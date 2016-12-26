#include "renamedialog.h"
#include "ui_renamedialog.h"
#include "debughelper.h"
#include "qtpasssettings.h"
#include "ui_mainwindow.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QTimer>




RenameDialog::RenameDialog(QFileInfo selectedFile, MainWindow *parent) :
    mainWindow(parent),
    selectedFile(selectedFile),
    QDialog(parent),
    ui(new Ui::RenameDialog)
{
    ui->setupUi(this);
    ui->currentNameLabel->setText(selectedFile.absoluteFilePath());
    QTimer::singleShot(0, ui->newNameLineEdit, SLOT(setFocus()));

}

RenameDialog::~RenameDialog()
{
    delete ui;
}

void RenameDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton clickedStandardButton = ui->buttonBox->standardButton(button);
    if(clickedStandardButton == QDialogButtonBox::Ok){
        QString lineEditText=ui->newNameLineEdit->text();
        QString newFilename = selectedFile.dir().filePath(lineEditText);

        // prevent files with ../ or ../../usr/bin
        if(isInSameDir(QFileInfo(newFilename))){
            Qt::CheckState checkState = ui->forceCheckbox->checkState();
            QtPassSettings::getPass()->Move(selectedFile.absoluteFilePath(), newFilename, checkState == Qt::Checked);
        }else{
            showStatusMessageOnMainWindow(tr("%1 not renamed, because its not in the same dir").arg(selectedFile.absoluteFilePath()), 5000);
        }
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
    ui->errorLabel->setStyleSheet("");

    if(newValue == ""){
        // lineedit is empty
        return;
    }
    QString newFilename = selectedFile.dir().filePath(newValue);

    QFileInfo newFileInfo(newFilename);
    if(newFileInfo.exists() ){
        ui->errorLabel->setText(tr("a file/directory with the given name already exists"));
        ui->errorLabel->setStyleSheet("QLabel {color : red;}");
    }

    // prevent files with ../ or ../../usr/bin
    if(isNotInSameDir(newFileInfo)){
        ui->errorLabel->setText(tr("you can not move accross folders"));
        ui->errorLabel->setStyleSheet("QLabel {color : red;}");
    }
}

bool RenameDialog::isInSameDir(const QFileInfo newFileInfo){
    // prevent files with ../ or ../../usr/bin
    QString relativePathBeetween = QDir(selectedFile.dir().absolutePath()).relativeFilePath(newFileInfo.dir().absolutePath());
    return relativePathBeetween == ".";
}
bool RenameDialog::isNotInSameDir(const QFileInfo newFileInfo){
    return isInSameDir(newFileInfo) == false;
}
