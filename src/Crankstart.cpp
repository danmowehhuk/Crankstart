#include <stdint.h>
#include "Crankstart.h"
#include "hal/CrankstartHal.h"

namespace crankstart {

namespace {

// Pointer equality first, since it's fast; then the names' text, for a
// name that's defined in more than one place.
bool namesMatch(const char* a, const char* b) {
  return a == b || (a && b && CrankstartHal::flashStrEqual(a, b));
}

#if (defined(DEBUG))
void printModuleError(const FlashStr* before, const char* name) {
  CrankstartHal::print(before);
  if (name) CrankstartHal::print(reinterpret_cast<const FlashStr*>(name));
  CrankstartHal::println(F("'"));
  CrankstartHal::delay(100); // Allow message to print before potentially crashing
}
#endif

}

AppConfig::~AppConfig() {
  delete[] modules;
  modules = nullptr;
}

bool AppConfig::start(App* app) {
  configure();
  if (!sortModulesTopologically()) {
    return false;
  }
  for (size_t i = 0; i < numModules; i++) {
    modules[i]->app = app;
    modules[i]->initialize();
  }
  return true;
}

bool AppConfig::noDuplicatesCheck() {
  // Check for duplicate module names
  for (size_t i = 0; i < numModules; i++) {
    for (size_t j = i + 1; j < numModules; j++) {
      if (namesMatch(modules[i]->name(), modules[j]->name())) {
#if (defined(DEBUG))
        printModuleError(F("FATAL: Multiple modules with name '"), modules[i]->name());
#endif
        return false;
      }
    }
  }
  return true;
}

void AppConfig::addModule(Module* module) {
  // Reallocate array with one more module
  Module** newModules = new Module*[numModules + 1];
  
  // Copy existing modules
  for (size_t i = 0; i < numModules; i++) {
    newModules[i] = modules[i];
  }
  
  // Add new module
  newModules[numModules] = module;
  
  // Clean up old array and update
  delete[] modules;
  modules = newModules;
  numModules++;
}

bool AppConfig::sortModulesTopologically() {
  if (!noDuplicatesCheck()) {
    return false;
  }

  // Create arrays for sorting
  Module** sorted = new Module*[numModules];
  bool* visited = new bool[numModules];
  bool* inStack = new bool[numModules];
  size_t sortedIndex = 0;
  
  // Initialize arrays
  for (size_t i = 0; i < numModules; i++) {
    visited[i] = false;
    inStack[i] = false;
  }
  
  // Perform topological sort using DFS
  for (size_t i = 0; i < numModules; i++) {
    if (!visited[i]) {
      VisitResult result = visitModule(i, modules, sorted, visited, inStack, sortedIndex);
      if (result != VISIT_SUCCESS) {
        if (result == VISIT_CIRCULAR_DEPENDENCY) {
#if (defined(DEBUG))
          CrankstartHal::println(F("FATAL: Circular dependencies detected"));
          CrankstartHal::delay(100); // Allow message to print before potentially crashing
#endif
        }
        delete[] sorted;
        delete[] visited;
        delete[] inStack;
        return false;
      }
    }
  }
  
  // Copy sorted modules back to our array
  for (size_t i = 0; i < numModules; i++) {
    modules[i] = sorted[i];
  }
  
  delete[] sorted;
  delete[] visited;
  delete[] inStack;
  return true;
}

AppConfig::VisitResult AppConfig::visitModule(size_t index, Module** configModules, Module** sorted, bool* visited, bool* inStack, size_t& sortedIndex) {
  if (inStack[index]) {
    return VISIT_CIRCULAR_DEPENDENCY; // Circular dependency detected
  }
  
  if (visited[index]) {
    return VISIT_SUCCESS; // Already processed
  }
  
  inStack[index] = true;
  visited[index] = true;

  // Get dependencies for this module
  DependsOn deps = configModules[index]->deps();
  if (deps.names != nullptr) {

    // Visit all dependencies first
    for (const char* const* depName = deps.names; *depName != nullptr; depName++) {
      Module* dep = findModule(*depName, configModules);
      if (dep == nullptr) {
        // Dependency not found
#if (defined(DEBUG))
        printModuleError(F("FATAL: Unsatisfied dependency on module with name '"), *depName);
#endif
        return VISIT_MISSING_DEPENDENCY;
      }
      
      // Find the index of the dependency
      size_t depIndex = findModuleIndex(dep, configModules);
      if (depIndex == SIZE_MAX) {
        return VISIT_MISSING_DEPENDENCY;
      }

      // Recursively visit dependency
      VisitResult result = visitModule(depIndex, configModules, sorted, visited, inStack, sortedIndex);
      if (result != VISIT_SUCCESS) {
        return result;
      }
    }
  }
  
  inStack[index] = false;
  
  // Add this module to sorted array
  sorted[sortedIndex++] = configModules[index];
  return VISIT_SUCCESS;
}

Module* AppConfig::findModule(const char* name, Module** configModules) {
  for (size_t i = 0; i < numModules; i++) {
    if (namesMatch(configModules[i]->name(), name)) {
      return configModules[i];
    }
  }
  return nullptr;
}

size_t AppConfig::findModuleIndex(Module* module, Module** configModules) {
  for (size_t i = 0; i < numModules; i++) {
    if (configModules[i] == module) {
      return i;
    }
  }
  return SIZE_MAX;
}

bool App::start() {
  _didStart = config.start(this);
  return _didStart;
}

void App::setup() {
  for (size_t i = 0; i < config.numModules; i++) {
    if (!config.modules[i]->preSetup()) {
#if (defined(DEBUG))
      printModuleError(F("ERROR: preSetup() failed for module with name '"),
                       config.modules[i]->name());
#endif
      return;
    }
  }
  for (size_t i = 0; i < config.numModules; i++) {
    if (!config.modules[i]->setup()) {
#if (defined(DEBUG))
      printModuleError(F("ERROR: setup() failed for module with name '"),
                       config.modules[i]->name());
#endif
      return;
    }
  }
  for (size_t i = 0; i < config.numModules; i++) {
    config.modules[i]->postSetup();
  }
}

void App::loop() {
  for (size_t i = 0; i < config.numModules; i++) {
    config.modules[i]->loop();
  }
}

void App::shutdown() {
  for (size_t i = 0; i < config.numModules; i++) {
    config.modules[i]->shutdown();
  }
}

Module* App::get(const char* name) {
  for (size_t i = 0; i < config.numModules; i++) {
    if (namesMatch(config.modules[i]->name(), name)) {
      return config.modules[i];
    }
  }
#if (defined(DEBUG))
  printModuleError(F("ERROR: get() found no module with name '"), name);
#endif
  return nullptr;
}

}

