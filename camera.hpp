class Camera
{
public:

    // camera attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up; // camera local UP vector

    GLfloat yaw = -90.0f;
    GLfloat pitch = 0.0f;;
    GLfloat roll = 0.0f;

    // camera options
    GLfloat movementSpeed = 1.0f;
    GLfloat mouseSensitivity = 0.25f;

    Camera(glm::vec3 position) :position(position)
    {
        this->up = glm::vec3(0.0f, 1.0f, 0.0f);
        // initialization of the camera reference system
        this->updateCameraVectors();
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(this->position, this->position + this->front, this->up);
    }

    glm::vec3 ProcessInput(GLFWwindow* window, GLfloat deltaTime)
    {
        glm::vec3 direction{ 0 };

        // W,A,S,D keys - Movement in camera's local horizontal plane
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            direction += front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            direction -= front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            direction -= right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            direction += right;

        // Q,E keys - Vertical movement
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            direction -= up;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            direction += up;

        // Only normalize if there's actual movement
        if (glm::length(direction) > 0.0f)
            direction = glm::normalize(direction) * movementSpeed * deltaTime;

        return direction;
    }

    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constraintPitch = GL_TRUE)
    {
        xoffset *= this->mouseSensitivity;
        yoffset *= this->mouseSensitivity;

        this->yaw += xoffset;
        this->pitch += yoffset;

        if (constraintPitch)
        {
            if (this->pitch > 89.0f)
                this->pitch = 89.0f;
            if (this->pitch < -89.0f)
                this->pitch = -89.0f;
        }

        this->updateCameraVectors();
    }

private:
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
        front.y = sin(glm::radians(this->pitch));
        front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));

        this->front = glm::normalize(front);
        this->right = glm::normalize(glm::cross(this->front, glm::vec3(0.0f, 1.0f, 0.0f)));
        this->up = glm::normalize(glm::cross(this->right, this->front));
    }
};
