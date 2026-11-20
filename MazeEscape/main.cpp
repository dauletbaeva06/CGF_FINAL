#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>

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
GLuint keyTexture = 0;

// Camera
const glm::vec3 startPos(1.5f, 0.5f, 1.5f);
glm::vec3 cameraPos = startPos;
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
bool lightEnabled = true;

bool hasKey = false;
bool gameWon = false;

enum GameStatus {
    STATUS_FIND_KEY,
    STATUS_KEY_COLLECTED,
    STATUS_WIN
};
GameStatus lastPrintedStatus = STATUS_FIND_KEY;
bool statusInitialized = false;

// Key & Exit positions (in world coordinates)
glm::vec3 keyPos(9.5f, 0.45f, 1.5f);
glm::vec3 exitPos(9.5f, 0.5f, 9.5f);
glm::vec3 exitTriggerPos(9.5f, 0.0f, 8.5f);
glm::ivec2 doorCell(-1, -1);
glm::vec2 doorOpenDir(0.0f, -1.0f);
const float keyPickupRadius = 0.70f;
const float exitTriggerRadius = 0.75f;

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
    "#.........#",
    "###########"
};

void randomizeKeyAndExit();
void mouseClick(int button, int state, int x, int y);
void drawCube(glm::vec3 position, glm::vec3 scale, glm::vec3 color, float rotYDeg = 0.0f);

void printStatus(GameStatus status, bool force = false)
{
    if (!force && statusInitialized && lastPrintedStatus == status)
        return;

    lastPrintedStatus = status;
    statusInitialized = true;

    if (status == STATUS_FIND_KEY)
        std::cout << "Find the Key" << std::endl;
    else if (status == STATUS_KEY_COLLECTED)
        std::cout << "Key Collected" << std::endl;
    else if (status == STATUS_WIN)
        std::cout << "You Win" << std::endl;
}

void updateCameraDirection()
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void resetGame()
{
    cameraPos = startPos;
    yaw = -90.0f;
    pitch = 0.0f;
    updateCameraDirection();

    hasKey = false;
    gameWon = false;
    firstMouse = true;
    mouseCaptured = true;

    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(windowWidth / 2, windowHeight / 2);

    randomizeKeyAndExit();
    glutMouseFunc(mouseClick);

    printStatus(STATUS_FIND_KEY, true);
}

// Button structure for "Play Again" button
struct ButtonRect {
    float x;
    float y;
    float w;
    float h;
};

ButtonRect getPlayAgainButton()
{
    float w = 240.0f;
    float h = 56.0f;
    float x = (windowWidth - w) * 0.5f;
    float y = (windowHeight * 0.5f) + 25.0f;
    return { x, y, w, h };
}

bool isPointInButton(int mx, int my, const ButtonRect& btn)
{
    return mx >= btn.x && mx <= (btn.x + btn.w) &&
           my >= btn.y && my <= (btn.y + btn.h);
}

