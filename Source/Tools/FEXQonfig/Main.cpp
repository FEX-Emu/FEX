// SPDX-License-Identifier: MIT
#include "Main.h"

#include <Common/Config.h>
#include <Common/FileFormatCheck.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/Config/Config.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <QApplication>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <sys/inotify.h>

namespace fextl {
// Helper to convert a std::filesystem::path to a fextl::string.
inline fextl::string string_from_path(const std::filesystem::path& Path) {
  return Path.string().c_str();
}
} // namespace fextl

static fextl::unique_ptr<FEXCore::Config::Layer> LoadedConfig {};
static fextl::map<FEXCore::Config::ConfigOption, std::pair<std::string, std::string_view>> ConfigToNameLookup;
static fextl::map<std::string, FEXCore::Config::ConfigOption> NameToConfigLookup;

ConfigModel::ConfigModel() {
  setItemRoleNames(QHash<int, QByteArray> {{Qt::DisplayRole, "display"}, {Qt::UserRole + 1, "optionType"}, {Qt::UserRole + 2, "optionValue"}});
  Reload();
}

void ConfigModel::Reload() {
  auto Options = LoadedConfig->GetOptionMap();

  beginResetModel();
  removeRows(0, rowCount());
  for (auto& Option : Options) {
    if (!LoadedConfig->OptionExists(Option.first)) {
      continue;
    }

    auto& [Name, TypeId] = ConfigToNameLookup.find(Option.first)->second;
    auto Item = new QStandardItem(QString::fromStdString(Name));

    const char* OptionType = TypeId.data();
    Item->setData(OptionType, Qt::UserRole + 1);
    Item->setData(QString::fromStdString(Option.second.front().c_str()), Qt::UserRole + 2);
    appendRow(Item);
  }
  endResetModel();
}

bool ConfigModel::has(const QString& Name, bool) const {
  auto Options = LoadedConfig->GetOptionMap();
  return LoadedConfig->OptionExists(NameToConfigLookup.at(Name.toStdString()));
}

void ConfigModel::erase(const QString& Name) {
  assert(has(Name, false));
  auto Options = LoadedConfig->GetOptionMap();
  LoadedConfig->Erase(NameToConfigLookup.at(Name.toStdString()));
  Reload();
}

bool ConfigModel::getBool(const QString& Name, bool) const {
  auto Options = LoadedConfig->GetOptionMap();

  auto ret = LoadedConfig->Get(NameToConfigLookup.at(Name.toStdString()));
  if (!ret || !*ret) {
    throw std::runtime_error("Could not find setting");
  }
  return **ret == "1";
}

void ConfigModel::setBool(const QString& Name, bool Value) {
  auto Options = LoadedConfig->GetOptionMap();
  LoadedConfig->EraseSet(NameToConfigLookup.at(Name.toStdString()), Value ? "1" : "0");
  Reload();
}

void ConfigModel::setString(const QString& Name, const QString& Value) {
  auto Options = LoadedConfig->GetOptionMap();
  LoadedConfig->EraseSet(NameToConfigLookup.at(Name.toStdString()), Value.toStdString());
  Reload();
}

void ConfigModel::setStringList(const QString& Name, const QStringList& Values) {
  auto Options = LoadedConfig->GetOptionMap();

  const auto& Option = NameToConfigLookup.at(Name.toStdString());
  LoadedConfig->Erase(Option);
  for (auto& Value : Values) {
    LoadedConfig->Set(Option, Value.toStdString().c_str());
  }
  Reload();
}

void ConfigModel::setInt(const QString& Name, int Value) {
  auto Options = LoadedConfig->GetOptionMap();
  LoadedConfig->EraseSet(NameToConfigLookup.at(Name.toStdString()), std::to_string(Value));
  Reload();
}

QString ConfigModel::getString(const QString& Name, bool) const {
  auto Options = LoadedConfig->GetOptionMap();

  auto ret = LoadedConfig->Get(NameToConfigLookup.at(Name.toStdString()));
  if (!ret || !*ret) {
    throw std::runtime_error("Could not find setting");
  }
  return QString::fromUtf8((*ret)->c_str());
}

QStringList ConfigModel::getStringList(const QString& Name, bool) const {
  auto Options = LoadedConfig->GetOptionMap();

  auto Values = LoadedConfig->All(NameToConfigLookup.at(Name.toStdString()));
  if (!Values || !*Values) {
    return {};
  }
  QStringList Ret;
  for (auto& Value : **Values) {
    Ret.append(Value.c_str());
  }
  return Ret;
}

