// SPDX-License-Identifier: MIT
#include "Main.h"

#include <Common/Async.h>
#include <Common/Config.h>
#include <Common/FileFormatCheck.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Config/Config.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <QApplication>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <sys/inotify.h>
#include <poll.h>

#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <utility>

namespace fextl {
// Helper to convert a std::filesystem::path to a fextl::string.
inline fextl::string string_from_path(const std::filesystem::path& Path) {
  return Path.string().c_str();
}
} // namespace fextl

static fextl::unique_ptr<FEXCore::Config::Layer> LoadedConfig {};
static fextl::map<FEXCore::Config::ConfigOption, std::pair<std::string, std::string_view>> ConfigToNameLookup;
static fextl::map<std::string, FEXCore::Config::ConfigOption> NameToConfigLookup;

#include "Common/JSONPool.h"
#include <FEXCore/Utils/FileLoading.h>

static void LoadThunkDatabase(fextl::unordered_map<fextl::string, bool>& HostLibsDB, bool Global) {
  auto ThunkDBPath = FEXCore::Config::GetConfigDirectory(Global) + "ThunksDB.json";
  fextl::vector<char> FileData;
  if (!FEXCore::FileLoading::LoadFile(FileData, ThunkDBPath)) {
    return;
  }

  FEX::JSON::JsonAllocator Pool {};
  const json_t* json = FEX::JSON::CreateJSON(FileData, Pool);
  if (!json) {
    ERROR_AND_DIE_FMT("Failed to parse JSON from ThunkDB file '{}' - invalid JSON format", ThunkDBPath);
  }

  const json_t* DB = json_getProperty(json, "DB");
  if (!DB || JSON_OBJ != json_getType(DB)) {
    return;
  }

  for (const json_t* Library = json_getChild(DB); Library != nullptr; Library = json_getSibling(Library)) {
    HostLibsDB[json_getName(Library)] = false;
  }
}

ConfigModel::ConfigModel() {
  setItemRoleNames(QHash<int, QByteArray> {{Qt::DisplayRole, "display"}, {Qt::UserRole + 1, "optionType"}, {Qt::UserRole + 2, "optionValue"}});
  Reload();
}

void ConfigModel::Reload() {
  const auto& Options = LoadedConfig->GetOptionMap();

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
    Item->setData(QString::fromStdString(std::get<fextl::string>(Option.second).c_str()), Qt::UserRole + 2);
    appendRow(Item);
  }
  endResetModel();
}

bool ConfigModel::has(const QString& Name, bool) const {
  return LoadedConfig->OptionExists(NameToConfigLookup.at(Name.toStdString()));
}

void ConfigModel::erase(const QString& Name) {
  assert(has(Name, false));
  LoadedConfig->Erase(NameToConfigLookup.at(Name.toStdString()));
  Reload();
}

bool ConfigModel::getBool(const QString& Name, bool) const {
  auto ret = LoadedConfig->Get(NameToConfigLookup.at(Name.toStdString()));
  if (!ret || !*ret) {
    throw std::runtime_error("Could not find setting");
  }
  return **ret == "1";
}

void ConfigModel::setBool(const QString& Name, bool Value) {
  LoadedConfig->Set(NameToConfigLookup.at(Name.toStdString()), Value ? "1" : "0");
  Reload();
}

void ConfigModel::setString(const QString& Name, const QString& Value) {
  LoadedConfig->Set(NameToConfigLookup.at(Name.toStdString()), Value.toStdString());
  Reload();
}

void ConfigModel::setStringList(const QString& Name, const QStringList& Values) {
  const auto& Option = NameToConfigLookup.at(Name.toStdString());
  LoadedConfig->Erase(Option);
  for (auto& Value : Values) {
    LoadedConfig->Set(Option, Value.toStdString().c_str());
  }
  Reload();
}

void ConfigModel::setInt(const QString& Name, int Value) {
  LoadedConfig->Set(NameToConfigLookup.at(Name.toStdString()), std::to_string(Value));
  Reload();
}

QString ConfigModel::getString(const QString& Name, bool) const {
  auto ret = LoadedConfig->Get(NameToConfigLookup.at(Name.toStdString()));
  if (!ret || !*ret) {
    throw std::runtime_error("Could not find setting");
  }
  return QString::fromUtf8((*ret)->c_str());
}

QStringList ConfigModel::getStringList(const QString& Name, bool) const {
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
  std::filesystem::path Dirs[] = {std::filesystem::absolute(ConfigFilename).parent_path(),
                                  std::filesystem::absolute(FEXCore::Config::GetDataDirectory()) / "RootFS/"};
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

HostLibsModel::HostLibsModel() {
  // Load list of available libraries
  LoadThunkDatabase(HostLibsDB, true);
  LoadThunkDatabase(HostLibsDB, false);
}

void HostLibsModel::Reload(const fextl::string& Path) {
  for (auto& [_, Enabled] : HostLibsDB) {
    Enabled = false;
  }

  {
    fextl::vector<char> FileData;
    if (!FEXCore::FileLoading::LoadFile(FileData, Path)) {
      goto RenderItems;
    }

    FEX::JSON::JsonAllocator Pool {};
    const json_t* json = FEX::JSON::CreateJSON(FileData, Pool);
    if (!json) {
      goto RenderItems;
    }

    const json_t* ThunksDB = json_getProperty(json, "ThunksDB");
    if (!ThunksDB) {
      goto RenderItems;
    }

    for (const json_t* Item = json_getChild(ThunksDB); Item != nullptr; Item = json_getSibling(Item)) {
      auto DBObject = HostLibsDB.find(json_getName(Item));
      if (DBObject != HostLibsDB.end()) {
        DBObject->second = (json_getInteger(Item) != 0);
      }
    }
  }

RenderItems:
  beginResetModel();
  removeRows(0, rowCount());
  for (auto& [Name, Enabled] : HostLibsDB) {
    auto Item = new QStandardItem(QString::fromUtf8(Name.c_str()));
    Item->setData(Enabled, Qt::CheckStateRole);
    appendRow(Item);
  }
  endResetModel();
}

QHash<int, QByteArray> HostLibsModel::roleNames() const {
  auto ret = QStandardItemModel::roleNames();
  ret[Qt::CheckStateRole] = "checked";
  return ret;
}

bool HostLibsModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  std::next(HostLibsDB.begin(), index.row())->second = value.toBool();
  return QStandardItemModel::setData(index, value, role);
}

