#include "../../include/camera/CinematicCamera.hpp"
#include <SDL2/SDL.h>
#include <cmath>

CinematicCamera::CinematicCamera(Camera &camera, const Vector3 &initialPosition)
    : cam(camera), initialPos(initialPosition), mode(CinematicMode::Manual),
      orbitAngle(0.0), orbitRadius(15.0), cinematicTime(0.0),
      rotationSpeed(0.3) {} // Start in Manual mode by default - slower rotation

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
  
  // Base movement speed (units per second) - reduced for slower, smoother movement
  const double baseMoveSpeed = 0.8;
  
  // Easing factor for smooth acceleration/deceleration (0.0 = instant, 1.0 = very slow)
  // Higher values = smoother but slower response
  const double moveEasingFactor = 12.0; // Increased for smoother, slower response
  
  // Calculate target movement direction and speed for each axis separately
  // This allows independent easing for each direction
  static double currentSpeedForward = 0.0;
  static double currentSpeedUp = 0.0;
  
  double targetSpeedForward = 0.0;
  double targetSpeedUp = 0.0;
  
  // Forward/backward movement (zoom)
  if (keyStates[SDL_SCANCODE_D]) {
    targetSpeedForward = baseMoveSpeed; // Zoom in / Move forward (positive)
  }
  if (keyStates[SDL_SCANCODE_A]) {
    targetSpeedForward = -baseMoveSpeed; // Zoom back / Move backward (negative)
  }
  
  // Up/down movement
  if (keyStates[SDL_SCANCODE_W]) {
    targetSpeedUp = baseMoveSpeed; // Move up (positive)
  }
  if (keyStates[SDL_SCANCODE_S]) {
    targetSpeedUp = -baseMoveSpeed; // Move down (negative)
  }
  
  // Apply exponential smoothing to each axis independently
  double moveSmoothing = 1.0 - std::exp(-moveEasingFactor * deltaTime);
  currentSpeedForward += (targetSpeedForward - currentSpeedForward) * moveSmoothing;
  currentSpeedUp += (targetSpeedUp - currentSpeedUp) * moveSmoothing;
  
  // Calculate movement vector from eased speeds
  movement = cam.forward * currentSpeedForward * deltaTime;
  movement += cam.up * currentSpeedUp * deltaTime;
  
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
  
  // Easing factor for smooth rotation acceleration/deceleration
  const double rotationEasingFactor = 15.0; // Increased for smoother, slower rotation response
  
  // Base rotation speed (radians per second) - already reduced via rotationSpeed member
  const double baseRotationSpeed = rotationSpeed;
  
  // Calculate target rotation speed based on key presses
  // We'll track rotation in each axis separately for smoother control
  double targetRotSpeedUp = 0.0;
  double targetRotSpeedRight = 0.0;
  double targetRotSpeedForward = 0.0;
  
  // 1. Rotate around Up (blue) axis - L/J keys
  if (keyStates[SDL_SCANCODE_L]) {
    targetRotSpeedUp -= baseRotationSpeed; // Negative rotation
  }
  if (keyStates[SDL_SCANCODE_J]) {
    targetRotSpeedUp += baseRotationSpeed; // Positive rotation
  }
  
  // 2. Rotate around Right (green) axis - I/K keys
  if (keyStates[SDL_SCANCODE_I]) {
    targetRotSpeedRight += baseRotationSpeed;
  }
  if (keyStates[SDL_SCANCODE_K]) {
    targetRotSpeedRight -= baseRotationSpeed;
  }
  
  // 3. Rotate around Forward (red) axis - O/U keys
  if (keyStates[SDL_SCANCODE_O]) {
    targetRotSpeedForward -= baseRotationSpeed; // Swapped
  }
  if (keyStates[SDL_SCANCODE_U]) {
    targetRotSpeedForward += baseRotationSpeed; // Swapped
  }
  
  // Apply exponential smoothing to rotation speeds
  // Store current rotation speeds as static variables for persistence
  static double currentRotSpeedUp = 0.0;
  static double currentRotSpeedRight = 0.0;
  static double currentRotSpeedForward = 0.0;
  
  double rotSmoothing = 1.0 - std::exp(-rotationEasingFactor * deltaTime);
  currentRotSpeedUp += (targetRotSpeedUp - currentRotSpeedUp) * rotSmoothing;
  currentRotSpeedRight += (targetRotSpeedRight - currentRotSpeedRight) * rotSmoothing;
  currentRotSpeedForward += (targetRotSpeedForward - currentRotSpeedForward) * rotSmoothing;
  
  // Convert to rotation angle for this frame
  double rotSpeedUp = currentRotSpeedUp * deltaTime;
  double rotSpeedRight = currentRotSpeedRight * deltaTime;
  double rotSpeedForward = currentRotSpeedForward * deltaTime;
  
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
  
  // Apply rotations incrementally with eased speeds
  // Rotations are applied each frame with smooth acceleration/deceleration
  Vector3 rotatedForward = currentForward;
  Vector3 rotatedRight = currentRight;
  Vector3 rotatedUp = currentUp;
  
  // Apply rotations only if there's actual rotation speed (smoothly decelerates to zero)
  // 1. Rotate around Up (blue) axis - L/J keys
  if (std::abs(rotSpeedUp) > 0.0001) {
    rotatedForward = rotateAroundAxis(rotatedForward, rotatedUp, rotSpeedUp);
    rotatedRight = rotateAroundAxis(rotatedRight, rotatedUp, rotSpeedUp);
  }
  
  // 2. Rotate around Right (green) axis - I/K keys
  if (std::abs(rotSpeedRight) > 0.0001) {
    rotatedForward = rotateAroundAxis(rotatedForward, rotatedRight, rotSpeedRight);
    rotatedUp = rotateAroundAxis(rotatedUp, rotatedRight, rotSpeedRight);
  }
  
  // 3. Rotate around Forward (red) axis - O/U keys
  if (std::abs(rotSpeedForward) > 0.0001) {
    rotatedRight = rotateAroundAxis(rotatedRight, rotatedForward, rotSpeedForward);
    rotatedUp = rotateAroundAxis(rotatedUp, rotatedForward, rotSpeedForward);
  }
  
  // Normalize all vectors
  rotatedForward = rotatedForward.normalized();
  rotatedRight = rotatedRight.normalized();
  rotatedUp = rotatedUp.normalized();
  
  // Use Gram-Schmidt orthogonalization to ensure orthogonality while preserving rotations
  // This maintains the rotated state while fixing any numerical drift
  
  // Keep forward as-is (it's the primary direction)
  Vector3 newForward = rotatedForward;
  
  // Make right orthogonal to forward
  Vector3 newRight = rotatedRight - newForward * newForward.dot(rotatedRight);
  newRight = newRight.normalized();
  if (newRight.length() < 0.001) {
    // Fallback: construct right from forward and world up
    newRight = newForward.cross(Vector3(0, 1, 0)).normalized();
    if (newRight.length() < 0.001) {
      newRight = newForward.cross(Vector3(1, 0, 0)).normalized();
    }
  }
  
  // Make up orthogonal to both forward and right
  Vector3 newUp = rotatedUp - newForward * newForward.dot(rotatedUp) - newRight * newRight.dot(rotatedUp);
  newUp = newUp.normalized();
  if (newUp.length() < 0.001) {
    // Fallback: construct up from forward and right
    newUp = newRight.cross(newForward).normalized();
  }
  
  // Ensure right-handed coordinate system
  Vector3 crossCheck = newRight.cross(newForward);
  if (crossCheck.dot(newUp) < 0.0) {
    // Flip up to maintain right-handedness
    newUp = newUp * -1.0;
  }
  
  // Update camera basis directly (don't use lookAt as it recalculates)
  cam.forward = newForward;
  cam.right = newRight;
  cam.up = newUp;
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