int ConfigModel::getInt(const QString& Name, bool) const {
  auto Options = LoadedConfig->GetOptionMap();

  auto ret = LoadedConfig->Get(NameToConfigLookup.at(Name.toStdString()));
  if (!ret || !*ret) {
    throw std::runtime_error("Could not find setting");
  }
  int value;
  auto res = std::from_chars(&*(*ret)->begin(), &*(*ret)->end(), value);
  if (res.ptr != &*(*ret)->end()) {
    throw std::runtime_error("Could not parse integer");
  }
  return value;
}

static void LoadDefaultSettings() {
  LoadedConfig = fextl::make_unique<FEX::Config::EmptyMapper>();
#define OPT_BASE(type, group, enum, json, default) LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(default));
#define OPT_STR(group, enum, json, default) LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, default);
#define OPT_STRARRAY(group, enum, json, default) // Do nothing
#define OPT_STRENUM(group, enum, json, default) \
  LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(FEXCore::ToUnderlying(default)));
#include <FEXCore/Config/ConfigValues.inl>

  // Erase unnamed options which shouldn't be set
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS_INTERPRETER);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_INTERPRETER_INSTALLED);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_FILENAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_CONFIG_NAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS64BIT_MODE);
}

static void ConfigInit(fextl::string ConfigFilename) {
#define OPT_BASE(type, group, enum, json, default)                                 \
  ConfigToNameLookup[FEXCore::Config::ConfigOption::CONFIG_##enum].first = #json;  \
  ConfigToNameLookup[FEXCore::Config::ConfigOption::CONFIG_##enum].second = #type; \
  NameToConfigLookup[#json] = FEXCore::Config::ConfigOption::CONFIG_##enum;
#include <FEXCore/Config/ConfigValues.inl>
#undef OPT_BASE

  // Ensure config and RootFS directories exist
  std::error_code ec {};
  fextl::string Dirs[] = {FHU::Filesystem::ParentPath(ConfigFilename), FEXCore::Config::GetDataDirectory() + "RootFS/"};
  for (auto& Dir : Dirs) {
    bool created = std::filesystem::create_directories(Dir, ec);
    if (created) {
      qInfo() << "Created folder" << Dir.c_str();
    }
    if (ec) {
      QMessageBox err(QMessageBox::Critical, "Failed to create directory", QString("Failed to create \"%1\" folder").arg(Dir.c_str()),
                      QMessageBox::Ok);
      err.exec();
      std::exit(EXIT_FAILURE);
      return;
    }
  }
}

RootFSModel::RootFSModel() {
  INotifyFD = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

  fextl::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
  FolderFD = inotify_add_watch(INotifyFD, RootFS.c_str(), IN_CREATE | IN_DELETE);
  if (FolderFD != -1) {
    Thread = std::thread {&RootFSModel::INotifyThreadFunc, this};
  } else {
    qWarning() << "Could not set up inotify. RootFS folder won't be monitored for changes.";
  }

  // Load initial data
  Reload();
}

RootFSModel::~RootFSModel() {
  close(INotifyFD);
  INotifyFD = -1;

  ExitRequest.count_down();
  Thread.join();
}

void RootFSModel::Reload() {
  beginResetModel();
  removeRows(0, rowCount());

  fextl::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
  std::vector<QString> NamedRootFS {};
  for (auto& it : std::filesystem::directory_iterator(RootFS)) {
    if (it.is_directory()) {
      NamedRootFS.push_back(QString::fromStdString(it.path().filename()));
    } else if (it.is_regular_file()) {
      // If it is a regular file then we need to check if it is a valid archive
      if (it.path().extension() == ".sqsh" && FEX::FormatCheck::IsSquashFS(fextl::string_from_path(it.path()))) {
        NamedRootFS.push_back(QString::fromStdString(it.path().filename()));
      } else if (it.path().extension() == ".ero" && FEX::FormatCheck::IsEroFS(fextl::string_from_path(it.path()))) {
        NamedRootFS.push_back(QString::fromStdString(it.path().filename()));
      }
    }
  }
  std::sort(NamedRootFS.begin(), NamedRootFS.end(), [](const QString& a, const QString& b) { return QString::localeAwareCompare(a, b) < 0; });
  for (auto& Entry : NamedRootFS) {
    appendRow(new QStandardItem(Entry));
  }

  endResetModel();
}

bool RootFSModel::hasItem(const QString& Name) const {
  return !findItems(Name, Qt::MatchExactly).empty();
}

QUrl RootFSModel::getBaseUrl() const {
  return QUrl::fromLocalFile(QString::fromStdString(FEXCore::Config::GetDataDirectory().c_str()) + "RootFS/");
}

void RootFSModel::INotifyThreadFunc() {
  while (!ExitRequest.try_wait()) {
    constexpr size_t DATA_SIZE = (16 * (sizeof(struct inotify_event) + NAME_MAX + 1));
    char buf[DATA_SIZE];
    int Ret {};
    do {
      fd_set Set {};
      FD_ZERO(&Set);
      FD_SET(INotifyFD, &Set);
      struct timeval tv {};
      // 50 ms
      tv.tv_usec = 50000;
      Ret = select(INotifyFD + 1, &Set, nullptr, nullptr, &tv);
    } while (Ret == 0 && INotifyFD != -1);

    if (Ret == -1 || INotifyFD == -1) {
      // Just return on error
      return;
    }

    // Spin through the events, we don't actually care what they are
    while (read(INotifyFD, buf, DATA_SIZE) > 0)
      ;

    // Queue update to the data model
    QMetaObject::invokeMethod(this, "Reload");
  }
}

// Returns true on success
static bool OpenFile(fextl::string Filename) {
  std::error_code ec {};
  if (!std::filesystem::exists(Filename, ec)) {
    return false;
  }

  LoadedConfig = FEX::Config::CreateMainLayer(&Filename);
  LoadedConfig->Load();

  // Load default options and only overwrite only if the option didn't exist
#define OPT_BASE(type, group, enum, json, default)                                                 \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) {                 \
    LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(default)); \
  }
#define OPT_STR(group, enum, json, default)                                        \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) { \
    LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_##enum, default); \
  }
#define OPT_STRARRAY(group, enum, json, default) // Do nothing
#define OPT_STRENUM(group, enum, json, default)                                                                           \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) {                                        \
    LoadedConfig->EraseSet(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(FEXCore::ToUnderlying(default))); \
  }
