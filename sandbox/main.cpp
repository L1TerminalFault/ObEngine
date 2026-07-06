#include <engine/engine.hpp>
#include <iostream>

int main() {
  ob::Engine engine;
  auto initResult = engine.init();
  if (!initResult.has_value()) {
    std::cerr << "Engine Initialization Aborted: " << initResult.error()
              << "\n";
    return EXIT_FAILURE;
  }
  engine.run();
  return EXIT_SUCCESS;
}
