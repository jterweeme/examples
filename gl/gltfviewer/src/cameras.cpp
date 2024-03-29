#include "cameras.hpp"
#include <GLFW/glfw3.h>

// Good reference here to map camera movements to lookAt calls
// http://learnwebgl.brown37.net/07_cameras/camera_movement.html

using namespace glm;

struct ViewFrame
{
  vec3 left;
  vec3 up;
  vec3 front;
  vec3 eye;

  ViewFrame(vec3 l, vec3 u, vec3 f, vec3 e) : left(l), up(u), front(f), eye(e)
  {
  }
};

Camera::Camera(glm::vec3 e, glm::vec3 c, glm::vec3 u) : m_eye(e), m_center(c), m_up(u)
{
    const auto front = m_center - m_eye;
    const auto left = cross(m_up, front);
    assert(left != glm::vec3(0));
    m_up = normalize(cross(front, left));
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_eye, m_center, m_up);
}

// Move the camera along its left axis.
void Camera::truckLeft(float offset)
{
    const auto front = m_center - m_eye;
    const auto left = normalize(cross(m_up, front));
    const auto translationVector = offset * left;
    m_eye += translationVector;
    m_center += translationVector;
}

void Camera::pedestalUp(float offset)
{
    const auto translationVector = offset * m_up;
    m_eye += translationVector;
    m_center += translationVector;
}

void Camera::dollyIn(float offset)
{ 
    const auto front = normalize(m_center - m_eye);
    const auto translationVector = offset * front;
    m_eye += translationVector;
    m_center += translationVector;
}

ViewFrame fromViewToWorldMatrix(const mat4 &viewToWorldMatrix)
{
  return ViewFrame{-vec3(viewToWorldMatrix[0]), vec3(viewToWorldMatrix[1]),
      -vec3(viewToWorldMatrix[2]), vec3(viewToWorldMatrix[3])};
}

bool FirstPersonCameraController::update(float elapsedTime)
{
  if (glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
      !m_LeftButtonPressed) {
    m_LeftButtonPressed = true;
    glfwGetCursorPos(
        m_pWindow, &m_LastCursorPosition.x, &m_LastCursorPosition.y);
  } else if (!glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
             m_LeftButtonPressed) {
    m_LeftButtonPressed = false;
  }

  const auto cursorDelta = ([&]() {
    if (m_LeftButtonPressed) {
      dvec2 cursorPosition;
      glfwGetCursorPos(m_pWindow, &cursorPosition.x, &cursorPosition.y);
      const auto delta = cursorPosition - m_LastCursorPosition;
      m_LastCursorPosition = cursorPosition;
      return delta;
    }
    return dvec2(0);
  })();

  float truckLeft = 0.f;
  float pedestalUp = 0.f;
  float dollyIn = 0.f;
  float rollRightAngle = 0.f;

  if (glfwGetKey(m_pWindow, GLFW_KEY_W)) {
    dollyIn += m_fSpeed * elapsedTime;
  }

  // Truck left
  if (glfwGetKey(m_pWindow, GLFW_KEY_A)) {
    truckLeft += m_fSpeed * elapsedTime;
  }

  // Pedestal up
  if (glfwGetKey(m_pWindow, GLFW_KEY_UP)) {
    pedestalUp += m_fSpeed * elapsedTime;
  }

  // Dolly out
  if (glfwGetKey(m_pWindow, GLFW_KEY_S)) {
    dollyIn -= m_fSpeed * elapsedTime;
  }

  // Truck right
  if (glfwGetKey(m_pWindow, GLFW_KEY_D)) {
    truckLeft -= m_fSpeed * elapsedTime;
  }

  // Pedestal down
  if (glfwGetKey(m_pWindow, GLFW_KEY_DOWN)) {
    pedestalUp -= m_fSpeed * elapsedTime;
  }

  if (glfwGetKey(m_pWindow, GLFW_KEY_Q)) {
    rollRightAngle -= 0.001f;
  }
  if (glfwGetKey(m_pWindow, GLFW_KEY_E)) {
    rollRightAngle += 0.001f;
  }

  // cursor going right, so minus because we want pan left angle:
  const float panLeftAngle = -0.01f * float(cursorDelta.x);
  const float tiltDownAngle = 0.01f * float(cursorDelta.y);

  const auto hasMoved = truckLeft || pedestalUp || dollyIn || panLeftAngle ||
                        tiltDownAngle || rollRightAngle;
  if (!hasMoved) {
    return false;
  }

  m_camera.moveLocal(truckLeft, pedestalUp, dollyIn);
  m_camera.rotateLocal(rollRightAngle, tiltDownAngle, 0.f);
  m_camera.rotateWorld(panLeftAngle, m_worldUpAxis);

  return true;
}

