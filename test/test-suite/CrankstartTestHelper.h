#ifndef _crankstart_test_CrankstartTestHelper_h
#define _crankstart_test_CrankstartTestHelper_h

#include <stdint.h>
#include <Crankstart.h>

using namespace crankstart;

static const char MODULE_A[] PROGMEM = "a";
static const char MODULE_B[] PROGMEM = "b";
static const char MODULE_C[] PROGMEM = "c";
static const char MODULE_D[] PROGMEM = "d";


// Create a couple of service classes for dependency injection testing.
// The AnotherService class depends on the MyService class, and each
// is declared in its respective module class.
class MyService {
  public:
    MyService() {};
    uint8_t getValue() { return value; }
  private:
    uint8_t value = 42;
};

class AnotherService {
  public:
    AnotherService(MyService* myService): _myService(myService) {};
    uint8_t getValueViaAnotherService() { return _myService->getValue(); };
  private:
    MyService* _myService;
};

// Test modules for dependency injection testing. TestModuleA has no
// dependencies, but provides a MyService instance.
class TestModuleA : public Module, public Provider<MyService> {

  public:
    const char* name() const override { return MODULE_A; }
    // no dependencies
    
    bool initialized = false;
    bool preSetupCalled = false;
    bool setupCalled = false;
    bool postSetupCalled = false;
    bool loopCalled = false;
    bool shutdownCalled = false;
    void initialize() override { 
      _myService = new MyService();
      initialized = true; 
    }
    bool preSetup() override { preSetupCalled = true; return true; }
    bool setup() override { setupCalled = true; return true; }
    void postSetup() override { postSetupCalled = true; }
    void loop() override { loopCalled = true; }
    void shutdown() override { shutdownCalled = true; }

    MyService* get() override { return _myService; }
    ~TestModuleA() {
      // Always clean up what you new'ed up in the initialize() method
      if (_myService) delete _myService;
    }
  private:
    MyService* _myService = nullptr;
};

// TestModuleB has no dependencies and provides nothing. It could be an
// independent service that simply needs lifecycle management.
class TestModuleB : public Module {
  public:
    const char* name() const override { return MODULE_B; }
    // no dependencies

    bool initialized = false;
    bool preSetupCalled = false;
    bool setupCalled = false;
    bool postSetupCalled = false;
    bool loopCalled = false;
    bool shutdownCalled = false;
    void initialize() override { initialized = true; }
    bool preSetup() override { preSetupCalled = true; return true; }
    bool setup() override { setupCalled = true; return true; }
    void postSetup() override { postSetupCalled = true; }
    void loop() override { loopCalled = true; }
    void shutdown() override { shutdownCalled = true; }  
};

// TestModuleC manages the AnotherService's lifecycle. It depends on
// TestModuleA in order to access the MyService instance.
class TestModuleC : public Module {
  public:
    const char* name() const override { return MODULE_C; }
    DependsOn deps() const override { 
      static const char* deps[] = { MODULE_A, nullptr };
      return { deps };
    };
  
    AnotherService* anotherService = nullptr;
    bool initialized = false;
    bool preSetupCalled = false;
    bool setupCalled = false;
    bool postSetupCalled = false;
    bool loopCalled = false;
    bool shutdownCalled = false;
    void initialize() override {
      TestModuleA* testModuleA = static_cast<TestModuleA*>(app->get(MODULE_A));
      MyService* myService = testModuleA->get();
      anotherService = new AnotherService(myService);
      initialized = true; 
    }
    bool preSetup() override { preSetupCalled = true; return true; }
    bool setup() override { setupCalled = true; return true; }
    void postSetup() override { postSetupCalled = true; }
    void loop() override { loopCalled = true; }
    void shutdown() override { shutdownCalled = true; }
    ~TestModuleC() {
      // Always clean up what you new'ed up in the initialize() method
      if (anotherService) delete anotherService;
    };
};

// This alternate version of TestModuleA depends on TestModuleC, which will
// create a circular dependency for testing purposes.
class AlternateModuleA : public Module {

  public:
    const char* name() const override { return MODULE_A; }
    DependsOn deps() const override { 
      static const char* deps[] = { MODULE_C, nullptr };
      return { deps };
    };
    bool initialized = false;
    bool preSetupCalled = false;
    bool setupCalled = false;
    bool postSetupCalled = false;
    bool loopCalled = false;
    bool shutdownCalled = false;
    void initialize() override { initialized = true; }
    bool preSetup() override { preSetupCalled = true; return true; }
    bool setup() override { setupCalled = true; return true; }
    void postSetup() override { postSetupCalled = true; }
    void loop() override { loopCalled = true; }
    void shutdown() override { shutdownCalled = true; }
};


// A basic working configuration using modules A, B, and C.
class SimpleTestConfig : public AppConfig {
  public:
    TestModuleA moduleA;
    TestModuleB moduleB;
    TestModuleC moduleC;
    void configure() override {
      addModule(&moduleA);
      addModule(&moduleB);
      addModule(&moduleC);
    }
};

// This configuration creates a circular dependency between 
// AlternateModuleA and TestModuleC.
class CircularDependencyTestConfig : public AppConfig {
  public:
    AlternateModuleA moduleA;
    TestModuleC moduleC;
    void configure() override {
      addModule(&moduleA);
      addModule(&moduleC);
    }
};

// This configures two modules with the same name, which should cause a 
// fatal error during startup.
class DuplicateModuleTestConfig : public AppConfig {
  public:
    TestModuleA moduleA;
    AlternateModuleA moduleA2;
    void configure() override {
      addModule(&moduleA);
      addModule(&moduleA2);
    }
};

// This configures a module with an unsatisfied dependency (module C depends 
// on module A, but module A is not added to the config). This should cause a 
// fatal error during startup.
class UnsatisfiedDependencyTestConfig : public AppConfig {
  public:
    TestModuleB moduleB;
    TestModuleC moduleC;
    void configure() override {
      addModule(&moduleB);
      addModule(&moduleC);
    }
};

#endif
