
#include "vk_camera.hpp"

#include <fmt/core.h>

void Camera::setPerspectiveProjection(float t_fovy, float t_aspectRatio, float t_near, float t_far){
    if(glm::abs(t_aspectRatio - std::numeric_limits<float>::epsilon()) <= 0.01f){
        t_aspectRatio = 16.0f/9.0f;
    }
    const float e = 1.0f / std::tan(t_fovy * 0.5f);

    const glm::mat4 reverseZ  = {   1.0f, 0.0f,  0.0f, 0.0f,
                                    0.0f, 1.0f,  0.0f, 0.0f,
                                    0.0f, 0.0f, -1.0f, 0.0f,
                                    0.0f, 0.0f,  1.0f, 1.0f };
                                
    m_projectionMatrix = glm::mat4{0.0f};
    m_projectionMatrix[0][0] = e / t_aspectRatio;
    m_projectionMatrix[1][1] = e;
    m_projectionMatrix[2][2] = t_far / (t_far - t_near);
    m_projectionMatrix[2][3] = 1.0f;
    m_projectionMatrix[3][2] = -(t_far * t_near) / (t_far - t_near);

    m_projectionMatrix  = m_projectionMatrix * reverseZ;
}

void Camera::processSDLEvent(const SDL_Event& t_event){
    
   
    if(t_event.type == SDL_MOUSEBUTTONDOWN && t_event.button.button == SDL_BUTTON_RIGHT){
        m_isMovementEnabled = true;
    }
    if(t_event.type == SDL_MOUSEBUTTONUP && t_event.button.button == SDL_BUTTON_RIGHT){
        m_isMovementEnabled = false;
    }
    if(!m_isMovementEnabled){
        return;
    }

    auto& rotation = m_transformation.getRotationRW();
    if (t_event.type == SDL_MOUSEMOTION) {
        
        m_yaw += (float)t_event.motion.xrel / 200.f;
        m_pitch += (float)t_event.motion.yrel / 200.f;
        m_pitch = glm::clamp(m_pitch, -glm::pi<float>() / 2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);
        glm::quat pitchRotation = glm::angleAxis(m_pitch * m_sensitivity, glm::vec3 { 1.f, 0.f, 0.f });
        glm::quat yawRotation = glm::angleAxis(m_yaw * m_sensitivity, glm::vec3 { 0.f, -1.f, 0.f });
        
        rotation = yawRotation * pitchRotation;
    }
    
    auto& position = m_transformation.getPositionRW();
    auto forward = m_transformation.getForwardVector();
    auto right = m_transformation.getRightVector();
    auto up = m_transformation.getUpVector();

    if (t_event.type == SDL_KEYDOWN) {
        if (t_event.key.keysym.sym == SDLK_w) { position +=  forward * m_speed; }
        if (t_event.key.keysym.sym == SDLK_s) { position -=  forward * m_speed; }
        if (t_event.key.keysym.sym == SDLK_a) { position -=  right * m_speed; }
        if (t_event.key.keysym.sym == SDLK_d) { position +=  right * m_speed; } 
        if (t_event.key.keysym.sym == SDLK_q) { position +=  up * m_speed; } 
        if (t_event.key.keysym.sym == SDLK_e) { position -=  up * m_speed; } 
    }
   
}