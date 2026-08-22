/*
 * basic — Module lifecycle hooks and AppConfig setup
 *
 * Demonstrates:
 *   - Defining a Module with lifecycle hooks
 *   - Holding module instances as AppConfig members
 *   - The order lifecycle hooks are called: initialize -> preSetup ->
 *     setup -> postSetup -> loop -> shutdown
 *   - app.start(), app.didStart(), app.setup(), app.loop(), app.shutdown()
 */

#include <TestTool.h>
#include <Crankstart.h>
#include <string.h>

using namespace crankstart;

// Global arrays tracking which lifecycle hooks fired and in what order.
//   1 = initialize   4 = postSetup
//   2 = preSetup     5 = loop
//   3 = setup        6 = shutdown
static uint8_t gCallOrder[6];
static uint8_t gCallCount = 0;

// Module names must be static const char[] in PROGMEM.
// Crankstart compares names by POINTER EQUALITY, not string value —
// always refer to the name via the same constant, never a string literal.
static const char LOGGER_MODULE[] PROGMEM = "logger";

// A minimal module that records each lifecycle call into gCallOrder.
class LoggerModule : public Module {
public:
  const char* name() const override { return LOGGER_MODULE; }
  void initialize() override { gCallOrder[gCallCount++] = 1; }
  bool preSetup()   override { gCallOrder[gCallCount++] = 2; return true; }
  bool setup()      override { gCallOrder[gCallCount++] = 3; return true; }
  void postSetup()  override { gCallOrder[gCallCount++] = 4; }
  void loop()       override { gCallOrder[gCallCount++] = 5; }
  void shutdown()   override { gCallOrder[gCallCount++] = 6; }
};

// AppConfig subclass holds module instances as member variables.
// configure() registers them via addModule(). Modules are destroyed
// automatically when the AppConfig is destroyed.
class BasicAppConfig : public AppConfig {
  LoggerModule _logger;
  void configure() override {
    addModule(&_logger);
  }
};

// Reset global tracking state before each test.
void before() {
  gCallCount = 0;
  memset(gCallOrder, 0, sizeof(gCallOrder));
}

void testStartCallsInitialize(TestInvocation* t) {
  t->setName(F("start() calls initialize() and returns true"));
  BasicAppConfig config;
  App app(config);
  t->verify(app.start(), F("start() should return true"));
  t->verify(app.didStart(), F("didStart() should be true after start()"));
  t->verify(gCallCount == 1, F("initialize() should be called once"));
  t->verify(gCallOrder[0] == 1, F("initialize() should be the first call"));
}

void testSetupLifecycleOrder(TestInvocation* t) {
  t->setName(F("setup() calls preSetup -> setup -> postSetup in order"));
  BasicAppConfig config;
  App app(config);
  app.start();
  app.setup();
  t->verify(gCallCount == 4, F("Should have 4 lifecycle calls total"));
  t->verify(gCallOrder[1] == 2, F("preSetup() should be second"));
  t->verify(gCallOrder[2] == 3, F("setup() should be third"));
  t->verify(gCallOrder[3] == 4, F("postSetup() should be fourth"));
}

void testLoopCallsModuleLoop(TestInvocation* t) {
  t->setName(F("loop() calls module loop()"));
  BasicAppConfig config;
  App app(config);
  app.start();
  app.setup();
  app.loop();
  t->verify(gCallCount == 5, F("Should have 5 lifecycle calls total"));
  t->verify(gCallOrder[4] == 5, F("loop() should be fifth"));
}

void testShutdownCallsModuleShutdown(TestInvocation* t) {
  t->setName(F("shutdown() calls module shutdown()"));
  BasicAppConfig config;
  App app(config);
  app.start();
  app.shutdown();
  // After start(): initialize(1). After shutdown(): shutdown(6).
  t->verify(gCallCount == 2, F("Should have 2 calls: initialize then shutdown"));
  t->verify(gCallOrder[1] == 6, F("shutdown() should follow initialize()"));
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  TestFunction tests[] = {
    testStartCallsInitialize,
    testSetupLifecycleOrder,
    testLoopCallsModuleLoop,
    testShutdownCallsModuleShutdown
  };

  runTestSuite(tests, before, nullptr);
}

void loop() {}
