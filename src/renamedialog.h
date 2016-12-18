#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

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
    explicit RenameDialog(QFileInfo selectedFile, QWidget *parent = 0);
    ~RenameDialog();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_newNameLineEdit_textChanged(const QString &newValue);

private:
    QFileInfo selectedFile;
    Ui::RenameDialog *ui;
};

#endif // RENAMEDIALOG_H
