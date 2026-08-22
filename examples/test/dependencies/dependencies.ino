/*
 * dependencies — Provider<T>, DependsOn, and dependency injection
 *
 * Demonstrates:
 *   - Using Provider<T> to expose a service from one module
 *   - Using DependsOn to declare that one module requires another
 *   - Using app->get() with static_cast to retrieve a dependency
 *   - That modules can be added to AppConfig in any order — Crankstart
 *     always initializes dependencies first
 */

#include <TestTool.h>
#include <Crankstart.h>

using namespace crankstart;

// A simple service class. CounterModule creates one and exposes it
// to other modules via Provider<Counter>.
class Counter {
  int _count = 0;
public:
  void increment() { _count++; }
  int  value() const { return _count; }
};

// Module name constants — PROGMEM, compared by pointer equality.
static const char COUNTER_MODULE[]   PROGMEM = "counter";
static const char PROCESSOR_MODULE[] PROGMEM = "processor";

// CounterModule creates a Counter and exposes it via Provider<Counter>.
// Other modules access it by casting app->get(COUNTER_MODULE) to
// CounterModule* and then calling ->get().
class CounterModule : public Module, public Provider<Counter> {
  Counter* _counter = nullptr;
public:
  const char* name() const override { return COUNTER_MODULE; }

  void initialize() override {
    _counter = new Counter();
  }

  Counter* get() override { return _counter; }

  ~CounterModule() override {
    if (_counter) { delete _counter; _counter = nullptr; }
  }
};

// ProcessorModule depends on CounterModule. Declaring the dependency
// ensures CounterModule::initialize() always runs first, regardless of
// the order modules are added to the AppConfig.
class ProcessorModule : public Module {
  Counter* _counter   = nullptr;
  bool _initialized   = false;
public:
  const char* name() const override { return PROCESSOR_MODULE; }

  DependsOn deps() const override {
    // nullptr-terminated array of module name pointers
    static const char* deps[] = { COUNTER_MODULE, nullptr };
    return { deps };
  }

  void initialize() override {
    // app->get() returns Module*; cast to the concrete provider type
    // with static_cast. Using the wrong type compiles but crashes at runtime.
    CounterModule* counterMod =
      static_cast<CounterModule*>(app->get(COUNTER_MODULE));
    _counter     = counterMod->get();  // retrieve the Counter service
    _initialized = true;
  }

  void loop() override {
    if (_counter) _counter->increment();
  }

  bool wasInitialized() const { return _initialized; }
  int  counterValue()   const { return _counter ? _counter->value() : -1; }
};

// Modules can be added in ANY ORDER. Crankstart sorts them so that
// CounterModule is always initialized before ProcessorModule.
class DepsAppConfig : public AppConfig {
  ProcessorModule _processor;  // added first, but initialized second
  CounterModule   _counter;
  void configure() override {
    addModule(&_processor);
    addModule(&_counter);
  }
};

void testDependencyIsInjected(TestInvocation* t) {
  t->setName(F("ProcessorModule receives Counter from CounterModule"));
  DepsAppConfig config;
  App app(config);
  app.start();
  ProcessorModule* proc =
    static_cast<ProcessorModule*>(app.get(PROCESSOR_MODULE));
  t->verify(proc != nullptr, F("ProcessorModule should be found"));
  t->verify(proc->wasInitialized(), F("ProcessorModule should be initialized"));
  // counterValue() returns -1 if _counter is nullptr (injection failed)
  t->verify(proc->counterValue() == 0, F("Counter starts at 0, not -1"));
}

void testSharedServiceState(TestInvocation* t) {
  t->setName(F("Both modules share the same Counter instance"));
  DepsAppConfig config;
  App app(config);
  app.start();
  app.setup();
  app.loop();  // ProcessorModule::loop() increments the shared Counter

  // Verify via CounterModule that the same Counter object was incremented
  CounterModule* counterMod =
    static_cast<CounterModule*>(app.get(COUNTER_MODULE));
  t->verify(counterMod->get()->value() == 1,
    F("Counter should be 1 after one loop()"));
}

void testAddOrderDoesNotMatter(TestInvocation* t) {
  t->setName(F("Dependency order resolved even if ProcessorModule added first"));
  // ProcessorModule was added before CounterModule in configure(), but
  // Crankstart ensures CounterModule::initialize() runs first.
  // If ordering were wrong, counterValue() would return -1 (nullptr crash).
  DepsAppConfig config;
  App app(config);
  app.start();
  ProcessorModule* proc =
    static_cast<ProcessorModule*>(app.get(PROCESSOR_MODULE));
  t->verify(proc->counterValue() == 0, F("Should be 0, not -1"));
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  TestFunction tests[] = {
    testDependencyIsInjected,
    testSharedServiceState,
    testAddOrderDoesNotMatter
  };

  runTestSuite(tests);
}

void loop() {}
