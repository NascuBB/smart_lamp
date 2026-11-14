#ifndef SimpleOTA_H
#define SimpleOTA_H

#include <Arduino.h>
#include <functional>

typedef std::function<void(int percent)> ProgressCallback;

enum class UpdateCheck : uint8_t {
  Error,
  UpToDate,
  Available,
};

UpdateCheck checkForUpdate(const char* latestReleaseUrl, const char* currentVersion);

bool performOTA(const char* firmwareURL, const ProgressCallback& onProgress = nullptr);

int compareVersions(const char* a, const char* b);

#endif