RootFSModel::RootFSModel() {
  auto INotifyFD = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

  fextl::string RootFS = FEXCore::Config::GetDataDirectory(false) + "RootFS/";
  int LocalFolderWD = inotify_add_watch(INotifyFD, RootFS.c_str(), IN_CREATE | IN_DELETE);

  RootFS = FEXCore::Config::GetDataDirectory(true) + "RootFS/";
  int GlobalFolderWD = inotify_add_watch(INotifyFD, RootFS.c_str(), IN_CREATE | IN_DELETE);
  if (INotifyFD != -1 && (LocalFolderWD != -1 || GlobalFolderWD != -1)) {
    INotifyReactor.enable_async_stop();
    Thread = std::thread {&RootFSModel::INotifyThreadFunc, this, INotifyFD};
  } else {
    qWarning() << "Could not set up inotify. RootFS folder won't be monitored for changes.";
  }

  // Load initial data
  Reload();
}

RootFSModel::~RootFSModel() {
  INotifyReactor.stop_async();
  Thread.join();
}

void RootFSModel::Reload() {
  beginResetModel();
  removeRows(0, rowCount());

  std::vector<QString> NamedRootFS {};
  for (auto Global : {false, true}) {
    const fextl::string RootFS = FEXCore::Config::GetDataDirectory(Global) + "RootFS/";

    std::error_code ec;
    for (auto& it : std::filesystem::directory_iterator(RootFS, ec)) {
      std::string Path {};
      if (Global) {
        // If global then keep the full path.
        Path = it.path();
      } else {
        // If local then only use the filename.
        Path = it.path().filename();
      }

      if (it.is_directory()) {
        NamedRootFS.push_back(QString::fromStdString(Path));
      } else if (it.is_regular_file()) {
        // If it is a regular file then we need to check if it is a valid archive
        if (it.path().extension() == ".sqsh" && FEX::FormatCheck::IsSquashFS(fextl::string_from_path(it.path()))) {
          NamedRootFS.push_back(QString::fromStdString(Path));
        } else if (it.path().extension() == ".ero" && FEX::FormatCheck::IsEroFS(fextl::string_from_path(it.path()))) {
          NamedRootFS.push_back(QString::fromStdString(Path));
        }
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

void RootFSModel::INotifyThreadFunc(int INotifyFD) {
  fasio::posix_descriptor INotify {INotifyReactor, INotifyFD};

  INotify.async_wait([this, INotifyFD](fasio::error ec) {
    // Spin through the events, we don't actually care what they are
    constexpr size_t DATA_SIZE = (16 * (sizeof(struct inotify_event) + NAME_MAX + 1));
    char buf[DATA_SIZE];
    while (read(INotifyFD, buf, DATA_SIZE) > 0)
      ;

    // Queue update to the data model
    QMetaObject::invokeMethod(this, "Reload");
    return fasio::post_callback::repeat;
  });

  INotifyReactor.run();
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
#define OPT_BASE(type, group, enum, json, default)                                            \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) {            \
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(default)); \
  }
#define OPT_STR(group, enum, json, default)                                        \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) { \
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, default);      \
  }
#define OPT_STRARRAY(group, enum, json, default) // Do nothing
#define OPT_STRENUM(group, enum, json, default)                                                                      \
  if (!LoadedConfig->OptionExists(FEXCore::Config::ConfigOption::CONFIG_##enum)) {                                   \
    LoadedConfig->Set(FEXCore::Config::ConfigOption::CONFIG_##enum, std::to_string(FEXCore::ToUnderlying(default))); \
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
  HostLibs.Reload(ConfigFilename.toStdString().c_str());

  qmlRegisterSingletonInstance<ConfigModel>("FEX.ConfigModel", 1, 0, "ConfigModel", &ConfigModelInst);
  qmlRegisterSingletonInstance<HostLibsModel>("FEX.HostLibsModel", 1, 0, "HostLibsModel", &HostLibs);
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
  // If no RootFS is selected, assume another Config layer is setting it up and drop it from the local configuration
  auto RootFS = LoadedConfig->Get(FEXCore::Config::ConfigOption::CONFIG_ROOTFS).value_or(nullptr);
  if (RootFS && RootFS->empty()) {
    LoadedConfig->Erase(FEXCore::Config::ConfigOption::CONFIG_ROOTFS);
  }

  qInfo() << "Saving to" << Filename.toLocalFile().toStdString().c_str();
  FEX::Config::SaveLayerToJSON(Filename.toLocalFile().toStdString().c_str(), LoadedConfig.get(), HostLibs.HostLibsDB);
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
  HostLibs.Reload(Filename.toLocalFile().toStdString().c_str());

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
  App.setWindowIcon(QIcon(":/icon.png"));
  return App.exec();
}