#include <FEXCore/Config/ConfigValues.inl>

  // Erase unnamed options which shouldn't be set
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS_INTERPRETER);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_INTERPRETER_INSTALLED);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_FILENAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_APP_CONFIG_NAME);
  LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_IS64BIT_MODE);

  return true;
}

ConfigRuntime::ConfigRuntime(const QString& ConfigFilename) {
  qmlRegisterSingletonInstance<ConfigModel>("FEX.ConfigModel", 1, 0, "ConfigModel", &ConfigModelInst);
  qmlRegisterSingletonInstance<RootFSModel>("FEX.RootFSModel", 1, 0, "RootFSModel", &RootFSList);
  Engine.load(QUrl("qrc:/main.qml"));

  Window = qobject_cast<QQuickWindow*>(Engine.rootObjects().first());
  if (!ConfigFilename.isEmpty()) {
    Window->setProperty("configFilename", QUrl::fromLocalFile(ConfigFilename));
  } else {
    Window->setProperty("configFilename", QUrl::fromLocalFile(FEXCore::Config::GetConfigFileLocation().c_str()));
    Window->setProperty("configDirty", true);
    Window->setProperty("loadedDefaults", true);
  }

  ConfigRuntime::connect(Window, SIGNAL(selectedConfigFile(const QUrl&)), this, SLOT(onLoad(const QUrl&)));
  ConfigRuntime::connect(Window, SIGNAL(triggeredSave(const QUrl&)), this, SLOT(onSave(const QUrl&)));
  ConfigRuntime::connect(&ConfigModelInst, SIGNAL(modelReset()), Window, SLOT(refreshUI()));
}

void ConfigRuntime::onSave(const QUrl& Filename) {
  qInfo() << "Saving to" << Filename.toLocalFile().toStdString().c_str();
  FEX::Config::SaveLayerToJSON(Filename.toLocalFile().toStdString().c_str(), LoadedConfig.get());
}

void ConfigRuntime::onLoad(const QUrl& Filename) {
  // TODO: Distinguish between "load" and "overlay".
  //       Currently, the new configuration is overlaid on top of the previous one.

  if (!OpenFile(Filename.toLocalFile().toStdString().c_str())) {
    // This basically never happens because OpenFile performs no actual syntax checks.
    // Treat as fatal since the UI state wouldn't be consistent after ignoring the error.
    QMessageBox err(QMessageBox::Critical, tr("Could not load config file"), tr("Failed to load \"%1\"").arg(Filename.toLocalFile()),
                    QMessageBox::Ok);
    err.exec();
    QApplication::exit();
    return;
  }

  ConfigModelInst.Reload();
  RootFSList.Reload();

  QMetaObject::invokeMethod(Window, "refreshUI");
}

int main(int Argc, char** Argv) {
  QApplication App(Argc, Argv);

  FEX::Config::InitializeConfigs(FEX::Config::PortableInformation {});
  fextl::string ConfigFilename = Argc > 1 ? Argv[1] : FEXCore::Config::GetConfigFileLocation();
  ConfigInit(ConfigFilename);

  qInfo() << "Opening" << ConfigFilename.c_str();
  if (!OpenFile(ConfigFilename)) {
    // Load defaults if not found
    ConfigFilename.clear();
    LoadDefaultSettings();
  }

  ConfigRuntime Runtime(ConfigFilename.c_str());

  return App.exec();
}
