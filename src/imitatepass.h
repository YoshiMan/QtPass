#ifndef IMITATEPASS_H
#define IMITATEPASS_H

#include "pass.h"

/*!
    \class ImitatePass
    \brief Imitates pass features when pass is not enabled or available
*/
class ImitatePass : public Pass {
  Q_OBJECT

  bool removeDir(const QString &dirName);

  void GitCommit(const QString &file, const QString &msg);

  bool hasSneakyPaths(const QStringList paths);
public:
  ImitatePass();
  virtual ~ImitatePass() {}
  virtual void GitInit() Q_DECL_OVERRIDE;
  virtual void GitPull() Q_DECL_OVERRIDE;
  virtual void GitPull_b() Q_DECL_OVERRIDE;
  virtual void GitPush() Q_DECL_OVERRIDE;
  virtual void Show(QString file) Q_DECL_OVERRIDE;
  virtual int Show_b(QString file) Q_DECL_OVERRIDE;
  virtual void Insert(QString file, QString value,
                      bool overwrite = false) Q_DECL_OVERRIDE;
  virtual void Remove(QString file, bool isDir = false) Q_DECL_OVERRIDE;
  virtual void Init(QString path, const QList<UserInfo> &list) Q_DECL_OVERRIDE;

  void reencryptPath(QString dir);

private:
  void executeGit(int id, const QStringList &args, QString input = QString(),
                  bool readStdout = true, bool readStderr = true);

  void executeGpg(int id, const QStringList &args, QString input = QString(),
                  bool readStdout = true,bool readStderr = true);
signals:
  void startReencryptPath();
  void endReencryptPath();
  void lastDecrypt(QString);

  // Pass interface
public:
  void Move(const QString srcParam, const QString destParam,
            const bool force = false) Q_DECL_OVERRIDE;
  void Copy(const QString src, const QString dest,
            const bool force = false) Q_DECL_OVERRIDE;
};

#endif // IMITATEPASS_H
