#include <Crankstart.h>
#include <TestTool.h>
#include "CrankstartTestHelper.h"

using namespace crankstart;

void testBasicModuleLifecycle(TestInvocation* t) {
  t->setName(F("Basic module lifecycle"));
  
  SimpleTestConfig config;
  App app(config);
  t->verify(app.start(), F("App should start"));

  // Test that modules were all initialized
  t->verify(config.moduleA.initialized, F("Module A should be initialized"));
  t->verify(config.moduleB.initialized, F("Module B should be initialized"));
  t->verify(config.moduleC.initialized, F("Module C should be initialized"));
  
  // Test setup lifecycle
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
  
  // Test loop lifecycle
  app.loop();
  
  t->verify(config.moduleA.loopCalled, F("Module A loop should be called"));
  t->verify(config.moduleB.loopCalled, F("Module B loop should be called"));
  t->verify(config.moduleC.loopCalled, F("Module C loop should be called"));
  
  // Test shutdown lifecycle
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
  
  // Test that modules were all initialized
  t->verify(config.moduleA.initialized, F("Module A should be initialized"));
  t->verify(config.moduleB.initialized, F("Module B should be initialized"));
  t->verify(config.moduleC.initialized, F("Module C should be initialized"));

  // Test that module C uses the MyService instance provided by module A
  t->verify(config.moduleC.anotherService->getValueViaAnotherService() == 42, 
      F("Expected 42 from MyService instance provided by module A"));
}

void testCircularDependency(TestInvocation* t) {
  t->setName(F("Circular dependency"));
  
  CircularDependencyTestConfig config;
  App app(config);

  // // Should have printed a FATAL message
  t->verify(!app.start(), F("App should not have started"));
}

void testDuplicateModule(TestInvocation* t) {
  t->setName(F("Duplicate module names"));
  
  DuplicateModuleTestConfig config;
  App app(config);

  // Should have printed a FATAL message
  t->verify(!app.start(), F("App should not have started"));
}

void testUnsatisfiedDependency(TestInvocation* t) {
  t->setName(F("Unsatisfied dependency"));
  
  UnsatisfiedDependencyTestConfig config;
  App app(config);

  // Should have printed a FATAL message
  t->verify(!app.start(), F("App should not have started"));
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  TestFunction tests[] = {
    testBasicModuleLifecycle,
    testServiceDependencyInjection,
    testCircularDependency,
    testDuplicateModule,
    testUnsatisfiedDependency
  };

  runTestSuiteShowMem(tests);
}

void loop() {}
