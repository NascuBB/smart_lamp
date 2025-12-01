#ifndef SimpleOTA_H
#define SimpleOTA_H

#include <Arduino.h>
#include <functional>

// callback funkrion on percentage
typedef std::function<void(int)> ProgressCallback;

bool performOTA(const char* firmwareURL, ProgressCallback onProgress = nullptr);

bool isUpdateAvailable(const char* versionURL, const char* currentVersion);

#endif