
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
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb/stb_image.h"

// Window
int windowWidth = 1280;
int windowHeight = 720;

// OpenGL
GLuint shaderProgram = 0;
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
GLuint wallTexture = 0;
GLuint floorTexture = 0;

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
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    FragPos = worldPos.xyz;

    Normal = mat3(transpose(inverse(model))) * aNormal;

    TexCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}
)";




const char* fragmentShaderSource = R"(
#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D ourTexture;
uniform vec3 objectColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    // Get texture color
    vec3 texColor = texture(ourTexture, TexCoord).rgb;
    vec3 baseColor = texColor * objectColor;

    // Ambient lighting
    float ambientStrength = 0.18;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Blinn-Phong specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float spec = pow(
        max(dot(norm, halfwayDir), 0.0),
        32.0
    );

    float specularStrength = 0.15;
    vec3 specular = specularStrength * spec * lightColor;

    // Distance attenuation
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 /
        (1.0 + 0.09 * distance + 0.032 * distance * distance);

    vec3 lighting = ambient +
                    (diffuse + specular) * attenuation;

    vec3 result = lighting * baseColor;

    FragColor = vec4(result, 1.0);
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

GLuint loadTexture(const char* path)
{
    int width, height, channels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(
        path, &width, &height, &channels, 0
    );

    if (!data) {
        std::cerr << "Failed to load texture: "
            << path << std::endl;
        return 0;
    }

    GLenum format;

    if (channels == 4)
        format = GL_RGBA;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 1)
        format = GL_RED;
    else {
        std::cerr << "Unsupported texture format: "
            << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    stbi_image_free(data);

    std::cout << "Loaded texture: "
        << path << std::endl;

    return texture;
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



    std::vector<float> texturedVertices;

    const float uv[6][2] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    // Each cube face has 6 vertices.
    for (int i = 0; i < 36; i++)
    {
        int face = i / 6;

        glm::vec3 p0(
            vertices[face * 18],
            vertices[face * 18 + 1],
            vertices[face * 18 + 2]
        );

        glm::vec3 p1(
            vertices[face * 18 + 3],
            vertices[face * 18 + 4],
            vertices[face * 18 + 5]
        );

        glm::vec3 p2(
            vertices[face * 18 + 6],
            vertices[face * 18 + 7],
            vertices[face * 18 + 8]
        );

        glm::vec3 normal = glm::normalize(
            glm::cross(p1 - p0, p2 - p0)
        );

        texturedVertices.push_back(vertices[i * 3]);
        texturedVertices.push_back(vertices[i * 3 + 1]);
        texturedVertices.push_back(vertices[i * 3 + 2]);

        texturedVertices.push_back(uv[i % 6][0]);
        texturedVertices.push_back(uv[i % 6][1]);

        texturedVertices.push_back(normal.x);
        texturedVertices.push_back(normal.y);
        texturedVertices.push_back(normal.z);
    }

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        texturedVertices.size() * sizeof(float),
        texturedVertices.data(),
        GL_STATIC_DRAW
    );

    // Position: XYZ
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        8 * sizeof(float), (void*)0
    );
    glEnableVertexAttribArray(0);

    // Texture coordinates: UV
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        8 * sizeof(float),
        (void*)(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    // Normal: NX, NY, NZ
    glVertexAttribPointer(
        2, 3, GL_FLOAT, GL_FALSE,
        8 * sizeof(float),
        (void*)(5 * sizeof(float))
    );
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        texturedVertices.size() * sizeof(float),
        texturedVertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        5 * sizeof(float), (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        (void*)(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

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

    // Point light position
    glm::vec3 lightPos = cameraPos + glm::vec3(0.0f, 0.5f, 0.0f);

    // Warm torch-like light
    glm::vec3 lightColor(1.0f, 0.75f, 0.45f);

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "lightPos"),
        1,
        glm::value_ptr(lightPos)
    );

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "lightColor"),
        1,
        glm::value_ptr(lightColor)
    );

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "viewPos"),
        1,
        glm::value_ptr(cameraPos)
    );

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

    // Activate texture unit 0
    glActiveTexture(GL_TEXTURE0);

    // Tell the shader to use texture unit 0
    glUniform1i(
        glGetUniformLocation(shaderProgram, "ourTexture"),
        0
    );

    // Draw maze
    for (int row = 0; row < static_cast<int>(maze.size()); row++)
    {
        for (int col = 0; col < static_cast<int>(maze[row].size()); col++)
        {
            // 1. Bind floor texture
            glBindTexture(GL_TEXTURE_2D, floorTexture);

            // Draw floor
            drawCube(
                glm::vec3(col + 0.5f, -0.08f, row + 0.5f),
                glm::vec3(1.0f, 0.16f, 1.0f),
                glm::vec3(1.0f, 1.0f, 1.0f)
            );
            // Ceiling
            glBindTexture(GL_TEXTURE_2D, wallTexture);

            drawCube(
                glm::vec3(col + 0.5f, 2.08f, row + 0.5f),
                glm::vec3(1.0f, 0.16f, 1.0f),
                glm::vec3(1.0f)
            );
            // 2. Bind wall texture
            if (maze[row][col] == '#')
            {
                glBindTexture(GL_TEXTURE_2D, wallTexture);

                // Draw wall
                drawCube(
                    glm::vec3(col + 0.5f, 1.0f, row + 0.5f),
                    glm::vec3(1.0f, 2.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f)
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
    wallTexture = loadTexture("assets/textures/wall.jpg");
    floorTexture = loadTexture("assets/textures/floor.jpg");
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
