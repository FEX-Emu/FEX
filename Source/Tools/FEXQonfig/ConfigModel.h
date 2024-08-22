#include <QStandardItemModel>
#include <QQmlEngine>

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
  int getInt(const QString&, bool unused) const;

  void setBool(const QString&, bool);
  void setString(const QString&, const QString&);
  void setInt(const QString&, int value);
};

class EnvVarModel : public QStandardItemModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  EnvVarModel();

  void Reload();
};

class RootFSModel : public QStandardItemModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  RootFSModel();

  void Reload();

public slots:
  bool hasItem(const QString&) const;

  QUrl getBaseUrl() const;
};

#include <QApplication>

class ConfigRuntime : public QObject {
  Q_OBJECT

public:
  ConfigRuntime() {}

public slots:
  void onSave(const QUrl&);
  void onLoad(const QUrl&);
};