void randomizeKeyAndExit()
{
    std::vector<glm::vec2> freeCells;
    struct DoorCandidate {
        int row;
        int col;
        glm::vec3 doorWorldPos;
        glm::vec3 triggerWorldPos;
        glm::vec2 openDir;
    };
    std::vector<DoorCandidate> doorCandidates;

    int rows = static_cast<int>(maze.size());
    for (int row = 0; row < rows; ++row) {
        int cols = static_cast<int>(maze[row].size());
        for (int col = 0; col < cols; ++col) {
            char cell = maze[row][col];
            glm::vec2 center(col + 0.5f, row + 0.5f);

            if (cell == '.' && glm::length(center - glm::vec2(startPos.x, startPos.z)) > 2.0f) {
                freeCells.push_back(center);
            }

            if (cell == '#' && row > 0 && row < rows - 1 && col > 0 && col < cols - 1) {
                const int dr[4] = { -1, 1, 0, 0 };
                const int dc[4] = { 0, 0, -1, 1 };
                for (int d = 0; d < 4; ++d) {
                    int nr = row + dr[d];
                    int nc = col + dc[d];
                    if (maze[nr][nc] == '.') {
                        glm::vec2 walkPos(nc + 0.5f, nr + 0.5f);
                        if (glm::length(walkPos - glm::vec2(startPos.x, startPos.z)) > 2.5f) {
                            glm::vec2 dir(static_cast<float>(dc[d]), static_cast<float>(dr[d]));
                            doorCandidates.push_back({
                                row,
                                col,
                                glm::vec3(col + 0.5f, 0.0f, row + 0.5f),
                                glm::vec3(walkPos.x, 0.0f, walkPos.y),
                                dir
                            });
                            break;
                        }
                    }
                }
            }
        }
    }

    static std::mt19937 rng(1337u);
    rng.seed(static_cast<unsigned int>(glutGet(GLUT_ELAPSED_TIME)) + 42u);

    if (!freeCells.empty()) {
        std::shuffle(freeCells.begin(), freeCells.end(), rng);
        keyPos = glm::vec3(freeCells[0].x, 0.45f, freeCells[0].y);
    }

    doorCell = glm::ivec2(-1, -1);
    if (!doorCandidates.empty()) {
        std::shuffle(doorCandidates.begin(), doorCandidates.end(), rng);
        const auto& chosen = doorCandidates[0];
        doorCell = glm::ivec2(chosen.row, chosen.col);
        exitPos = chosen.doorWorldPos;
        exitTriggerPos = chosen.triggerWorldPos;
        doorOpenDir = chosen.openDir;
    }
}

void draw2DRect(float centerX, float centerY, float w, float h, const glm::vec3& color, float z = -1.0f)
{
    drawCube(glm::vec3(centerX, centerY, z), glm::vec3(w, h, 1.0f), color, 0.0f);
}

void drawBlockLabel(float centerX, float centerY, float height, const std::string& text, const glm::vec3& color, float z = -0.5f)
{
    float glyphW = height * 0.62f;
    float gap = height * 0.24f;
    float thick = std::max(2.5f, height * 0.16f);
    float totalW = text.size() * glyphW + (!text.empty() ? (text.size() - 1) * gap : 0.0f);
    float startX = centerX - totalW * 0.5f + glyphW * 0.5f;

    float halfW = glyphW * 0.5f - thick * 0.5f;
    float halfH = height * 0.5f - thick * 0.5f;

    for (size_t i = 0; i < text.size(); ++i) {
        char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
        float x = startX + i * (glyphW + gap);
        float y = centerY;

        if (ch == ' ' || ch == '\0') continue;

        auto vBar = [&](float vx, float vy, float vh) {
            draw2DRect(vx, vy, thick, vh, color, z);
        };
        auto hBar = [&](float hx, float hy, float hw) {
            draw2DRect(hx, hy, hw, thick, color, z);
        };

        switch (ch) {
        case 'A':
            hBar(x, y - halfH, glyphW);
            hBar(x, y, glyphW);
            vBar(x - halfW, y, height);
            vBar(x + halfW, y, height);
            break;
        case 'G':
            hBar(x, y - halfH, glyphW);
            hBar(x, y + halfH, glyphW);
            vBar(x - halfW, y, height);
            vBar(x + halfW, y + height * 0.22f, height * 0.55f);
            hBar(x + glyphW * 0.18f, y + height * 0.05f, glyphW * 0.55f);
            break;
        case 'I':
            hBar(x, y - halfH, glyphW * 0.8f);
            hBar(x, y + halfH, glyphW * 0.8f);
            vBar(x, y, height);
            break;
        case 'N':
            vBar(x - halfW, y, height);
            vBar(x + halfW, y, height);
            vBar(x - halfW * 0.35f, y - halfH * 0.35f, height * 0.45f);
            hBar(x, y, glyphW * 0.65f);
            vBar(x + halfW * 0.35f, y + halfH * 0.35f, height * 0.45f);
            break;
        case 'O':
            hBar(x, y - halfH, glyphW);
            hBar(x, y + halfH, glyphW);
            vBar(x - halfW, y, height);
            vBar(x + halfW, y, height);
            break;
        case 'R':
            hBar(x, y - halfH, glyphW);
            hBar(x, y - height * 0.08f, glyphW);
            vBar(x - halfW, y, height);
            vBar(x + halfW, y - halfH * 0.5f, height * 0.5f);
            vBar(x + halfW * 0.65f, y + halfH * 0.55f, height * 0.52f);
            break;
        case 'S':
            hBar(x, y - halfH, glyphW);
            hBar(x, y, glyphW);
            hBar(x, y + halfH, glyphW);
            vBar(x - halfW, y - halfH * 0.5f, height * 0.5f);
            vBar(x + halfW, y + halfH * 0.5f, height * 0.5f);
            break;
        case 'T':
            hBar(x, y - halfH, glyphW);
            vBar(x, y, height);
            break;
        case 'U':
            hBar(x, y + halfH, glyphW);
            vBar(x - halfW, y, height);
            vBar(x + halfW, y, height);
            break;
        case 'W':
            vBar(x - halfW, y, height);
            vBar(x + halfW, y, height);
            vBar(x, y + halfH * 0.25f, height * 0.65f);
            hBar(x, y + halfH, glyphW);
            break;
        case 'Y':
            vBar(x - halfW, y - halfH * 0.5f, height * 0.5f);
            vBar(x + halfW, y - halfH * 0.5f, height * 0.5f);
            hBar(x, y, glyphW);
            vBar(x, y + halfH * 0.5f, height * 0.55f);
            break;
        default:
            hBar(x, y, glyphW * 0.7f);
            break;
        }
    }
}

