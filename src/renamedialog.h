#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include "mainwindow.h"
#include <QDialog>
#include <QFileInfo>
#include <QAbstractButton>

namespace Ui {
class RenameDialog;
}

class RenameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameDialog(QFileInfo selectedFile, MainWindow *mainWindow);
    ~RenameDialog();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_newNameLineEdit_textChanged(const QString &newValue);

signals:
    void showStatusMessageOnMainWindow(QString, int);

private:
    MainWindow *mainWindow;
    Ui::RenameDialog *ui;
    QFileInfo selectedFile;
    bool isInSameDir(const QFileInfo newFileInfo);
    bool isNotInSameDir(const QFileInfo newFileInfo);
};

#endif // RENAMEDIALOG_H
