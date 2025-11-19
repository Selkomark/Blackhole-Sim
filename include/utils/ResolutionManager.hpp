#pragma once

/**
 * Resolution preset structure
 */
struct Resolution {
  int width;
  int height;
  const char* name;
};

/**
 * Manages resolution presets and selection
 */
class ResolutionManager {
public:
  // Common resolutions from 144p to 4K
  static constexpr int NUM_PRESETS = 11;
  static const Resolution PRESETS[NUM_PRESETS];
  
  ResolutionManager();
  
  // Get current resolution
  const Resolution& getCurrent() const { return PRESETS[currentIndex]; }
  
  // Get current index
  int getCurrentIndex() const { return currentIndex; }
  
  // Cycle to next resolution
  void next();
  
  // Cycle to previous resolution
  void previous();
  
  // Set resolution by index
  void setResolution(int index);
  
  // Find closest preset to given dimensions
  int findClosestPreset(int width, int height) const;
  
  // Get resolution name
  const char* getCurrentName() const { return PRESETS[currentIndex].name; }
  
  // Save current resolution to file
  void saveResolution() const;
  
  // Load resolution from file
  void loadResolution();

private:
  int currentIndex;
  static constexpr const char* CONFIG_FILE = ".blackhole_resolution";
};

