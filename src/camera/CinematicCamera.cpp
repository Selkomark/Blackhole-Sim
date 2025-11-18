#include "../../include/camera/CinematicCamera.hpp"
#include <SDL2/SDL.h>
#include <cmath>

CinematicCamera::CinematicCamera(Camera &camera, const Vector3 &initialPosition)
    : cam(camera), initialPos(initialPosition), mode(CinematicMode::Manual),
      orbitAngle(0.0), orbitRadius(15.0), cinematicTime(0.0),
      rotationSpeed(1.0) {} // Start in Manual mode by default

void CinematicCamera::update(double deltaTime, const uint8_t *keyStates) {
  // Always advance time, even if deltaTime is small
  cinematicTime += deltaTime;
  
  // Store deltaTime for use in rotation (needed in updateCameraLookDirection)
  // We'll pass it through a member variable or use it directly
  
  // Update camera position based on mode FIRST
  // This ensures position is always updated
  switch (mode) {
    case CinematicMode::Manual:
      updateManualMode(deltaTime, keyStates);
      break;
    case CinematicMode::SmoothOrbit:
      updateSmoothOrbit(deltaTime);
      break;
    case CinematicMode::WaveMotion:
      updateWaveMotion(deltaTime);
      break;
    case CinematicMode::RisingSpiral:
      updateRisingSpiral(deltaTime);
      break;
    case CinematicMode::CloseFlyby:
      updateCloseFlyby(deltaTime);
      break;
  }
  
  // Always update camera look direction after position change
  // This handles rotations incrementally based on current key states
  // Rotations only happen when keys are pressed, stop when released
  updateCameraLookDirection(deltaTime);
}

void CinematicCamera::updateManualMode(double deltaTime, const uint8_t *keyStates) {
  // Store current position for manual movement
  Vector3 currentPos = cam.position;
  Vector3 movement(0, 0, 0);
  
  double moveSpeed = 2.0 * deltaTime; // Reduced from 5.0 to 2.0 for smoother movement
  
  // Controls:
  // W: Move up
  // S: Move down
  // D: Zoom in / Move forward
  // A: Zoom back / Move backward
  
  // Forward/backward movement (zoom)
  if (keyStates[SDL_SCANCODE_D]) {
    movement += cam.forward * moveSpeed; // Zoom in / Move forward
  }
  if (keyStates[SDL_SCANCODE_A]) {
    movement -= cam.forward * moveSpeed; // Zoom back / Move backward
  }
  
  // Up/down movement
  if (keyStates[SDL_SCANCODE_W]) {
    movement += cam.up * moveSpeed; // Move up
  }
  if (keyStates[SDL_SCANCODE_S]) {
    movement -= cam.up * moveSpeed; // Move down
  }
  
  // Apply movement - camera stays where you move it
  // Even if movement is zero, we still update to ensure rendering continues
  cam.position = currentPos + movement;
  
  // Note: Camera look direction is updated after this function returns
  // This ensures smooth rendering even when idle in manual mode
}

void CinematicCamera::updateSmoothOrbit(double deltaTime) {
  orbitAngle += 0.25 * deltaTime;
  orbitRadius = 15.0;
  cam.position.x = std::cos(orbitAngle) * orbitRadius;
  cam.position.z = std::sin(orbitAngle) * orbitRadius;
  cam.position.y = 3.0 + std::sin(orbitAngle * 0.5) * 1.5;
}

void CinematicCamera::updateWaveMotion(double deltaTime) {
  orbitAngle += 0.3 * deltaTime;
  cam.position.x = std::cos(orbitAngle) * 12.0;
  cam.position.z = std::sin(orbitAngle * 2.0) * 8.0; // Figure-8 motion
  cam.position.y = 2.0 + std::sin(orbitAngle * 1.5) * 3.0;
}

void CinematicCamera::updateRisingSpiral(double deltaTime) {
  orbitAngle += 0.35 * deltaTime;
  orbitRadius = 10.0 + std::sin(cinematicTime * 0.3) * 3.0;
  cam.position.x = std::cos(orbitAngle) * orbitRadius;
  cam.position.z = std::sin(orbitAngle) * orbitRadius;
  cam.position.y = 1.0 + cinematicTime * 0.4;
  
  // Reset height periodically
  if (cam.position.y > 8.0) {
    cam.position.y = 1.0;
    cinematicTime = 0.0;
  }
}

