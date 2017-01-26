#include "storemodel.h"
#include <QDebug>
#include <QMessageBox>
#include <QMimeData>
#include <QStringListModel>

QDataStream &
operator<<(QDataStream &out,
           const dragAndDropInfoPasswordStore &dragAndDropInfoPasswordStore) {
  out << dragAndDropInfoPasswordStore.isDir
      << dragAndDropInfoPasswordStore.isFile
      << dragAndDropInfoPasswordStore.path;
  return out;
}

QDataStream &
operator>>(QDataStream &in,
           dragAndDropInfoPasswordStore &dragAndDropInfoPasswordStore) {
  in >> dragAndDropInfoPasswordStore.isDir >>
      dragAndDropInfoPasswordStore.isFile >> dragAndDropInfoPasswordStore.path;
  return in;
}

/**
 * @brief StoreModel::StoreModel
 * SubClass of QSortFilterProxyModel via
 * http://www.qtcentre.org/threads/46471-QTreeView-Filter
 */
StoreModel::StoreModel() { fs = NULL; }

/**
 * @brief StoreModel::filterAcceptsRow should row be shown, wrapper for
 * StoreModel::ShowThis method.
 * @param sourceRow
 * @param sourceParent
 * @return
 */
bool StoreModel::filterAcceptsRow(int sourceRow,
                                  const QModelIndex &sourceParent) const {
  QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
  return ShowThis(index);
}

/**
 * @brief StoreModel::ShowThis should a row be shown, based on our search
 * criteria.
 * @param index
 * @return
 */
bool StoreModel::ShowThis(const QModelIndex index) const {
  bool retVal = false;
  if (fs == NULL)
    return retVal;
  // Gives you the info for number of childs with a parent
  if (sourceModel()->rowCount(index) > 0) {
    for (int nChild = 0; nChild < sourceModel()->rowCount(index); ++nChild) {
      QModelIndex childIndex = sourceModel()->index(nChild, 0, index);
      if (!childIndex.isValid())
        break;
      retVal = ShowThis(childIndex);
      if (retVal)
        break;
    }
  } else {
    QModelIndex useIndex = sourceModel()->index(index.row(), 0, index.parent());
    QString path = fs->filePath(useIndex);
    path = QDir(store).relativeFilePath(path);
    path.replace(QRegExp("\\.gpg$"), "");
    retVal = path.contains(filterRegExp());
  }
  return retVal;
}

/**
 * @brief StoreModel::setModelAndStore update the source model and store.
 * @param sourceModel
 * @param passStore
 */
void StoreModel::setModelAndStore(QFileSystemModel *sourceModel,
                                  QString passStore) {
  fs = sourceModel;
  store = passStore;
}

/**
 * @brief StoreModel::data don't show the .gpg at the end of a file.
 * @param index
 * @param role
 * @return
 */
QVariant StoreModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  QVariant initial_value;
  initial_value = QSortFilterProxyModel::data(index, role);

  if (role == Qt::DisplayRole) {
    QString name = initial_value.toString();
    name.replace(QRegExp("\\.gpg$"), "");
    initial_value.setValue(name);
  }

  return initial_value;
}

/**
 * @brief StoreModel::supportedDropActions enable drop.
 * @return
 */
Qt::DropActions StoreModel::supportedDropActions() const {
  return Qt::CopyAction | Qt::MoveAction;
}
/**
 * @brief StoreModel::supportedDragActions enable drag.
 * @return
 */
Qt::DropActions StoreModel::supportedDragActions() const {
  return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags StoreModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags defaultFlags = QSortFilterProxyModel::flags(index);

  if (index.isValid()) {
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  } else {
    return Qt::ItemIsDropEnabled | defaultFlags;
  }
}

QStringList StoreModel::mimeTypes() const {
  QStringList types;
  types << "application/vnd+qtpass.dragAndDropInfoPasswordStore";
  return types;
}

