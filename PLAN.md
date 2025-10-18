Crankstart is a stripped-down dependency injection framework for wiring up interdependent modules in complex Arduino projects. The user will supply an instance of crankstart::AppConfig, which contains a listing of Modules that are to be instantiated with their declared dependencies. These can be in any order, and may implement lifecycle hooks for setup() and loop().

Anywhere in the project, the user must implement 1 or (probably) more types extending Module. A Module declaration will give the module a name, indicate what interface it provides, hold a pointer to an implementation of that interface, and a list of names of other Modules that that this one depends on.

In the sketch's .ino file, the user will instantiate the app something like this:

#if (defined(SIMULATOR))
  SimulatorAppConfig config;
#else
  ProdAppConfig config;
#endif

crankstart::App app(config);

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}

The user can have different crankstart::AppConfig versions for different environments, or ones that provide fakes of a module for testing purposes.

When the crankstart::App class is instantiated, it will in turn instantiate all the Modules in the config in topo order. Only wiring will happen at this stage. The App's setup() method will execute all the Modules' setup() callback hooks allowing them to "get" other modules and do things like register handlers as well as calling setup on other internal pieces. Finally, the App's loop() method will invoke the Modules' loop() callback hooks.

Error handling:
During instantiation of the crankstart::App, if dependency resolution fails, a circular dependency is discovered, or any module instantiation fails, an error message will be printed when app.setup() is called (if DEBUG is set) and no modules will be initialized. It's up to the user if they want to handle this gracefully.

Modules accessing other modules during setup():
Every module has a static const char* name (ideally in PROGMEM). Modules will have to implement a method that returns the module's name, and that name will only be compared by pointer equality. Modules will declare their dependencies by these names, and the specific Module can be retrieved by looping over an array looking for one with the right name. If the Module implementation also extends crankstart::Provider<T>, it will have a get() method that returns a pointer to the underlying instance of T.