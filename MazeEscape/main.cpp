
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#include <cstdlib>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Window
int windowWidth = 1280;
int windowHeight = 720;

// OpenGL
GLuint shaderProgram = 0;
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;

// Camera
glm::vec3 cameraPos(1.5f, 0.5f, 1.5f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float cameraSpeed = 2.5f;
float mouseSensitivity = 0.12f;

// Input
bool keys[256] = {};
bool firstMouse = true;
bool mouseCaptured = true;

int lastMouseX = 640;
int lastMouseY = 360;
int lastTime = 0;

// Maze
const std::vector<std::string> maze = {
    "###########",
    "#S........#",
    "#.#######.#",
    "#.....#...#",
    "#####.#.###",
    "#.....#...#",
    "#.#######.#",
    "#.........#",
    "#.#######.#",
    "#........E#",
    "###########"
};

// Shaders
const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model *
                  vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

uniform vec3 objectColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(objectColor, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error: "
            << log << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return shader;
}

void createShaders()
{
    GLuint vertex = compileShader(
        GL_VERTEX_SHADER, vertexShaderSource
    );

    GLuint fragment = compileShader(
        GL_FRAGMENT_SHADER, fragmentShaderSource
    );

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    GLint success = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(
            shaderProgram, 1024, nullptr, log
        );
        std::cerr << "Shader link error: "
            << log << std::endl;
        std::exit(EXIT_FAILURE);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void createCube()
{
    float vertices[] = {
        // Front
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

        // Back
         0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
         0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,

         // Left
         -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,

         // Right
          0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
          0.5f,-0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,

          // Top
          -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
          -0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,

          // Bottom
          -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
          -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), (void*)0
    );

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void drawCube(glm::vec3 position,
    glm::vec3 scale,
    glm::vec3 color)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "objectColor"),
        1, glm::value_ptr(color)
    );

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

bool isWall(float x, float z)
{
    int col = static_cast<int>(std::floor(x));
    int row = static_cast<int>(std::floor(z));

    if (row < 0 || row >= static_cast<int>(maze.size()))
        return true;

    if (col < 0 || col >= static_cast<int>(maze[row].size()))
        return true;

    return maze[row][col] == '#';
}

bool canMove(float x, float z)
{
    const float radius = 0.18f;

    return !isWall(x - radius, z - radius) &&
        !isWall(x + radius, z - radius) &&
        !isWall(x - radius, z + radius) &&
        !isWall(x + radius, z + radius);
}

void processMovement(float deltaTime)
{
    float velocity = cameraSpeed * deltaTime;

    glm::vec3 forward(
        cameraFront.x, 0.0f, cameraFront.z
    );

    if (glm::length(forward) < 0.001f)
        return;

    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(
        glm::cross(forward, cameraUp)
    );

    glm::vec3 movement(0.0f);

    if (keys['w']) movement += forward;
    if (keys['s']) movement -= forward;
    if (keys['a']) movement -= right;
    if (keys['d']) movement += right;

    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement) * velocity;

        float newX = cameraPos.x + movement.x;
        float newZ = cameraPos.z + movement.z;

        if (canMove(newX, cameraPos.z))
            cameraPos.x = newX;

        if (canMove(cameraPos.x, newZ))
            cameraPos.z = newZ;
    }
}

void display()
{
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    if (deltaTime > 0.05f)
        deltaTime = 0.05f;

    processMovement(deltaTime);

    glClearColor(0.08f, 0.09f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glm::mat4 view = glm::lookAt(
        cameraPos,
        cameraPos + cameraFront,
        cameraUp
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(70.0f),
        static_cast<float>(windowWidth) / windowHeight,
        0.1f,
        100.0f
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(view)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection)
    );

    // Floor and walls
    for (int row = 0; row < static_cast<int>(maze.size()); row++)
    {
        for (int col = 0; col < static_cast<int>(maze[row].size()); col++)
        {
            // Floor
            drawCube(
                glm::vec3(col + 0.5f, -0.08f, row + 0.5f),
                glm::vec3(1.0f, 0.16f, 1.0f),
                glm::vec3(0.25f, 0.24f, 0.23f)
            );

            // Wall
            if (maze[row][col] == '#')
            {
                drawCube(
                    glm::vec3(col + 0.5f, 1.0f, row + 0.5f),
                    glm::vec3(1.0f, 2.0f, 1.0f),
                    glm::vec3(0.48f, 0.47f, 0.45f)
                );
            }
        }
    }

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    windowWidth = width;
    windowHeight = (height > 0) ? height : 1;

    glViewport(0, 0, windowWidth, windowHeight);
}

void keyboardDown(unsigned char key, int x, int y)
{
    if (key == 27) {
        glutLeaveMainLoop();
        return;
    }

    if (key >= 'A' && key <= 'Z')
        key = key - 'A' + 'a';

    keys[key] = true;
}

void keyboardUp(unsigned char key, int x, int y)
{
    if (key >= 'A' && key <= 'Z')
        key = key - 'A' + 'a';

    keys[key] = false;
}

void mouseMotion(int x, int y)
{
    if (!mouseCaptured)
        return;

    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;

    if (firstMouse) {
        firstMouse = false;
        glutWarpPointer(centerX, centerY);
        return;
    }

    float xOffset = (x - centerX) * mouseSensitivity;
    float yOffset = (centerY - y) * mouseSensitivity;

    if (x == centerX && y == centerY)
        return;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) *
        cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) *
        cos(glm::radians(pitch));

    cameraFront = glm::normalize(direction);

    glutWarpPointer(centerX, centerY);
}

void idle()
{
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(
        GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH
    );

    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("3D Maze Escape - Day 1");

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed!\n";
        return 1;
    }

    std::cout << "OpenGL: "
        << glGetString(GL_VERSION) << "\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    createShaders();
    createCube();

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(idle);

    glutIgnoreKeyRepeat(1);
    glutSetCursor(GLUT_CURSOR_NONE);

    glutWarpPointer(windowWidth / 2, windowHeight / 2);

    glutMainLoop();

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteProgram(shaderProgram);

    return 0;
}