void drawWinOverlay()
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 orthoProj = glm::ortho(
        0.0f, static_cast<float>(windowWidth),
        static_cast<float>(windowHeight), 0.0f,
        -10.0f, 10.0f
    );
    glm::mat4 identityView(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(orthoProj));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(identityView));
    glUniform1i(glGetUniformLocation(shaderProgram, "isUI"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

    float cx = windowWidth * 0.5f;
    float cy = windowHeight * 0.5f;

    draw2DRect(cx, cy, 460.0f, 240.0f, glm::vec3(0.14f, 0.18f, 0.30f), -2.0f);
    draw2DRect(cx, cy - 55.0f, 320.0f, 52.0f, glm::vec3(0.18f, 0.78f, 0.42f), -1.0f);
    drawBlockLabel(cx, cy - 55.0f, 30.0f, "YOU WIN", glm::vec3(1.0f, 1.0f, 1.0f), -0.5f);

    ButtonRect btn = getPlayAgainButton();
    float btnCenterX = btn.x + btn.w * 0.5f;
    float btnCenterY = btn.y + btn.h * 0.5f;
    draw2DRect(btnCenterX, btnCenterY, btn.w, btn.h, glm::vec3(0.96f, 0.62f, 0.12f), -1.0f);
    drawBlockLabel(btnCenterX, btnCenterY, 20.0f, "START AGAIN", glm::vec3(0.12f, 0.12f, 0.16f), -0.5f);

    glUniform1i(glGetUniformLocation(shaderProgram, "isUI"), 0);
    glEnable(GL_DEPTH_TEST);
}

void mouseClick(int button, int state, int x, int y)
{
    if (!gameWon)
        return;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        ButtonRect btn = getPlayAgainButton();
        if (isPointInButton(x, y, btn)) {
            resetGame();
        }
    }
}

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

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform bool lightEnabled;
uniform bool isUI;
uniform sampler2D ourTexture;
uniform vec3 objectColor;


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
    
    if (!lightEnabled) {
        lighting = vec3(0.12);
    }

    if (isUI) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

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
    int width = 0, height = 0, channels = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(
        path, &width, &height, &channels, 3
    );

    if (!data || width <= 0 || height <= 0) {
        std::cerr << "Failed to load texture: "
            << path << std::endl;
        if (data) stbi_image_free(data);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    std::cout << "Loaded texture: " << path << std::endl;
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

}

