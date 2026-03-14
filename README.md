# Crankstart

A lightweight dependency injection framework for Arduino.

Written by Dan Mowehhuk (danmowehhuk@gmail.com)\
BSD license, check license.txt for more information\
All text above must be included in any redistribution

## Overview

Crankstart provides a structured way to wire up interdependent components in complex Arduino
projects. You define **Modules** — self-contained components with lifecycle hooks — declare
their dependencies, and let Crankstart initialize them in the correct order.

A project can have multiple `AppConfig` subclasses — for example, one for production and
one for testing with mock modules — making it easy to swap implementations without touching
application logic.

## Core Concepts

| Class | Role |
|---|---|
| `Module` | A named component with lifecycle hooks |
| `AppConfig` | Declares which modules make up an application |
| `App` | Drives the lifecycle of all modules |
| `Provider<T>` | Mixin for modules that expose a service to other modules |

## Setup in a Sketch

```cpp
#include <Crankstart.h>

MyAppConfig config;
crankstart::App app(config);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!app.start()) {
    // dependency resolution failed — Serial output above will show the cause
    while(1);
  }
  app.setup();
}

void loop() {
  app.loop();
}
```

## Module Lifecycle

`app.start()` calls `configure()` on the AppConfig, resolves dependencies, and calls
`initialize()` on each module in dependency order. The remaining lifecycle hooks fire
when `app.setup()` and `app.loop()` are called:

| Hook | Triggered by | Typical use |
|---|---|---|
| `initialize()` | `app.start()` | Allocate services, retrieve dependencies via `app->get()` |
| `preSetup()` | `app.setup()` | Pin modes, clocks, static validation — return `false` to abort |
| `setup()` | `app.setup()` | `begin()`, handshakes, timeouts — return `false` to abort |
| `postSetup()` | `app.setup()` | Logging, cache warming, post-setup actions |
| `loop()` | `app.loop()` | Polling, periodic work |
| `shutdown()` | `app.shutdown()` | Cleanup (optional) |

## Defining a Module

Module instances are held as **member variables of the AppConfig subclass**, not allocated
on the heap. `configure()` registers them with `addModule()`:

```cpp
// Module names must be static const char[] in PROGMEM.
// Crankstart compares names by POINTER EQUALITY, not string value.
// Always use the same constant — never use string literals or F() for names.
static const char MY_MODULE[] PROGMEM = "my-module";

class MyModule : public crankstart::Module {
public:
  const char* name() const override { return MY_MODULE; }

  void initialize() override {
    _service = new MyService();
  }

  bool setup() override {
    return _service->begin();  // return false to abort startup
  }

  void loop() override {
    _service->poll();
  }

  ~MyModule() override {
    if (_service) { delete _service; _service = nullptr; }
  }

private:
  MyService* _service = nullptr;
};

class MyAppConfig : public crankstart::AppConfig {
  MyModule _myModule;
  void configure() override {
    addModule(&_myModule);
  }
};
```

## Declaring Dependencies

Override `deps()` to return a **nullptr-terminated array** of module name pointers.
Crankstart sorts modules topologically, so dependencies are always initialized first
regardless of the order they are added in `configure()`:

```cpp
DependsOn deps() const override {
  static const char* deps[] = { OTHER_MODULE, nullptr };
  return { deps };
}
```

## Accessing Dependencies

In `initialize()`, use `app->get(<name>)` to retrieve a module by name. The return type
is `Module*`; cast to the concrete type with `static_cast`:

```cpp
void initialize() override {
  OtherModule* other = static_cast<OtherModule*>(app->get(OTHER_MODULE));
  _service = new MyService(other->get());
}
```

> **Warning:** `app->get()` returns `Module*`. Casting to the wrong concrete type will
> compile but crash at runtime. Make sure the cast matches the module registered for
> that name in your AppConfig.

## Providing a Service via `Provider<T>`

Modules that expose a service to other modules should inherit from both `Module` and
`Provider<T>` and implement `get()`:

```cpp
class StorageModule : public crankstart::Module,
                      public crankstart::Provider<Storage> {
public:
  const char* name() const override { return STORAGE_MODULE; }
  void initialize() override { _storage = new EepromStorage(); }
  Storage* get() override { return _storage; }

  ~StorageModule() override {
    if (_storage) { delete _storage; _storage = nullptr; }
  }

private:
  Storage* _storage = nullptr;
};
```

Other modules retrieve the service by casting to the provider type:

```cpp
void initialize() override {
  StorageModule* storageMod =
    static_cast<StorageModule*>(app->get(STORAGE_MODULE));
  _service = new MyService(storageMod->get());
}
```

## Alternate AppConfig Implementations

Different AppConfig subclasses can register different module implementations under the
same module name. This is useful for testing, simulation, or environment-specific builds.
Application logic modules are unchanged — only the AppConfig differs:

```cpp
// Production: real hardware
class ProdAppConfig : public crankstart::AppConfig {
  RealStorageModule _storage;
  AppLogicModule    _logic;
  void configure() override {
    addModule(&_storage);
    addModule(&_logic);
  }
};

// Testing: mock storage, same app logic
class TestAppConfig : public crankstart::AppConfig {
  MockStorageModule _storage;
  AppLogicModule    _logic;
  void configure() override {
    addModule(&_storage);
    addModule(&_logic);
  }
};
```

Select at compile time in the sketch:

```cpp
#if defined(TEST_ENV)
  TestAppConfig config;
#else
  ProdAppConfig config;
#endif
crankstart::App app(config);
```

For modules that need to access a swappable dependency, declare a common abstract base
that both implementations inherit from, and cast to that base:

```cpp
// Both RealStorageModule and MockStorageModule extend this
class StorageModuleBase : public crankstart::Module,
                          public crankstart::Provider<Storage> {
protected:
  StorageModuleBase() = default;
};

// AppLogicModule casts to StorageModuleBase — works with either implementation
void initialize() override {
  StorageModuleBase* storageMod =
    static_cast<StorageModuleBase*>(app->get(STORAGE_MODULE));
  _service = new MyService(storageMod->get());
}
```

## Error Handling

If `app.start()` returns `false`, dependency resolution failed. With `DEBUG` defined,
Crankstart prints the cause to Serial:

- **Duplicate module names** — two modules registered with the same name
- **Circular dependencies** — module A depends on B which depends on A
- **Missing dependencies** — a declared dependency has no matching module

## Examples

The `examples/test/` directory contains focused examples, each verified with the
[TestTool](https://github.com/danmowehhuk/TestTool) framework:

- `basic/` — Module lifecycle hooks and AppConfig setup
- `dependencies/` — `Provider<T>`, `DependsOn`, and dependency injection
- `alternate-configs/` — Swapping implementations between production and test configs
