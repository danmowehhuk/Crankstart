// Bare-metal AVR port of ../test-suite/test-suite.ino - demonstrates
// Crankstart's whole NO_ARDUINO chain behaving correctly under real
// (simulated) hardware, not just linking. Reuses CrankstartTestHelper.h
// unmodified, exactly like the Arduino-branch suite does.

#include <Crankstart.h>
#include <TestTool.h>
#include <BareMetalHAL.h>
#include "../test-suite/CrankstartTestHelper.h"

using namespace crankstart;

void testBasicModuleLifecycle(TestInvocation* t) {
  t->setName(F("Basic module lifecycle"));

  SimpleTestConfig config;
  App app(config);
  t->verify(app.start(), F("App should start"));

  t->verify(config.moduleA.initialized, F("Module A should be initialized"));
  t->verify(config.moduleB.initialized, F("Module B should be initialized"));
  t->verify(config.moduleC.initialized, F("Module C should be initialized"));

  app.setup();

  t->verify(config.moduleA.preSetupCalled, F("Module A preSetup should be called"));
  t->verify(config.moduleA.setupCalled, F("Module A setup should be called"));
  t->verify(config.moduleA.postSetupCalled, F("Module A postSetup should be called"));

  t->verify(config.moduleB.preSetupCalled, F("Module B preSetup should be called"));
  t->verify(config.moduleB.setupCalled, F("Module B setup should be called"));
  t->verify(config.moduleB.postSetupCalled, F("Module B postSetup should be called"));

  t->verify(config.moduleC.preSetupCalled, F("Module C preSetup should be called"));
  t->verify(config.moduleC.setupCalled, F("Module C setup should be called"));
  t->verify(config.moduleC.postSetupCalled, F("Module C postSetup should be called"));

  app.loop();

  t->verify(config.moduleA.loopCalled, F("Module A loop should be called"));
  t->verify(config.moduleB.loopCalled, F("Module B loop should be called"));
  t->verify(config.moduleC.loopCalled, F("Module C loop should be called"));

  app.shutdown();

  t->verify(config.moduleA.shutdownCalled, F("Module A shutdown should be called"));
  t->verify(config.moduleB.shutdownCalled, F("Module B shutdown should be called"));
  t->verify(config.moduleC.shutdownCalled, F("Module C shutdown should be called"));
}

void testServiceDependencyInjection(TestInvocation* t) {
  t->setName(F("Service dependency injection"));

  SimpleTestConfig config;
  App app(config);
  t->verify(app.start(), F("App should start"));

  t->verify(config.moduleA.initialized, F("Module A should be initialized"));
  t->verify(config.moduleB.initialized, F("Module B should be initialized"));
  t->verify(config.moduleC.initialized, F("Module C should be initialized"));

  t->verify(config.moduleC.anotherService->getValueViaAnotherService() == 42,
      F("Expected 42 from MyService instance provided by module A"));
}

void testCircularDependency(TestInvocation* t) {
  t->setName(F("Circular dependency"));

  CircularDependencyTestConfig config;
  App app(config);

  t->verify(!app.start(), F("App should not have started"));
}

void testDuplicateModule(TestInvocation* t) {
  t->setName(F("Duplicate module names"));

  DuplicateModuleTestConfig config;
  App app(config);

  t->verify(!app.start(), F("App should not have started"));
}

void testUnsatisfiedDependency(TestInvocation* t) {
  t->setName(F("Unsatisfied dependency"));

  UnsatisfiedDependencyTestConfig config;
  App app(config);

  t->verify(!app.start(), F("App should not have started"));
}

void testCopiedNameFindsModule(TestInvocation* t) {
  t->setName(F("A separate copy of a name finds the module"));

  t->verify(MODULE_A_COPY != MODULE_A, F("The names should be separate copies"));

  CopiedNameTestConfig config;
  App app(config);
  t->verify(app.start(), F("App should start"));

  t->verify(config.moduleD.myService && config.moduleD.myService->getValue() == 42,
      F("Module D should get module A's service"));
  t->verify(app.get(MODULE_A_COPY) == &config.moduleA, F("get() should find module A"));
}

void testGetMatchesWholeNames(TestInvocation* t) {
  t->setName(F("get() matches whole names only"));

  SimpleTestConfig config;
  App app(config);
  t->verify(app.start(), F("App should start"));

  t->verify(app.get(MODULE_A_LONGER) == nullptr, F("'ab' should not find module 'a'"));
  t->verify(app.get(MODULE_D) == nullptr, F("'d' should find nothing"));
}

void testCopiedDuplicateModule(TestInvocation* t) {
  t->setName(F("Duplicate names in separate copies"));

  CopiedDuplicateTestConfig config;
  App app(config);

  t->verify(!app.start(), F("App should not have started"));
}

void testFailedSetupStopsSetup(TestInvocation* t) {
  t->setName(F("A failed setup() stops the rest of setup"));

  FailingSetupTestConfig config;
  App app(config);
  t->verify(app.start(), F("App should start"));

  app.setup();
  t->verify(config.moduleB.preSetupCalled, F("Module B preSetup should be called"));
  t->verify(!config.moduleB.setupCalled, F("Module B setup should not be called"));
  t->verify(!config.moduleB.postSetupCalled, F("Module B postSetup should not be called"));

  app.loop();
  t->verify(config.moduleB.loopCalled, F("Module B loop should still be called"));
}

int main() {
  BareMetalHAL::Uart0::begin(9600);
  BareMetalHAL::timingInit();

  TestFunction tests[] = {
    testBasicModuleLifecycle,
    testServiceDependencyInjection,
    testCircularDependency,
    testDuplicateModule,
    testUnsatisfiedDependency,
    testCopiedNameFindsModule,
    testGetMatchesWholeNames,
    testCopiedDuplicateModule,
    testFailedSetupStopsSetup
  };

  runTestSuiteShowMem(tests);

  while (true) {}
  return 0;
}
