// SPDX-License-Identifier: MIT
#include <QStandardItemModel>
#include <QQmlApplicationEngine>

#include <latch>
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

class RootFSModel : public QStandardItemModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  std::thread Thread;
  std::latch ExitRequest {1};

  int INotifyFD;
  int FolderFD;

  void INotifyThreadFunc();

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

public:
  ConfigRuntime(const QString& ConfigFilename);

public slots:
  void onSave(const QUrl&);
  void onLoad(const QUrl&);
};
