#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include "ConfigModel.h"

#include <FEXCore/Config/Config.h>
#include <Common/Config.h>
#include <Common/FileFormatCheck.h>

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/map.h>

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

static void ConfigInit() {
#define OPT_BASE(type, group, enum, json, default)                                 \
  ConfigToNameLookup[FEXCore::Config::ConfigOption::CONFIG_##enum].first = #json;  \
  ConfigToNameLookup[FEXCore::Config::ConfigOption::CONFIG_##enum].second = #type; \
  NameToConfigLookup[#json] = FEXCore::Config::ConfigOption::CONFIG_##enum;
#include <FEXCore/Config/ConfigValues.inl>
#undef OPT_BASE
}

QQuickWindow* Window = nullptr; // TODO: Drop global

static std::mutex NamedRootFSUpdater {};

fextl::unique_ptr<RootFSModel> RootFSList;

RootFSModel::RootFSModel() {
  Reload();
}

// TODO: Watch via inotify
void RootFSModel::Reload() {
  std::unique_lock<std::mutex> lk {NamedRootFSUpdater};

  beginResetModel();
  removeRows(0, rowCount());

  fextl::string RootFS = FEXCore::Config::GetDataDirectory() + "RootFS/";
  std::error_code ec {};
  if (!std::filesystem::exists(RootFS, ec)) {
    // Doesn't exist, create the the folder as a user convenience
    if (!std::filesystem::create_directories(RootFS, ec)) {
      // Well I guess we failed
      return;
    }
  }
  std::vector<std::string> NamedRootFS {};
  for (auto& it : std::filesystem::directory_iterator(RootFS)) {
    if (it.is_directory()) {
      NamedRootFS.emplace_back(it.path().filename());
    } else if (it.is_regular_file()) {
      // If it is a regular file then we need to check if it is a valid archive
      if (it.path().extension() == ".sqsh" && FEX::FormatCheck::IsSquashFS(fextl::string_from_path(it.path()))) {
        NamedRootFS.emplace_back(it.path().filename());
      } else if (it.path().extension() == ".ero" && FEX::FormatCheck::IsEroFS(fextl::string_from_path(it.path()))) {
        NamedRootFS.emplace_back(it.path().filename());
      }
    }
  }
  std::sort(NamedRootFS.begin(), NamedRootFS.end());
  for (auto& Entry : NamedRootFS) {
    appendRow(new QStandardItem(Entry.c_str()));
  }

  endResetModel();
}

bool RootFSModel::hasItem(const QString& Name) const {
  std::unique_lock<std::mutex> lk {NamedRootFSUpdater};
  return !findItems(Name, Qt::MatchExactly).empty();
}

QUrl RootFSModel::getBaseUrl() const {
  return QUrl::fromLocalFile(QString::fromStdString(FEXCore::Config::GetDataDirectory().c_str()) + "RootFS/");
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

fextl::unique_ptr<ConfigModel> ConfigModelInst;
fextl::unique_ptr<EnvVarModel> EnvVarModelInst;

void ConfigRuntime::onSave(const QUrl& Filename) {
  qInfo() << "Saving to" << Filename.toLocalFile().toStdString().c_str();
  FEX::Config::SaveLayerToJSON(Filename.toLocalFile().toStdString().c_str(), LoadedConfig.get());
}

void ConfigRuntime::onLoad(const QUrl& Filename) {
  // TODO: Distinguish between "load" and "overlay".
  //       Currently, the new configuration is overlaid on top of the previous one.

  // TODO: Handle failure
  OpenFile(Filename.toLocalFile().toStdString().c_str());

  ConfigModelInst->Reload();
  EnvVarModelInst->Reload();
  RootFSList->Reload();

  QMetaObject::invokeMethod(Window, "refreshUI");
}

EnvVarModel::EnvVarModel() {
  Reload();
}

void EnvVarModel::Reload() {
  FEXCore::Config::LayerValue EmptyVars;
  auto Vars = LoadedConfig->All(FEXCore::Config::ConfigOption::CONFIG_ENV).value_or(&EmptyVars);

  beginResetModel();

  removeRows(0, rowCount());
  for (auto& Var : *Vars) {
    appendRow(new QStandardItem(Var.c_str()));
  }

  endResetModel();
}

int main(int Argc, char** Argv) {
  QApplication App(Argc, Argv);

  FEX::Config::InitializeConfigs();
  ConfigInit();

  fextl::string ConfigFilename = Argc > 1 ? Argv[1] : FEXCore::Config::GetConfigFileLocation();
  qInfo() << "Opening" << ConfigFilename.c_str();
  if (OpenFile(ConfigFilename)) {
  } else {
    // Load defaults if not found
    ConfigFilename.clear();
    LoadDefaultSettings();
  }

  ConfigRuntime Runtime;

  ConfigModelInst = fextl::make_unique<ConfigModel>();
  EnvVarModelInst = fextl::make_unique<EnvVarModel>();
  RootFSList = fextl::make_unique<RootFSModel>();

  QQmlApplicationEngine Engine;
  qmlRegisterSingletonInstance<ConfigModel>("FEX.ConfigModel", 1, 0, "ConfigModel", ConfigModelInst.get());
  qmlRegisterSingletonInstance<EnvVarModel>("FEX.EnvVarModel", 1, 0, "EnvVarModel", EnvVarModelInst.get());
  qmlRegisterSingletonInstance<RootFSModel>("FEX.RootFSModel", 1, 0, "RootFSModel", RootFSList.get());
  Engine.load(QUrl("qrc:/main.qml"));
  /*auto**/ Window = qobject_cast<QQuickWindow*>(Engine.rootObjects().first());
  if (!ConfigFilename.empty()) {
    Window->setProperty("configFilename", "file://" + QString {ConfigFilename.c_str()});
  } else {
    Window->setProperty("configDirty", true);
  }
  ConfigRuntime::connect(Window, SIGNAL(selectedConfigFile(const QUrl&)), &Runtime, SLOT(onLoad(const QUrl&)));
  ConfigRuntime::connect(Window, SIGNAL(triggeredSave(const QUrl&)), &Runtime, SLOT(onSave(const QUrl&)));
  ConfigRuntime::connect(ConfigModelInst.get(), SIGNAL(modelReset()), Window, SLOT(refreshUI()));

  return App.exec();
}
