#include "imitatepass.h"
#include "debughelper.h"
#include "qtpasssettings.h"
#include "util.h"
#include <QDirIterator>

/**
 * @brief ImitatePass::ImitatePass for situaions when pass is not available
 * we imitate the behavior of pass https://www.passwordstore.org/
 */
ImitatePass::ImitatePass() {}


/**
 * @brief ImitatePass::GitInit git init wrapper
 */
void ImitatePass::GitInit() {
  executeGit(GIT_INIT, {"init", QtPassSettings::getPassStore()});
}

/**
 * @brief ImitatePass::GitPull git init wrapper
 */
void ImitatePass::GitPull() {
  executeGit(GIT_PULL,{"pull"});
}

/**
 * @brief ImitatePass::GitPull_b git pull wrapper
 */
void ImitatePass::GitPull_b() {
  exec.executeBlocking(QtPassSettings::getGitExecutable(), {"pull"});
}

/**
 * @brief ImitatePass::GitPush git init wrapper
 */
void ImitatePass::GitPush() {
  if(QtPassSettings::isUseGit()){
      executeGit(GIT_PUSH, {"push"});
  }
}

/**
 * @brief ImitatePass::Show shows content of file
 */

void ImitatePass::Show(QString file) {
  file = QtPassSettings::getPassStore() + file + ".gpg";
  QStringList args = {"-d",      "--quiet",     "--yes", "--no-encrypt-to",
                      "--batch", "--use-agent", file};
  executeGpg(PASS_SHOW, args);
}

/**
 * @brief ImitatePass::Show_b show content of file, blocking version
 *
 * @returns process exitCode
 */
int ImitatePass::Show_b(QString file) {
  file = QtPassSettings::getPassStore() + file + ".gpg";
  QStringList args = {"-d",      "--quiet",     "--yes", "--no-encrypt-to",
                      "--batch", "--use-agent", file};
  return exec.executeBlocking(QtPassSettings::getGpgExecutable(), args);
}

/**
 * @brief ImitatePass::Insert create new file with encrypted content
 *
 * @param file      file to be created
 * @param newValue  value to be stored in file
 * @param overwrite whether to overwrite existing file
 */
void ImitatePass::Insert(QString file, QString newValue, bool overwrite) {
  file = file + ".gpg";
  QStringList recipients = Pass::getRecipientList(file);
  if (recipients.isEmpty()) {
    //  TODO(bezet): probably throw here
    emit critical(tr("Can not edit"),
                  tr("Could not read encryption key to use, .gpg-id "
                     "file missing or invalid."));
    return;
  }
  QStringList args = {"--batch", "-eq", "--output", file};
  for (auto &r : recipients) {
    args.append("-r");
    args.append(r);
  };
  if (overwrite)
    args.append("--yes");
  args.append("-");
  executeGpg(PASS_INSERT, args,
                 newValue);
  if (!QtPassSettings::isUseWebDav() && QtPassSettings::isUseGit()) {
    if (!overwrite)
      executeGit(GIT_ADD, {"add", file});
    QString path = QDir(QtPassSettings::getPassStore()).relativeFilePath(file);
    path.replace(QRegExp("\\.gpg$"), "");
    QString msg = QString(overwrite ? "Edit" : "\"Add") + " for " + path +
                  " using QtPass.";
    GitCommit(file, msg);
  }
}

/**
 * @brief ImitatePass::GitCommit commit a file to git with an appropriate commit
 * message
 * @param file
 * @param msg
 */
void ImitatePass::GitCommit(const QString &file, const QString &msg) {
  executeGit(GIT_COMMIT, {"commit", "-m", msg, "--", file});
}

/**
 * @brief ImitatePass::Remove git init wrapper
 */
void ImitatePass::Remove(QString file, bool isDir) {
  file = QtPassSettings::getPassStore() + file;
  if (!isDir)
    file += ".gpg";
  if (QtPassSettings::isUseGit()) {
    executeGit(GIT_RM, {"rm", (isDir ? "-rf" : "-f"), file});
    //  TODO(bezet): commit message used to have pass-like file name inside(ie.
    //  getFile(file, true)
    GitCommit(file, "Remove for " + file + " using QtPass.");
  } else {
    if (isDir) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
      QDir dir(file);
      dir.removeRecursively();
#else
      removeDir(QtPassSettings::getPassStore() + file);
#endif
    } else
      QFile(file).remove();
  }
}