QMimeData *StoreModel::mimeData(const QModelIndexList &indexes) const
{
    // only use the first, otherwise we should enable multiselection
    QModelIndex index = indexes.at(0);
    QMimeData *mimeData = new QMimeData();
    if (index.isValid()) {
        QModelIndex useIndex  = mapToSource(index);

        // write date to struct
        dragAndDropInfoPasswordStore info;
        info.isDir = fs->fileInfo(useIndex).isDir();
        info.isFile = fs->fileInfo(useIndex).isFile();
        info.path = fs->fileInfo(useIndex).absoluteFilePath();

        // write stuct into byte array
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::WriteOnly);
        stream << info;
        mimeData->setData("application/vnd+qtpass.dragAndDropInfoPasswordStore", encodedData);
    }

    return mimeData;
}

bool StoreModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column,
                                 const QModelIndex &parent) const {
    if (data->hasFormat("application/vnd+qtpass.dragAndDropInfoPasswordStore") == false){
        return false;
    }


  QModelIndex useIndex =
      this->index(parent.row(), parent.column(), parent.parent());
  QByteArray encodedData =
      data->data("application/vnd+qtpass.dragAndDropInfoPasswordStore");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  dragAndDropInfoPasswordStore info;
  stream >> info;
  if (data->hasFormat("application/vnd+qtpass.dragAndDropInfoPasswordStore") ==
      false)
    return false;

  if (column > 0) {
    return false;
  }

  // you can drop a folder on a folder
  if (fs->fileInfo(mapToSource(useIndex)).isDir() && info.isDir) {
    return true;
  }
  // you can drop a file on a folder
  if (fs->fileInfo(mapToSource(useIndex)).isDir() && info.isFile) {
    return true;
  }
  // you can drop a file on a file
  if (fs->fileInfo(mapToSource(useIndex)).isFile() && info.isFile) {
    return true;
  }

  return false;
}

bool StoreModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                              int row, int column, const QModelIndex &parent) {
  if (!canDropMimeData(data, action, row, column, parent))
    return false;

  if (action == Qt::IgnoreAction) {
    return true;
  }
  QByteArray encodedData =
      data->data("application/vnd+qtpass.dragAndDropInfoPasswordStore");

  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  dragAndDropInfoPasswordStore info;
  stream >> info;
  QModelIndex destIndex =
      this->index(parent.row(), parent.column(), parent.parent());
  QFileInfo destFileInfo = fs->fileInfo(mapToSource(destIndex));
  QFileInfo srcFileInfo = QFileInfo(info.path);
    QString srcAbs = srcFileInfo.absoluteFilePath();
    QString destAbs = destFileInfo.absoluteFilePath();
    if(info.isDir){
        QDir srcDir = QDir(info.path);
        // dropped dir onto dir
        if(destFileInfo.isDir()){
            QDir destDir = QDir(destAbs).filePath(srcFileInfo.fileName());
            if(action == Qt::MoveAction){
                QtPassSettings::getPass()->Move(srcAbs, destDir.absolutePath());
            }else if(action == Qt::CopyAction){
                QtPassSettings::getPass()->Copy(srcAbs, destDir.absolutePath());
            }
        }
    }else if(info.isFile){
        // dropped file onto a directory
        if(destFileInfo.isDir()){
            if(action == Qt::MoveAction){
                QtPassSettings::getPass()->Move(srcAbs, destAbs);
            }else if(action == Qt::CopyAction){
                QtPassSettings::getPass()->Copy(srcAbs, destAbs);
            }
        }else if(destFileInfo.isFile()){
            // dropped file onto a file
            int answer = QMessageBox::question(0, tr("force overwrite?"), tr("overwrite %1 with %2?").arg(destAbs).arg(srcAbs), QMessageBox::Yes | QMessageBox::No);
            bool force = answer==QMessageBox::Yes;
            if(action == Qt::MoveAction){
                QtPassSettings::getPass()->Move(srcAbs, destAbs, force);
            }else if(action == Qt::CopyAction){
                QtPassSettings::getPass()->Copy(srcAbs, destAbs, force);
            }
        }

    }

  return true;
}
