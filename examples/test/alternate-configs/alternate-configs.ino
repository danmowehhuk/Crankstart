/*
 * alternate-configs — Swapping implementations between configs
 *
 * Demonstrates:
 *   - How to define a common abstract base for swappable module implementations
 *   - A production AppConfig using a real service implementation
 *   - A test AppConfig that injects a mock — with identical application logic
 *   - Using static_cast to the common base so app logic works with either config
 */

#include <TestTool.h>
#include <Crankstart.h>

using namespace crankstart;

// ---------------------------------------------------------------------------
// Service interface and implementations
// ---------------------------------------------------------------------------

class Storage {
public:
  virtual void write(const char* key, int value) = 0;
  virtual int  read(const char* key) = 0;
  virtual ~Storage() = default;
};

// Production storage — stores a single value in RAM (simplified)
class RealStorage : public Storage {
  int _stored = 0;
public:
  void write(const char* key, int value) override { _stored = value; }
  int  read(const char* key)             override { return _stored; }
};

// Mock storage — records whether write() was called, for verification
class MockStorage : public Storage {
  int  _value       = 0;
  bool _writeCalled = false;
public:
  void write(const char* key, int value) override {
    _writeCalled = true;
    _value = value;
  }
  int  read(const char* key)  override { return _value; }
  bool writeCalled() const            { return _writeCalled; }
};

// ---------------------------------------------------------------------------
// Module name constants
// ---------------------------------------------------------------------------

static const char STORAGE_MODULE[] PROGMEM = "storage";
static const char LOGIC_MODULE[]   PROGMEM = "logic";

// ---------------------------------------------------------------------------
// Common abstract base for all storage module implementations.
//
// Both RealStorageModule and MockStorageModule inherit from this, so
// AppLogicModule can cast app->get(STORAGE_MODULE) to StorageModuleBase*
// regardless of which concrete implementation is registered in the AppConfig.
// ---------------------------------------------------------------------------
class StorageModuleBase : public Module, public Provider<Storage> {
protected:
  StorageModuleBase() = default;
};

// Production storage module
class RealStorageModule : public StorageModuleBase {
  RealStorage* _storage = nullptr;
public:
  const char* name() const override { return STORAGE_MODULE; }
  void initialize() override { _storage = new RealStorage(); }
  Storage* get()    override { return _storage; }
  ~RealStorageModule() override {
    if (_storage) { delete _storage; _storage = nullptr; }
  }
};

// Test storage module — same name, fully swappable with RealStorageModule
class MockStorageModule : public StorageModuleBase {
  MockStorage* _mock = nullptr;
public:
  const char* name() const override { return STORAGE_MODULE; }
  void initialize() override { _mock = new MockStorage(); }
  Storage* get()    override { return _mock; }
  MockStorage* getMock() { return _mock; }
  ~MockStorageModule() override {
    if (_mock) { delete _mock; _mock = nullptr; }
  }
};

// ---------------------------------------------------------------------------
// AppLogicModule — unchanged between production and test configs.
//
// It casts to StorageModuleBase* (the common base) so it works with
// both RealStorageModule and MockStorageModule without modification.
// ---------------------------------------------------------------------------
class AppLogicModule : public Module {
  int _lastRead = -1;
public:
  const char* name() const override { return LOGIC_MODULE; }

  DependsOn deps() const override {
    static const char* deps[] = { STORAGE_MODULE, nullptr };
    return { deps };
  }

  void initialize() override {
    StorageModuleBase* storageMod =
      static_cast<StorageModuleBase*>(app->get(STORAGE_MODULE));
    Storage* storage = storageMod->get();
    storage->write("brightness", 42);
    _lastRead = storage->read("brightness");
  }

  int getLastRead() const { return _lastRead; }
};

// ---------------------------------------------------------------------------
// AppConfig subclasses — the only difference is which storage module is used
// ---------------------------------------------------------------------------

class ProdAppConfig : public AppConfig {
  RealStorageModule _storage;
  AppLogicModule    _logic;
  void configure() override {
    addModule(&_storage);
    addModule(&_logic);
  }
};

class TestAppConfig : public AppConfig {
  MockStorageModule _storage;
  AppLogicModule    _logic;
  void configure() override {
    addModule(&_storage);
    addModule(&_logic);
  }
public:
  MockStorageModule* getMockStorage() { return &_storage; }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void testProdConfigWorks(TestInvocation* t) {
  t->setName(F("Production config: app logic reads from real storage"));
  ProdAppConfig config;
  App app(config);
  app.start();
  AppLogicModule* logic =
    static_cast<AppLogicModule*>(app.get(LOGIC_MODULE));
  t->assert(logic != nullptr, F("AppLogicModule should be found"));
  t->assert(logic->getLastRead() == 42, F("Should read back 42"));
}

void testTestConfigSwapsMock(TestInvocation* t) {
  t->setName(F("Test config: same app logic, mock storage injected"));
  TestAppConfig config;
  App app(config);
  app.start();
  AppLogicModule* logic =
    static_cast<AppLogicModule*>(app.get(LOGIC_MODULE));
  t->assert(logic != nullptr, F("AppLogicModule should be found"));
  // AppLogicModule behavior is identical regardless of storage implementation
  t->assert(logic->getLastRead() == 42, F("Should still read back 42"));
  // The mock lets us verify the write was called
  t->assert(config.getMockStorage()->getMock()->writeCalled(),
    F("Mock should have recorded the write call"));
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  TestFunction tests[] = {
    testProdConfigWorks,
    testTestConfigSwapsMock
  };

  runTestSuite(tests);
}

void loop() {}