/**
 * @brief ImitatePass::Init initialize pass repository
 *
 * @param path      path in which new password-store will be created
 * @param users     list of users who shall be able to decrypt passwords in
 * path
 */
void ImitatePass::Init(QString path, const QList<UserInfo> &users) {
  QString gpgIdFile = path + ".gpg-id";
  QFile gpgId(gpgIdFile);
  bool addFile = false;
  if (QtPassSettings::isAddGPGId(true)) {
    QFileInfo checkFile(gpgIdFile);
    if (!checkFile.exists() || !checkFile.isFile())
      addFile = true;
  }
  if (!gpgId.open(QIODevice::WriteOnly | QIODevice::Text)) {
    emit critical(tr("Cannot update"),
                  tr("Failed to open .gpg-id for writing."));
    return;
  }
  bool secret_selected = false;
  foreach (const UserInfo &user, users) {
    if (user.enabled) {
      gpgId.write((user.key_id + "\n").toUtf8());
      secret_selected |= user.have_secret;
    }
  }
  gpgId.close();
  if (!secret_selected) {
    emit critical(
        tr("Check selected users!"),
        tr("None of the selected keys have a secret key available.\n"
           "You will not be able to decrypt any newly added passwords!"));
    return;
  }

  if (!QtPassSettings::isUseWebDav() && QtPassSettings::isUseGit() &&
      !QtPassSettings::getGitExecutable().isEmpty()) {
    if (addFile)
      executeGit(GIT_ADD, {"add", gpgIdFile});
    QString path = gpgIdFile;
    path.replace(QRegExp("\\.gpg$"), "");
    GitCommit(gpgIdFile, "Added " + path + " using QtPass.");
  }
  reencryptPath(path);
}

/**
 * @brief ImitatePass::removeDir delete folder recursive.
 * @param dirName which folder.
 * @return was removal succesful?
 */
bool ImitatePass::removeDir(const QString &dirName) {
  bool result = true;
  QDir dir(dirName);

  if (dir.exists(dirName)) {
    Q_FOREACH (QFileInfo info,
               dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System |
                                     QDir::Hidden | QDir::AllDirs | QDir::Files,
                                 QDir::DirsFirst)) {
      if (info.isDir())
        result = removeDir(info.absoluteFilePath());
      else
        result = QFile::remove(info.absoluteFilePath());

      if (!result)
        return result;
    }
    result = dir.rmdir(dirName);
  }
  return result;
}

/**
 * @brief ImitatePass::reencryptPath reencrypt all files under the chosen
 * directory
 *
 * This is stil quite experimental..
 * @param dir
 */
