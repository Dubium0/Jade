#pragma once 

#include <SDL_events.h>

#include "vk_transform.hpp"


class Camera{
    private:
    Transformation m_transformation{};
    glm::mat4 m_projectionMatrix{1.0f};
    float m_speed{2.0f};
    float m_sensitivity{1.0f};
    float m_yaw{0.0f};
    float m_pitch{0.0f};
    bool m_isMovementEnabled{false};

    public://inline functions
    inline const glm::mat4&  getProjectionMatrix() const { return m_projectionMatrix;}

    inline glm::mat4 getViewMatrix() const { return glm::inverse(m_transformation.getTransformationMatrix());}

    inline const Transformation& getTransformationR()const { return m_transformation; }
    inline Transformation& getTransformationRW() { return m_transformation; }
    
    inline float& getCameraSpeedRW() {return m_speed;}
    inline const float& getCameraSpeedR() const {return m_speed;}

    inline float& getCameraSensitivityRW() {return m_sensitivity;}
    inline const float& getCameraSensitivityR() const {return m_sensitivity;}
    public:
    void setPerspectiveProjection(float t_fovy, float t_aspectRatio, float t_near, float t_far);
    void processSDLEvent(const SDL_Event& t_event);
};
