#include "../../include/utils/ResolutionManager.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>

// Resolution presets from 144p to 4K
const Resolution ResolutionManager::PRESETS[ResolutionManager::NUM_PRESETS] = {
  {256, 144, "144p"},             // 256×144
  {426, 240, "240p"},             // 426×240
  {640, 360, "360p"},             // 640×360
  {854, 480, "480p"},             // 854×480
  {1280, 720, "720p HD"},         // 1280×720
  {1920, 1080, "1080p FHD"},      // 1920×1080
  {2560, 1440, "1440p QHD"},      // 2560×1440
  {2880, 1620, "1620p"},          // 2880×1620
  {3840, 2160, "2160p 4K"},       // 3840×2160
  {5120, 2880, "2880p 5K"},      // 5120×2880
  {7680, 4320, "4320p 8K"},      // 7680×4320
};

ResolutionManager::ResolutionManager() : currentIndex(5) { // Default to 1080p
  loadResolution(); // Load saved resolution if available
}

void ResolutionManager::next() {
  currentIndex = (currentIndex + 1) % NUM_PRESETS;
}

void ResolutionManager::previous() {
  currentIndex = (currentIndex - 1 + NUM_PRESETS) % NUM_PRESETS;
}

void ResolutionManager::setResolution(int index) {
  if (index >= 0 && index < NUM_PRESETS) {
    currentIndex = index;
  }
}

int ResolutionManager::findClosestPreset(int width, int height) const {
  int closestIndex = 0;
  int minDiff = std::numeric_limits<int>::max();
  
  for (int i = 0; i < NUM_PRESETS; i++) {
    int diff = std::abs(PRESETS[i].width - width) + std::abs(PRESETS[i].height - height);
    if (diff < minDiff) {
      minDiff = diff;
      closestIndex = i;
    }
  }
  
  return closestIndex;
}

void ResolutionManager::saveResolution() const {
  // Get home directory
  const char* home = std::getenv("HOME");
  if (!home) {
    return; // Can't save without home directory
  }
  
  std::string configPath = std::string(home) + "/" + CONFIG_FILE;
  std::ofstream file(configPath);
  if (file.is_open()) {
    file << currentIndex << std::endl;
    file.close();
  }
}

void ResolutionManager::loadResolution() {
  // Get home directory
  const char* home = std::getenv("HOME");
  if (!home) {
    return; // Can't load without home directory
  }
  
  std::string configPath = std::string(home) + "/" + CONFIG_FILE;
  std::ifstream file(configPath);
  if (file.is_open()) {
    int savedIndex;
    file >> savedIndex;
    if (savedIndex >= 0 && savedIndex < NUM_PRESETS) {
      currentIndex = savedIndex;
    }
    file.close();
  }
}