void CinematicCamera::updateCloseFlyby(double deltaTime) {
  orbitAngle += 0.5 * deltaTime; // Faster rotation
  orbitRadius = 6.0 + std::sin(orbitAngle * 0.7) * 2.0;
  cam.position.x = std::cos(orbitAngle) * orbitRadius;
  cam.position.z = std::sin(orbitAngle) * orbitRadius;
  cam.position.y = 1.5 + std::cos(orbitAngle * 1.3) * 2.0;
}

void CinematicCamera::cycleMode() {
  int nextMode = (static_cast<int>(mode) + 1) % 5;
  mode = static_cast<CinematicMode>(nextMode);
  cinematicTime = 0.0;
  orbitAngle = 0.0;
  
  // Ensure camera is in valid state when switching modes
  // Force update of camera look direction to prevent invalid state
  updateCameraLookDirection(0.016); // Use typical frame time
}

const char* CinematicCamera::getModeName() const {
  return getCinematicModeName(mode);
}


// Helper function to rotate a vector around an axis using Rodrigues' rotation formula
Vector3 rotateAroundAxis(const Vector3& vec, const Vector3& axis, double angle) {
  if (angle == 0.0 || axis.length() < 0.001) {
    return vec;
  }
  
  Vector3 normalizedAxis = axis.normalized();
  double cosAngle = std::cos(angle);
  double sinAngle = std::sin(angle);
  
  // Rodrigues' rotation formula: v' = v*cos(θ) + (axis × v)*sin(θ) + axis*(axis·v)*(1-cos(θ))
  Vector3 crossProduct = normalizedAxis.cross(vec);
  double dotProduct = normalizedAxis.dot(vec);
  
  return vec * cosAngle + crossProduct * sinAngle + normalizedAxis * dotProduct * (1.0 - cosAngle);
}