void ImitatePass::reencryptPath(QString dir) {
  emit statusMsg(tr("Re-encrypting from folder %1").arg(dir), 3000);
  emit startReencryptPath();
  if (QtPassSettings::isAutoPull()) {
    //  TODO(bezet): move statuses inside actions?
    emit statusMsg(tr("Updating password-store"), 2000);
    GitPull_b();
  }
  QDir currentDir;
  QDirIterator gpgFiles(dir, QStringList() << "*.gpg", QDir::Files,
                        QDirIterator::Subdirectories);
  QStringList gpgId;
  while (gpgFiles.hasNext()) {
    QString fileName = gpgFiles.next();
    if (gpgFiles.fileInfo().path() != currentDir.path()) {
      gpgId = getRecipientList(fileName);
      gpgId.sort();
    }
    QStringList gpgKeys;
    foreach(QString key, gpgId){
       QList<UserInfo> userInfos = listKeys(key);
       foreach (UserInfo userInfo, userInfos) {
           gpgKeys.append(userInfo.subkey_ids);
       }
    }
    gpgKeys.sort();

    //  TODO(bezet): enable --with-colons for better future-proofness?
    QStringList args = {
        "-v",          "--no-secmem-warning", "--no-permission-warning",
        "--list-only", "--keyid-format=long", fileName};
    QString keys, err;
    exec.executeBlocking(QtPassSettings::getGpgExecutable(), args, &keys, &err);
    QStringList actualKeys;
    keys += err;
    QStringList key = keys.split("\n");
    QListIterator<QString> itr(key);
    while (itr.hasNext()) {
      QString current = itr.next();
      QStringList cur = current.split(" ");
      if (cur.length() > 4) {
        QString actualKey = cur.takeAt(4);
        if (actualKey.length() == 16) {
          actualKeys << actualKey;
        }
      }
    }
    actualKeys.sort();
    if (actualKeys != gpgKeys) {
      // dbg()<< actualKeys << gpgId << getRecipientList(fileName);
      dbg() << "reencrypt " << fileName << " for " << gpgId;
      QString local_lastDecrypt = "Could not decrypt";
      emit lastDecrypt(local_lastDecrypt);
      args = QStringList{"-d",      "--quiet",     "--yes", "--no-encrypt-to",
                         "--batch", "--use-agent", fileName};
      exec.executeBlocking(QtPassSettings::getGpgExecutable(), args,
                           &local_lastDecrypt);
      emit lastDecrypt(local_lastDecrypt);

      if (!local_lastDecrypt.isEmpty() &&
          local_lastDecrypt != "Could not decrypt") {
        if (local_lastDecrypt.right(1) != "\n")
          local_lastDecrypt += "\n";

        emit lastDecrypt(local_lastDecrypt);
        QStringList recipients = Pass::getRecipientList(fileName);
        if (recipients.isEmpty()) {
          emit critical(tr("Can not edit"),
                        tr("Could not read encryption key to use, .gpg-id "
                           "file missing or invalid."));
          return;
        }
        args = QStringList{"--yes", "--batch", "-eq", "--output", fileName};
        for (auto &i : recipients) {
          args.append("-r");
          args.append(i);
        }
        args.append("-");
        exec.executeBlocking(QtPassSettings::getGpgExecutable(), args,
                             local_lastDecrypt);

        if (!QtPassSettings::isUseWebDav() && QtPassSettings::isUseGit()) {
          exec.executeBlocking(QtPassSettings::getGitExecutable(),
                               {"add", fileName});
          QString path =
              QDir(QtPassSettings::getPassStore()).relativeFilePath(fileName);
          path.replace(QRegExp("\\.gpg$"), "");
          exec.executeBlocking(QtPassSettings::getGitExecutable(),
                               {"commit", fileName, "-m",
                                "Edit for " + path + " using QtPass."});
        }

      } else {
        dbg() << "Decrypt error on re-encrypt";
      }
    }
  }
  if (QtPassSettings::isAutoPush()) {
    emit statusMsg(tr("Updating password-store"), 2000);
    GitPush();
  }
  emit endReencryptPath();
}

void ImitatePass::Move(const QString srcParam, const QString destParam, const bool force)
{
    copyMove(srcParam, destParam, force, true);
}


void ImitatePass::Copy(const QString srcParam, const QString destParam, const bool force)
{
    copyMove(srcParam, destParam, force, false);
}


