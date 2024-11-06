// SPDX-License-Identifier: MIT
#include <Common/Async.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>

#include <QStandardItemModel>
#include <QQmlApplicationEngine>

#include <thread>

class QQuickWindow;

class ConfigModel : public QStandardItemModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  ConfigModel();

  void Reload();

public slots:
  bool has(const QString&, bool unused) const;
  void erase(const QString&);

  bool getBool(const QString&, bool unused) const;
  QString getString(const QString&, bool unused) const;
  QStringList getStringList(const QString&, bool unused) const;
  int getInt(const QString&, bool unused) const;

  void setBool(const QString&, bool);
  void setString(const QString&, const QString&);
  void setStringList(const QString&, const QStringList&);
  void setInt(const QString&, int value);
};

class HostLibsModel : public QStandardItemModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  fextl::unordered_map<fextl::string, bool> HostLibsDB;

  HostLibsModel();

  QHash<int, QByteArray> roleNames() const override;

  bool setData(const QModelIndex&, const QVariant&, int role) override;

  void Reload(const fextl::string& Filename);
};

class RootFSModel : public QStandardItemModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  std::thread Thread;
  fasio::poll_reactor INotifyReactor;

  void INotifyThreadFunc(int INotifyFD);

public:
  RootFSModel();
  ~RootFSModel();

public slots:
  void Reload();

  bool hasItem(const QString&) const;

  QUrl getBaseUrl() const;
};

class ConfigRuntime : public QObject {
  Q_OBJECT

  QQmlApplicationEngine Engine;
  QQuickWindow* Window = nullptr;
  RootFSModel RootFSList;
  ConfigModel ConfigModelInst;
  HostLibsModel HostLibs;

public:
  ConfigRuntime(const QString& ConfigFilename);

public slots:
  void onSave(const QUrl&);
  void onLoad(const QUrl&);
};
