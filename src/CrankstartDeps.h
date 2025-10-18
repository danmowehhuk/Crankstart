#ifndef _crankstart_CrankstartDeps_h
#define _crankstart_CrankstartDeps_h


namespace crankstart {

  class App; // Forward declaration

  /*
   * Modules can optionally override the deps() method to return a list of
   * module names that the module depends on. This is used to ensure that
   * modules are initialized in the correct order. Note that deps must 
   * refer to module names via the Module::name() method, not string literals
   * or the F() macro.
   */
 struct DependsOn {
    const char* const* names;  
  };

  /*
   * Modules that provide a dependency to other modules must inherit from this class
   * and override the get() method to return the dependency; for example:
   *
   * class MyModule : public Module, public Provider<MyService> {
   *   public:
   *     const char* name() const override { return MY_COMPONENT_NAME; };
   *     void initialize() override {
   *       _myService = new MyService();
   *     };
   *     ...
   *     MyService* get() override { return _myService; };
   *     ...
   *   private:
   *     MyService* _myService = nullptr;
   * };
   *
   * Another module may access this one using the app->get(<name>) method in its
   * initialize() method. Crankstart guarantees that any declared module 
   * dependencies will be available at the time the module is initialized. 
   * For example:
   *
   * class AnotherModule : public Module {
   *   public:
   *     const char* name() const override { return ANOTHER_COMPONENT_NAME; };
   *     DependsOn deps() const override { 
   *       static const char* deps[] = { COMPONENT_C, nullptr }; // note the nullptr terminator
   *       return { deps };
   *     };
   *     void initialize() override {
   *       MyModule* myModule = app->get(MY_COMPONENT_NAME);
   *       MyService* myService = myModule->get();
   *       _someOtherAPI = new SomeOtherAPI(myService);
   *     };
   *     ...
   *   private:
   *     SomeOtherAPI* _someOtherAPI = nullptr;
   * };
   */
  template<class T>
  class Provider {
    public:
      virtual T* get() = 0;
      virtual ~Provider() = default;
  };

}


#endif
