#pragma once
#include <unordered_map>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>



class Transformation{
    
    private:
    glm::vec3 m_position = {0.0f,0.0f,0.0f};
    glm::quat m_rotation = {0.0,0.0,0.0,1.0};
    glm::vec3 m_scale = {1.0f,1.0f,1.0f};

    static const glm::vec3 kGlobalRight;
    static const glm::vec3 kGlobalUp;
    static const glm::vec3 kGlobalForward;

    public:
    Transformation(){}
    Transformation( glm::vec3 t_position, 
                    glm::quat t_rotation, 
                    glm::vec3 t_scale):
     m_position(t_position),m_scale(t_scale),m_rotation(t_rotation){}
    
    inline const glm::vec3& getPositionR() const {return m_position;}
    inline glm::vec3& getPositionRW() { return m_position;}

    inline const glm::quat& getRotationR() const {return m_rotation;}
    inline glm::quat& getRotationRW() { return m_rotation; }

    inline const glm::vec3& getScaleR() const {return m_scale;}
    inline glm::vec3& getScaleRW() { return m_scale;}

    inline glm::mat4 getTransformationMatrix() const 
    {
        return glm::translate(glm::mat4(1.0f),m_position)
                * glm::mat4_cast(m_rotation) 
                * glm::scale(glm::mat4(1.0f),m_scale);

    }
    inline glm::vec3 getRightVector() const { return m_rotation * kGlobalRight;}
    inline glm::vec3 getUpVector() const { return m_rotation *  kGlobalUp;}
    inline glm::vec3 getForwardVector() const { return m_rotation *  kGlobalForward;}

    #ifdef TRANSFORMATION_ENABLE_SET_FUNCTIONS
    inline void setPosition(float t_x,float t_y, float t_z){
        m_position.x = t_x;
        m_position.y = t_y;
        m_position.z = t_z;
    }
    inline void setScale(float t_x,float t_y, float t_z){
        m_scale.x = t_x;
        m_scale.y = t_y;
        m_scale.z = t_z;

    }

    inline void setRotation(const glm::quat& t_rotation){
        m_rotation = t_rotation;
    }
    inline void setRotation(const glm::vec3& t_rotationEulerAngles){
        m_rotation = glm::quat(t_rotationEulerAngles);
    }
    inline void setRotation(float t_rotationAngle, const glm::vec3& t_rotationAxis){
        m_rotation = glm::angleAxis(t_rotationAngle,t_rotationAxis);
    }
    #endif
   
    inline glm::mat4 getTranslationMatrix() const 
    {
        return glm::translate(glm::mat4(1.0f),m_position);
    }
    inline glm::mat4 getRotationMatrix() const 
    {
        return glm::mat4_cast(m_rotation);
    }
    inline glm::mat4 getScaleMatrix() const 
    {
        return glm::scale(glm::mat4(1.0f),m_scale);
    }
   
};