void CinematicCamera::updateCameraLookDirection(double deltaTime) {
  // Get keyboard state for rotation (applied incrementally each frame)
  const Uint8 *keyStates = SDL_GetKeyboardState(nullptr);
  double rotSpeed = rotationSpeed * deltaTime; // Use actual deltaTime for smooth rotation
  
  Vector3 currentForward, currentRight, currentUp;
  
  // In cinematic modes (non-manual), always point camera at black hole center
  // In manual mode, preserve current orientation unless vectors are invalid
  if (mode != CinematicMode::Manual) {
    // Cinematic modes: always look at black hole center
    Vector3 toCenter = Vector3(0, 0, 0) - cam.position;
    double distance = toCenter.length();
    
    if (distance < 0.001) {
      // Too close, use default forward
      cam.lookAt(Vector3(0, 0, 0));
      return;
    }
    
    currentForward = toCenter.normalized();
    Vector3 worldUp(0, 1, 0);
    currentRight = currentForward.cross(worldUp).normalized();
    if (currentRight.length() < 0.001) {
      currentRight = currentForward.cross(Vector3(1, 0, 0)).normalized();
    }
    currentUp = currentRight.cross(currentForward).normalized();
  } else {
    // Manual mode: preserve current orientation, only recalculate if invalid
    currentForward = cam.forward;
    currentRight = cam.right;
    currentUp = cam.up;
    
    // Only recalculate if basis vectors are invalid
    if (currentForward.length() < 0.001 || currentRight.length() < 0.001 || currentUp.length() < 0.001) {
      // Recalculate from position to center
      Vector3 toCenter = Vector3(0, 0, 0) - cam.position;
      double distance = toCenter.length();
      
      if (distance < 0.001) {
        // Too close, use default forward
        cam.lookAt(Vector3(0, 0, 0));
        return;
      }
      
      currentForward = toCenter.normalized();
      Vector3 worldUp(0, 1, 0);
      currentRight = currentForward.cross(worldUp).normalized();
      if (currentRight.length() < 0.001) {
        currentRight = currentForward.cross(Vector3(1, 0, 0)).normalized();
      }
      currentUp = currentRight.cross(currentForward).normalized();
    }
  }
  
  // Apply rotations incrementally ONLY when keys are pressed
  // Rotations are applied each frame, so they stop when keys are released
  Vector3 rotatedForward = currentForward;
  Vector3 rotatedRight = currentRight;
  Vector3 rotatedUp = currentUp;
  
  // 1. Rotate around Up (blue) axis - L/J keys (swapped)
  // Rotates Forward and Right around Up, Up stays fixed
  if (keyStates[SDL_SCANCODE_L]) {
    rotatedForward = rotateAroundAxis(rotatedForward, rotatedUp, -rotSpeed); // Swapped
    rotatedRight = rotateAroundAxis(rotatedRight, rotatedUp, -rotSpeed);
  }
  if (keyStates[SDL_SCANCODE_J]) {
    rotatedForward = rotateAroundAxis(rotatedForward, rotatedUp, rotSpeed); // Swapped
    rotatedRight = rotateAroundAxis(rotatedRight, rotatedUp, rotSpeed);
  }
  
  // 2. Rotate around Right (green) axis - I/K keys
  // Rotates Forward and Up around Right, Right stays fixed
  if (keyStates[SDL_SCANCODE_I]) {
    rotatedForward = rotateAroundAxis(rotatedForward, rotatedRight, rotSpeed);
    rotatedUp = rotateAroundAxis(rotatedUp, rotatedRight, rotSpeed);
  }
  if (keyStates[SDL_SCANCODE_K]) {
    rotatedForward = rotateAroundAxis(rotatedForward, rotatedRight, -rotSpeed);
    rotatedUp = rotateAroundAxis(rotatedUp, rotatedRight, -rotSpeed);
  }
  
  // 3. Rotate around Forward (red) axis - O/U keys (swapped)
  // Rotates Right and Up around Forward, Forward stays fixed
  if (keyStates[SDL_SCANCODE_O]) {
    rotatedRight = rotateAroundAxis(rotatedRight, rotatedForward, -rotSpeed); // Swapped
    rotatedUp = rotateAroundAxis(rotatedUp, rotatedForward, -rotSpeed);
  }
  if (keyStates[SDL_SCANCODE_U]) {
    rotatedRight = rotateAroundAxis(rotatedRight, rotatedForward, rotSpeed); // Swapped
    rotatedUp = rotateAroundAxis(rotatedUp, rotatedForward, rotSpeed);
  }
  
  // Normalize and ensure orthogonality
  rotatedForward = rotatedForward.normalized();
  rotatedRight = rotatedRight.normalized();
  rotatedUp = rotatedUp.normalized();
  
  // Reconstruct orthogonal basis from forward
  rotatedRight = rotatedForward.cross(rotatedUp).normalized();
  if (rotatedRight.length() < 0.001) {
    // Vectors are parallel, use fallback
    rotatedRight = rotatedForward.cross(Vector3(0, 1, 0)).normalized();
    if (rotatedRight.length() < 0.001) {
      rotatedRight = rotatedForward.cross(Vector3(1, 0, 0)).normalized();
    }
  }
  rotatedUp = rotatedRight.cross(rotatedForward).normalized();
  
  // Update camera basis directly (don't use lookAt as it recalculates)
  cam.forward = rotatedForward;
  cam.right = rotatedRight;
  cam.up = rotatedUp;
}

void CinematicCamera::reset() {
  cam.position = initialPos;
  orbitAngle = 0.0;
  cinematicTime = 0.0;
  // Reset camera orientation
  cam.lookAt(Vector3(0, 0, 0));
}

const char* getCinematicModeName(CinematicMode mode) {
  switch (mode) {
    case CinematicMode::Manual:
      return "Manual Control";
    case CinematicMode::SmoothOrbit:
      return "Smooth Orbit";
    case CinematicMode::WaveMotion:
      return "Wave Motion";
    case CinematicMode::RisingSpiral:
      return "Rising Spiral";
    case CinematicMode::CloseFlyby:
      return "Close Fly-by";
    default:
      return "Unknown";
  }
}

