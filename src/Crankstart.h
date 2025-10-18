#ifndef _crankstart_Crankstart_h
#define _crankstart_Crankstart_h

#include <Arduino.h>
#include "CrankstartDeps.h"

namespace crankstart {

  /*
   * All modules to be used in an AppConfig must inherit from this class and
   * override the name() method to return a static const PROGMEM string.
   * Module names are compared only by pointer equality, so do not use 
   * string literals or the F() macro for names. Instead, always create a static
   * const char[] (ideally PROGMEM) variable to hold the name.
   *
   * Modules can optionally override the deps() method to return a list of
   * module names that the module depends on. This is used to ensure that
   * modules are initialized in the correct order. Note that deps must 
   * refer to module names via the Module::name() method, not string literals
   * or the F() macro.
   *
   * Modules can optionally override the initialize(), preSetup(), setup(),
   * postSetup(), loop(), and shutdown() methods to customize the module's
   * behavior.
   *
   * In the initialize() method, the module can access other modules via
   * the app->get(<name>) method. Crankstart guarantees that any declared 
   * dependencies will be available at the time the module is initialized.
   */
  class Module {
    
    public:
      virtual const char* name() const = 0;
      virtual DependsOn deps() const { return { nullptr }; }
      virtual void initialize() {}               // create/instantiate the module
      virtual bool preSetup() { return true; }   // pinModes, clocks, static checks
      virtual bool setup()    { return true; }   // begin(), handshake, timeouts
      virtual void postSetup() {}                // log, warm caches
      virtual void loop() {}                     // optional
      virtual void shutdown() {}                 // optional
      virtual ~Module() = default;
      
      // Disable moving and copying
      Module(Module&& other) = delete;
      Module& operator=(Module&& other) = delete;
      Module(const Module&) = delete;
      Module& operator=(const Module&) = delete;

    protected:
      Module() = default;
    
      // AppConfig is a friend so it can set the app pointer
      friend class AppConfig;

      // Lets Modules use app->get(<name>) to access other modules
      App* app = nullptr;
  };


  /*
   * AppConfig is the base class for configuring an application. Derived classes
   * must implement the configure() method to add modules to the application.
   *
   * A project may have multiple AppConfig subclasses; for example, one for the
   * main application and others for various test suites.
   */
  class AppConfig {
   
    public:
      AppConfig() = default;
      virtual ~AppConfig();
      
      // Disable moving and copying
      AppConfig(AppConfig&& other) = delete;
      AppConfig& operator=(AppConfig&& other) = delete;
      AppConfig(const AppConfig&) = delete;
      AppConfig& operator=(const AppConfig&) = delete;

    protected:
      virtual void configure() = 0;
      void addModule(Module* module);
      
    private:
      Module** modules = nullptr;
      size_t numModules = 0;
      bool noDuplicatesCheck();
      bool sortModulesTopologically();

      enum VisitResult {
        VISIT_SUCCESS,
        VISIT_CIRCULAR_DEPENDENCY,
        VISIT_MISSING_DEPENDENCY
      };

      VisitResult visitModule(size_t index, Module** configModules, 
                                 Module** sorted, bool* visited, bool* inStack, 
                                 size_t& sortedIndex);
      Module* findModule(const char* name, Module** configModules);      
      size_t findModuleIndex(Module* module, Module** configModules);

      bool start(App* app);
      friend class App;
  };

  /*
   * The App class takes an AppConfig instance and manages the lifecycle of all
   * the modules in the application.
   * 
   * The user must call setup() in the Arduino setup() function and loop() in the
   * Arduino loop() function. shutdown() is optional, but can be called if the 
   * application needs to be shut down gracefully.
   *
   * The get(<name>) method is intended to be used in the initialize() method of
   * modules to access other modules. Crankstart guarantees that any declared
   * dependencies will be available at the time the module is initialized.
   *
   * NOTE: C++ will implicitly cast the Module* returned by get(<name>) to
   * whatever variable type the user assigns it to. If the user assigns it to
   * the incorrect module type, the program will compile but crash at runtime.
   * Be careful to assign the return value of get(<name>) to the correct 
   * module type.
   */
  class App {
    public:
      App(AppConfig& config) : config(config) {};
      ~App() = default;
      bool start();
      bool didStart() const { return _didStart; }
      void setup();
      void loop();
      void shutdown();
      Module* get(const char* name);

      // Disable moving and copying
      App(App&& other) = delete;
      App& operator=(App&& other) = delete;
      App(const App&) = delete;
      App& operator=(const App&) = delete;

    private:
      App() = default;
      AppConfig& config;
      bool _didStart = false;
  };

}


#endif