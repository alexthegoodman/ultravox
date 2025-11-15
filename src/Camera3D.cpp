#pragma once

#define GLM_ENABLE_EXPERIMENTAL 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

class Camera3D {
public:
    glm::vec3 position3D;
    glm::quat rotation;
    glm::vec3 up;
    glm::vec3 target;

    float fov;   // field of view in radians
    float nearPlane;
    float farPlane;
    float zoom;

    // Default state
    glm::vec3 defaultPosition3D;
    glm::quat defaultRotation;
    glm::vec3 defaultTarget;

    float mouseSensitivity;
    float resetZ;

    Camera3D()
        : position3D(0.0f, 2.0f, 5.0f),
          rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
          up(0.0f, 1.0f, 0.0f),
          target(0.0f, 0.0f, -1.0f),
          fov(glm::radians(22.5f)), // ~PI/8
          nearPlane(0.1f),
          farPlane(1000.0f),
          zoom(1.0f),
          mouseSensitivity(0.5f),
          resetZ(5.0f)
    {
        defaultPosition3D = position3D;
        defaultRotation = rotation;
        defaultTarget = target;
    }

    // Perspective projection
    glm::mat4 getProjection(float aspectRatio = 1.0f) const {
        return glm::perspective(fov, aspectRatio, nearPlane, farPlane);
    }

    // View matrix
    glm::mat4 getView() const {
        return glm::lookAt(position3D, target, up);
    }

    // Set position
    void setPosition(float x, float y, float z) {
        position3D = glm::vec3(x, y, z);
    }

    // Look at a specific target
    void lookAt(const glm::vec3& newTarget) {
        target = newTarget;

        glm::vec3 direction = glm::normalize(target - position3D);
        glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);

        rotation = glm::rotation(forward, direction);
    }

    // Update target based on rotation quaternion
    void updateTarget() {
        glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
        forward = rotation * forward; // rotate the forward vector
        target = position3D + forward;
    }

    // Pitch (look up/down)
    void pitch(float angleDegrees) {
        float angleRadians = glm::radians(angleDegrees);
        glm::quat pitchQuat = glm::angleAxis(angleRadians, glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::normalize(pitchQuat * rotation);
        updateTarget();
    }

    // Yaw (look left/right)
    void yaw(float angleDegrees) {
        float angleRadians = glm::radians(angleDegrees);
        glm::quat yawQuat = glm::angleAxis(angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::normalize(yawQuat * rotation);
        updateTarget();
    }

    void updateZoomFromSlider(float zoomValue) {
        glm::vec3 direction = glm::normalize(position3D - target);
        position3D = target + direction * zoomValue;
    }

    void updateZoomFromSlider2(float zoomDelta) {
        glm::vec3 viewDirection = glm::normalize(target - position3D); // Looking direction
        position3D += viewDirection * zoomDelta; // Move forward or backward
    }

    // Get current pitch in degrees
    float getPitch() const {
        // Extract pitch from quaternion without converting to Euler
        glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        forward = glm::normalize(forward);
        
        // Calculate pitch angle from the forward vector's y component
        float pitch = glm::degrees(asin(forward.y));
        return pitch;
    }

    // Get current yaw in degrees
    float getYaw() const {
        // Extract yaw from quaternion without converting to Euler
        glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        forward = glm::normalize(forward);
        
        // Calculate yaw angle from the forward vector's x and z components
        float yaw = glm::degrees(atan2(forward.x, -forward.z));
        return yaw;
    }

    // Set pitch directly
    void setPitch(float angleDegrees) {
        // Get current yaw to preserve it
        float currentYaw = getYaw();
        
        // Build rotation from scratch using pitch and yaw
        glm::quat yawQuat = glm::angleAxis(glm::radians(currentYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat pitchQuat = glm::angleAxis(glm::radians(angleDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Apply yaw first, then pitch
        rotation = glm::normalize(yawQuat * pitchQuat);
        updateTarget();
    }

    // Set yaw directly
    void setYaw(float angleDegrees) {
        // Get current pitch to preserve it
        float currentPitch = getPitch();
        
        // Build rotation from scratch using pitch and yaw
        glm::quat yawQuat = glm::angleAxis(glm::radians(angleDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat pitchQuat = glm::angleAxis(glm::radians(currentPitch), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Apply yaw first, then pitch
        rotation = glm::normalize(yawQuat * pitchQuat);
        updateTarget();
    }

    // Pan camera
    void pan(float deltaX, float deltaY) {
        glm::vec3 right = glm::normalize(glm::cross(target - position3D, up));
        glm::vec3 cameraUp = glm::normalize(glm::cross(right, target - position3D));

        position3D += right * deltaX;
        target += right * deltaX;

        position3D += cameraUp * deltaY;
        target += cameraUp * deltaY;
    }

    // Move forward/backward (zoom)
    void updateZoom(float delta) {
        glm::vec3 direction = glm::normalize(target - position3D);
        position3D += direction * delta;
        target += direction * delta;
    }

    // Reset camera to default state
    void resetCamera() {
        position3D = defaultPosition3D;
        rotation = defaultRotation;
        target = defaultTarget;
        zoom = 1.0f;
    }

    // Reset zoom and target
    void resetZoom() {
        position3D.z = resetZ;
        target = defaultTarget;
    }
};