void ImitatePass::copyMove(const QString srcParam, const QString destParam, const bool force ,const bool move){
    QString src(QDir(QtPassSettings::getPassStore()).relativeFilePath(srcParam));
    QString dest(QDir(QtPassSettings::getPassStore()).relativeFilePath(destParam));
    // replace .gpg, becaus pass doesnt use this as well
    src.replace(QRegExp("\\.gpg$"), "");
    dest.replace(QRegExp("\\.gpg$"), "");

    // from now on this is like the pass shell script
    if(src.trimmed().isEmpty()){
        statusMsg(tr("You passed an empty source") , 5000);
        return;
    }
    if(dest.trimmed().isEmpty()){
        statusMsg(tr("You passed an empty destination") , 5000);
        return;
    }
    if (hasSneakyPaths({src, dest})){
        statusMsg(tr("You've attempted to pass a sneaky path.\"%1\" or \"%2\"").arg(src).arg(dest) , 5000);
        return;
    }
    QFileInfo old_path(QtPassSettings::getPassStore() + Util::removeOneTrailingSlash(src));
    QFileInfo old_dir(old_path);
    QFileInfo new_path(QtPassSettings::getPassStore() + dest);

    QFileInfo old_pathGpg(old_path.absoluteFilePath() + ".gpg");
    if(((old_pathGpg.isFile() && old_path.isDir()) || srcParam.endsWith(QDir::separator()) || old_pathGpg.isFile() == false) == false){
        old_dir = QFileInfo(old_path.dir().absolutePath());
        old_path = old_pathGpg;
    }
    if(old_path.exists()==false){
        statusMsg(tr("\"%1\" is not in the password store").arg(srcParam) , 5000);
        return;
    }
    // just created a QDir to call mkpath
    QDir qDir;
    qDir.mkpath(new_path.dir().absolutePath());
    if((old_path.isDir() || new_path.isDir() || new_path.absoluteFilePath().endsWith(QDir::separator())) == false){
        new_path = new_path.absoluteFilePath() + ".gpg";
    }

    QString destCopy = new_path.absoluteFilePath();
    // moving/copying a file into a folder
    if(old_path.isFile() && new_path.isDir()){
        destCopy = new_path.absoluteFilePath() + QDir::separator() + old_path.fileName();
    }
    if(force){
        qDir.remove(destCopy);
    }
    if(move){
        bool success = qDir.rename(old_path.absoluteFilePath(), destCopy);
        if(success == false){
            statusMsg(tr("the moving wasnt successfull"), 5000);
        }
        if(QFileInfo(destCopy).exists()){
            // reecrypt all files under the new folder
            reencryptPath(new_path.absoluteFilePath());
        }
        old_path.refresh();
        if (QtPassSettings::isUseGit() && QFileInfo(old_path).exists() == false) {
            QStringList argsRm;
            argsRm << "rm";
            argsRm << "-qr";
            argsRm << old_path.absoluteFilePath();
            executeGit(GIT_RM, argsRm);

            QStringList argsAdd;
            argsAdd << "add";
            argsAdd << destCopy;
            executeGit(GIT_ADD, argsAdd);

            QString message=QString("moved from %1 to %2 using QTPass.");
            message= message.arg(old_path.absoluteFilePath()).arg(destCopy);
            GitCommit(destCopy, message);
            if(QtPassSettings::isAutoPush()){
              GitPush();
            }
        }
    }else{
        bool success = QFile::copy(old_path.absoluteFilePath(), destCopy);
        if(success == false){
            statusMsg(tr("the copying wasnt successfull"), 5000);
        }
        if(QFileInfo(destCopy).exists()){
            // reecrypt all files under the new folder
            reencryptPath(new_path.absoluteFilePath());
        }
        old_path.refresh();
        if (QtPassSettings::isUseGit() && old_path.exists() == false) {
            QStringList argsAdd;
            argsAdd << "add";
            argsAdd << destCopy;
            executeGit(GIT_ADD, argsAdd);

            QString message=QString("copied from %1 to %2 using QTPass.");
            message= message.arg(old_path.absoluteFilePath()).arg(destCopy);
            GitCommit(destCopy, message);
            if(QtPassSettings::isAutoPush()){
              GitPush();
            }
        }
    }

    // end pass shell script
}
/**
 * @brief ImitatePass::hasSneakyPaths
 *  you cant move copy accross the file system.
 *  adapted from the original pass implementation
 *  see https://www.passwordstore.org/
 * @param paths to check
 * @return
 */
bool ImitatePass::hasSneakyPaths(const QStringList paths){
    foreach (QString path, paths) {
        if(path.endsWith("/..")){
            return true;
        }
        if(path.startsWith("../")){
            return true;
        }
        if(path.contains("/../")){
            return true;
        }
        if(path == ".."){
            return true;
        }
    }
    return false;
}

/**
 * @brief ImitatePass::executeGpg easy wrapper for running gpg commands
 * @param args
 */
void ImitatePass::executeGpg(int id, const QStringList &args, QString input,
                      bool readStdout,bool readStderr)
{
    executeWrapper(id, QtPassSettings::getGpgExecutable(), args, input,
                   readStdout, readStderr);
}
/**
 * @brief ImitatePass::executeGit easy wrapper for running git commands
 * @param args
 */
void ImitatePass::executeGit(int id, const QStringList &args, QString input,
                      bool readStdout,bool readStderr)
{
    executeWrapper(id, QtPassSettings::getGitExecutable(), args, input,
                   readStdout, readStderr);
}