void drawCube(glm::vec3 position,
    glm::vec3 scale,
    glm::vec3 color,
    float rotYDeg)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);
    model = glm::rotate(model, glm::radians(rotYDeg), glm::vec3(0.0f, 1.0f, 0.0f));
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

void drawKeyPart(const glm::vec3& center,
                 const glm::vec3& localOffset,
                 const glm::vec3& scale,
                 const glm::vec3& color,
                 float spinDeg)
{
    glm::mat4 rotMat = glm::rotate(
        glm::mat4(1.0f),
        glm::radians(spinDeg),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::vec3 worldOffset = glm::vec3(rotMat * glm::vec4(localOffset, 1.0f));
    drawCube(center + worldOffset, scale, color, spinDeg);
}

void drawKey(float timeSec)
{
    float bobY = keyPos.y + 0.08f * sin(timeSec * 3.0f);
    float spinDeg = timeSec * 90.0f;
    glm::vec3 center(keyPos.x, bobY, keyPos.z);

    glm::vec3 gold(1.0f, 0.84f, 0.05f);
    glm::vec3 darkGold(0.75f, 0.55f, 0.05f);

    glBindTexture(GL_TEXTURE_2D, keyTexture ? keyTexture : floorTexture);

    float ringZ = -0.22f;
    drawKeyPart(center, glm::vec3(0.0f,  0.07f, ringZ),        glm::vec3(0.05f, 0.035f, 0.14f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f, -0.07f, ringZ),        glm::vec3(0.05f, 0.035f, 0.14f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f,  0.0f,  ringZ - 0.055f), glm::vec3(0.05f, 0.17f, 0.035f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f,  0.0f,  ringZ + 0.055f), glm::vec3(0.05f, 0.17f, 0.035f), gold, spinDeg);

    drawKeyPart(center, glm::vec3(0.0f,  0.0f,  0.02f),         glm::vec3(0.045f, 0.045f, 0.30f), gold,     spinDeg);
    drawKeyPart(center, glm::vec3(0.0f,  0.0f, -0.06f),         glm::vec3(0.052f, 0.055f, 0.025f), darkGold, spinDeg);

    drawKeyPart(center, glm::vec3(0.0f, -0.055f, 0.13f),         glm::vec3(0.04f,  0.08f,  0.035f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f, -0.045f, 0.07f),         glm::vec3(0.04f,  0.06f,  0.035f), gold, spinDeg);
}

void drawDoor()
{
    glBindTexture(GL_TEXTURE_2D, wallTexture);

    glm::vec3 frameColor(0.45f, 0.35f, 0.25f);
    glm::vec3 doorColor = hasKey
        ? glm::vec3(0.2f, 0.9f, 0.35f)
        : glm::vec3(0.85f, 0.22f, 0.22f);

    float yawDeg = 0.0f;
    if (std::abs(doorOpenDir.x) > 0.5f) {
        yawDeg = 90.0f;
    }

    glm::vec3 frontShift(doorOpenDir.x * 0.22f, 0.0f, doorOpenDir.y * 0.22f);
    glm::vec3 base = exitPos + frontShift;

    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(yawDeg), glm::vec3(0.0f, 1.0f, 0.0f));
    auto toWorld = [&](glm::vec3 localPos) {
        return base + glm::vec3(rotMat * glm::vec4(localPos, 1.0f));
    };

    drawCube(toWorld(glm::vec3(-0.36f, exitPos.y + 0.75f, 0.0f)), glm::vec3(0.12f, 1.50f, 0.16f), frameColor, yawDeg);
    drawCube(toWorld(glm::vec3( 0.36f, exitPos.y + 0.75f, 0.0f)), glm::vec3(0.12f, 1.50f, 0.16f), frameColor, yawDeg);

    float doorY = exitPos.y + (hasKey ? 1.25f : 0.70f);
    drawCube(toWorld(glm::vec3(0.0f, doorY, 0.0f)), glm::vec3(0.62f, 1.35f, 0.10f), doorColor, yawDeg);
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

void updateGameplayLogic()
{
    if (gameWon)
        return;

    glm::vec2 playerXZ(cameraPos.x, cameraPos.z);
    glm::vec2 keyXZ(keyPos.x, keyPos.z);
    const float pickupRadius = 1.4f;

    // Key pickup check
    if (!hasKey && glm::length(playerXZ - keyXZ) <= pickupRadius) {
    hasKey = true;
    printStatus(STATUS_KEY_COLLECTED);
}

    // Exit win check 
    if (hasKey) {
        float distToExit = glm::length(playerXZ - glm::vec2(exitTriggerPos.x, exitTriggerPos.z));
        if (distToExit < 0.90f) {
            gameWon = true;
            mouseCaptured = false;
            glutSetCursor(GLUT_CURSOR_INHERIT);
            printStatus(STATUS_WIN);
        }
    }
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
    static bool firstFrame = true;
    if (firstFrame) {
        firstFrame = false;
        std::cout << "First frame start" << std::endl;
    
    }
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    if (deltaTime > 0.05f)
        deltaTime = 0.05f;

    processMovement(deltaTime);
    updateGameplayLogic();

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

    glUniform1i(
        glGetUniformLocation(shaderProgram, "lightEnabled"),
        lightEnabled ? 1 : 0
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"),
        0
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

            bool isDoorCell = (row == doorCell.x && col == doorCell.y);

            if (maze[row][col] == '#' && !isDoorCell)
            {
                glBindTexture(GL_TEXTURE_2D, wallTexture);

                drawCube(
                    glm::vec3(col + 0.5f, 1.0f, row + 0.5f),
                    glm::vec3(1.0f, 2.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f)
                );
            }
        }
    }

    // Draw door at exitPos
    drawDoor();

    // Draw golden key if not collected yet
    if (!hasKey)
    {
        drawKey(currentTime / 1000.0f);
    }

    if (gameWon)
    {
        drawWinOverlay();
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
    
    if (key == 'l') {
        lightEnabled = !lightEnabled;
        std::cout << "Light toggled: " << (lightEnabled ? "Light : ON" : "Light : OFF") << std::endl;
    }

    if (key == 'r') {
        resetGame();
    }
}

void keyboardUp(unsigned char key, int x, int y)
{
    if (key >= 'A' && key <= 'Z')
        key = key - 'A' + 'a';

    keys[key] = false;
}

void mouseMotion(int x, int y)
{
    if (gameWon || !mouseCaptured)
        return;

    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;

    if (firstMouse) {
        firstMouse = false;
        glutWarpPointer(centerX, centerY);
        return;
    }

    if (x == centerX && y == centerY)
        return;

    float xOffset = (x - centerX) * mouseSensitivity;
    float yOffset = (centerY - y) * mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraDirection();
    glutWarpPointer(centerX, centerY);
}

void drawBitmapText(float xPx, float yPx, const char* text, void* font = GLUT_BITMAP_HELVETICA_18)
{
    float ndcX = (xPx / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
    float ndcY = 1.0f - (yPx / static_cast<float>(windowHeight)) * 2.0f;

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(ndcX, ndcY);
    for (const char* c = text; *c != '\0'; ++c) {
        glutBitmapCharacter(font, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
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
    std::cout << "Before keyTexture" << std::endl;
    keyTexture = loadTexture("assets/textures/key.jpg");
    std::cout << "After keyTexture" << std::endl;

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    printStatus(STATUS_FIND_KEY, true);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(idle);

    glutIgnoreKeyRepeat(1);
    std::cout << "Before cursor/warp" << std::endl;
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(windowWidth / 2, windowHeight / 2);

    std::cout << "Before randomize" << std::endl;
    randomizeKeyAndExit();
    std::cout << "After randomize" << std::endl;

    glutMouseFunc(mouseClick);
    std::cout << "Entering glutMainLoop" << std::endl;
    glutMainLoop();

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteProgram(shaderProgram);

    return 0;
}