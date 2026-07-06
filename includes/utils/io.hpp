#pragma once
#include <expected>
#include <fstream>
#include <string>
#include <vector>

namespace ob::io {
/**
 * @brief Streams a binary file directly into a provided memory buffer.
 * @param path The absolute or relative filesystem path to the asset.
 * @param outBuffer The target container reference (zero-allocation if
 * pre-reserved).
 */
template <typename T>
[[nodiscard]] inline std::expected<void, std::string>
ReadBinaryFile(const std::string &path, std::vector<T> &outBuffer) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    return std::unexpected("IO Error: Failed to open file context at " + path);
  }

  const std::streamsize fileSize = file.tellg();
  if (fileSize <= 0) {
    return std::unexpected("IO Error: File is empty or corrupted: " + path);
  }

  if (fileSize % sizeof(T) != 0) {
    return std::unexpected(
        "IO Error: File size is not a multiple of the target type alignment: " +
        path);
  }

  outBuffer.resize(static_cast<size_t>(fileSize) / sizeof(T));
  file.seekg(0, std::ios::beg);

  if (!file.read(reinterpret_cast<char *>(outBuffer.data()), fileSize)) {
    return std::unexpected("IO Error: Direct memory streaming failed for " +
                           path);
  }

  return {};
}

inline std::string ResolveAssetPath(const std::string &relativePath) {
#ifdef OBLIQUE_PROD_BUILD
  return (std::filesystem::current_path() / "assets" / relativePath).string();
#else
  return std::string(OBLIQUE_PROJECT_ROOT) + "assets/" + relativePath;
#endif
}

} // namespace ob::io