bool TrackballCameraController::update(float elapsedTime)
{
  if (glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
      !m_MiddleButtonPressed) {
    m_MiddleButtonPressed = true;
    glfwGetCursorPos(
        m_pWindow, &m_LastCursorPosition.x, &m_LastCursorPosition.y);
  } else if (!glfwGetMouseButton(m_pWindow, GLFW_MOUSE_BUTTON_MIDDLE) &&
             m_MiddleButtonPressed) {
    m_MiddleButtonPressed = false;
  }

  const auto cursorDelta = ([&]() {
    if (m_MiddleButtonPressed) {
      dvec2 cursorPosition;
      glfwGetCursorPos(m_pWindow, &cursorPosition.x, &cursorPosition.y);
      const auto delta = cursorPosition - m_LastCursorPosition;
      m_LastCursorPosition = cursorPosition;
      return delta;
    }
    return dvec2(0);
  })();

  if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_SHIFT)) {
    // Pan
    const auto truckLeft = 0.01f * float(cursorDelta.x);
    const auto pedestalUp = 0.01f * float(cursorDelta.y);
    const auto hasMoved = truckLeft || pedestalUp;
    if (!hasMoved) {
      return false;
    }

    m_camera.moveLocal(truckLeft, pedestalUp, 0.f);

    return true;
  }

  if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL)) {
    // Zoom
    auto mouseOffset = 0.01f * float(cursorDelta.x);
    if (mouseOffset == 0.f) {
      return false;
    }

    // We need to move along the view vector of the camera
    const auto viewVector = m_camera.center() - m_camera.eye();
    const auto l = glm::length(viewVector);
    if (mouseOffset > 0.f) {
      // We don't want to move more that the length of the view vector (cannot
      // go beyond target)
      mouseOffset = glm::min(mouseOffset, l - 1e-4f);
    }
    // Normalize view vector for the translation
    const auto front = viewVector / l;
    const auto translationVector = mouseOffset * front;

    // Update camera with new eye position
    const auto newEye = m_camera.eye() + translationVector;
    m_camera = Camera(newEye, m_camera.center(), m_worldUpAxis);

    return true;
  }

  // Rotate around target

  const auto longitudeAngle = 0.01f * float(cursorDelta.y); // Vertical angle
  const auto latitudeAngle = -0.01f * float(cursorDelta.x); // Horizontal angle
  const auto hasMoved = longitudeAngle || latitudeAngle;
  if (!hasMoved) {
    return false;
  }

  // We need to rotate eye around center, for that we rotate the vector [center,
  // eye] (= depthAxis) in order to compute a new eye position
  const auto depthAxis = m_camera.eye() - m_camera.center();

  const auto latitudeRotationMatrix =
      rotate(mat4(1), latitudeAngle, m_worldUpAxis);

  const auto horizontalAxis = m_camera.left();
  const auto rotationMatrix =
      rotate(latitudeRotationMatrix, longitudeAngle, horizontalAxis);
  auto rotatedDepthAxis = vec3(rotationMatrix * vec4(depthAxis, 0));

  // Update camera with new eye position
  const auto newEye = m_camera.center() + rotatedDepthAxis;
  m_camera = Camera(newEye, m_camera.center(), m_worldUpAxis);

  return true;
}
