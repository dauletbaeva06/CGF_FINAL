#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <queue>
#include <cstdlib>
#include <map>
#include <array>
#include <cctype>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb/stb_image.h"


int windowWidth = 1280;
int windowHeight = 720;


GLuint shaderProgram = 0;
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
GLuint wallTexture = 0;
GLuint floorTexture = 0;
GLuint keyTexture = 0;

const glm::vec3 startPos(1.5f, 0.5f, 1.5f);
glm::vec3 cameraPos = startPos;
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
glm::vec3 movableLightPos(3.5f, 2.0f, 3.5f);
float lightMoveSpeed = 3.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float cameraSpeed = 2.5f;
bool freeCamera = false;
bool manualLightMode = false;
glm::vec3 savedCameraPos;
float savedYaw = -90.0f;
float savedPitch = 0.0f;
float mouseSensitivity = 0.12f;

bool keys[256] = {};
bool firstMouse = true;
bool mouseCaptured = true;
bool lightEnabled = true;
bool showHelp = false;
bool hasKey = false;
bool gameWon = false; 
bool wasInFrontOfDoor = false;
bool gamePaused = false;
float animationTime = 0.0f;
bool gameLost = false;
const float GAME_TIME_LIMIT =90.0f;
enum GameStatus {
    STATUS_FIND_KEY,
    STATUS_KEY_COLLECTED,
    STATUS_WIN
};
GameStatus lastPrintedStatus = STATUS_FIND_KEY;
bool statusInitialized = false;

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
float gameTime = 0.0f;

int fpsFrames = 0;
int fpsLastTime = 0;
float currentFPS = 0.0f;
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
    movableLightPos = glm::vec3(3.5f, 2.0f, 3.5f);
 
    cameraPos = startPos;
    yaw = -90.0f;
    pitch = 0.0f;
    updateCameraDirection();
    manualLightMode = false;
    movableLightPos = glm::vec3(3.5f, 2.0f, 3.5f);

    hasKey = false;
    gameWon = false;
    wasInFrontOfDoor = false;
    gamePaused = false;
    animationTime = 0.0f;
    freeCamera = false;
    savedCameraPos = startPos;
    savedYaw = -90.0f;
    savedPitch = 0.0f;

    gameTime = 0.0f;
    lastTime = glutGet(GLUT_ELAPSED_TIME);

    fpsFrames = 0;
    fpsLastTime = lastTime;
    currentFPS = 0.0f;

    std::fill(std::begin(keys), std::end(keys), false);

    lightEnabled = true;

    /
    firstMouse = true;
    mouseCaptured = true;

    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(windowWidth / 2, windowHeight / 2);

    randomizeKeyAndExit();

    statusInitialized = false;
    printStatus(STATUS_FIND_KEY, true);

    std::cout << "Game restarted!" << std::endl;
}


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

std::vector<glm::ivec2> getReachableCells()
{
    std::vector<glm::ivec2> result;

    const int rows = static_cast<int>(maze.size());
    const int startRow = static_cast<int>(startPos.z);
    const int startCol = static_cast<int>(startPos.x);

    std::vector<std::vector<bool>> visited(rows);

    for (int r = 0; r < rows; ++r)
        visited[r].resize(maze[r].size(), false);

    std::queue<glm::ivec2> q;

    q.push(glm::ivec2(startRow, startCol));
    visited[startRow][startCol] = true;

    const int dr[4] = { -1, 1, 0, 0 };
    const int dc[4] = { 0, 0, -1, 1 };

    while (!q.empty())
    {
        glm::ivec2 current = q.front();
        q.pop();

        int row = current.x;
        int col = current.y;

        result.push_back(current);

        for (int d = 0; d < 4; ++d)
        {
            int nr = row + dr[d];
            int nc = col + dc[d];

            if (nr < 0 || nr >= rows)
                continue;

            if (nc < 0 || nc >= static_cast<int>(maze[nr].size()))
                continue;

            if (visited[nr][nc] || maze[nr][nc] == '#')
                continue;

            visited[nr][nc] = true;
            q.push(glm::ivec2(nr, nc));
        }
    }

    return result;
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
            std::vector<glm::ivec2> reachable = getReachableCells();
            if (cell == '.' &&
                glm::length(center - glm::vec2(startPos.x, startPos.z)) > 2.0f)
            {
                bool canReach = false;

                for (const auto& pos : reachable)
                {
                    if (pos.x == row && pos.y == col)
                    {
                        canReach = true;
                        break;
                    }
                }

                if (canReach)
                    freeCells.push_back(center);
            }

            if (cell == '#' && row > 0 && row < rows - 1 && col > 0 && col < cols - 1) {
                const int dr[4] = { -1, 1, 0, 0 };
                const int dc[4] = { 0, 0, -1, 1 };
                for (int d = 0; d < 4; ++d) {
                    int nr = row + dr[d];
                    int nc = col + dc[d];
                    int oppositeRow = row - dr[d];
                    int oppositeCol = col - dc[d];

                    if (maze[nr][nc] == '.' &&
                        maze[oppositeRow][oppositeCol] == '.') {
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


void drawBlockLabel(
    float centerX,
    float centerY,
    float height,
    const std::string& text,
    const glm::vec3& color,
    float z = -0.5f)
{
    static const std::map<char, std::array<unsigned char, 7>> font = {
        {'A', {14,17,17,31,17,17,17}},
        {'B', {30,17,17,30,17,17,30}},
        {'C', {14,17,16,16,16,17,14}},
        {'D', {30,17,17,17,17,17,30}},
        {'E', {31,16,16,30,16,16,31}},
        {'F', {31,16,16,30,16,16,16}},
        {'G', {14,17,16,23,17,17,14}},
        {'H', {17,17,17,31,17,17,17}},
        {'I', {31,4,4,4,4,4,31}},
        {'J', {7,2,2,2,18,18,12}},
        {'K', {17,18,20,24,20,18,17}},
        {'L', {16,16,16,16,16,16,31}},
        {'M', {17,27,21,21,17,17,17}},
        {'N', {17,25,21,19,17,17,17}},
        {'O', {14,17,17,17,17,17,14}},
        {'P', {30,17,17,30,16,16,16}},
        {'Q', {14,17,17,17,21,18,13}},
        {'R', {30,17,17,30,20,18,17}},
        {'S', {15,16,16,14,1,1,30}},
        {'T', {31,4,4,4,4,4,4}},
        {'U', {17,17,17,17,17,17,14}},
        {'V', {17,17,17,17,17,10,4}},
        {'W', {17,17,17,21,21,21,10}},
        {'X', {17,17,10,4,10,17,17}},
        {'Y', {17,17,10,4,4,4,4}},
        {'Z', {31,1,2,4,8,16,31}},

        {'0', {14,17,19,21,25,17,14}},
        {'1', {4,12,4,4,4,4,14}},
        {'2', {14,17,1,2,4,8,31}},
        {'3', {30,1,1,14,1,1,30}},
        {'4', {2,6,10,18,31,2,2}},
        {'5', {31,16,16,30,1,1,30}},
        {'6', {14,16,16,30,17,17,14}},
        {'7', {31,1,2,4,8,8,8}},
        {'8', {14,17,17,14,17,17,14}},
        {'9', {14,17,17,15,1,1,14}},

        {':', {0,4,4,0,4,4,0}},
        {'-', {0,0,0,31,0,0,0}},
        {'.', {0,0,0,0,0,12,12}},
        {'/', {1,1,2,4,8,16,16}},
        {'!', {4,4,4,4,4,0,4}},
        {'?', {14,17,1,2,4,0,4}}
    };

    const float pixel = height / 7.0f;
    const float glyphWidth = pixel * 5.0f;
    const float gap = pixel * 2.0f;
    const float advance = glyphWidth + gap;

    if (text.empty())
        return;

    const float totalWidth =
        text.size() * glyphWidth +
        (text.size() - 1) * gap;

    const float startX = centerX - totalWidth / 2.0f;
    const float startY = centerY - height / 2.0f;

    for (size_t i = 0; i < text.size(); ++i)
    {
        unsigned char ch = static_cast<unsigned char>(text[i]);
        ch = static_cast<unsigned char>(std::toupper(ch));

        if (ch == ' ')
            continue;

        auto it = font.find(static_cast<char>(ch));

        if (it == font.end())
            it = font.find('?');

        const auto& rows = it->second;

        for (int row = 0; row < 7; ++row)
        {
            for (int col = 0; col < 5; ++col)
            {
                bool filled = (rows[row] & (1 << (4 - col))) != 0;

                if (!filled)
                    continue;

                float px = startX + i * advance +
                    col * pixel + pixel * 0.5f;

                float py = startY +
                    row * pixel + pixel * 0.5f;

                draw2DRect(
                    px, py,
                    pixel * 0.92f,
                    pixel * 0.92f,
                    color,
                    z
                );
            }
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

void drawLoseOverlay()
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 orthoProj = glm::ortho(
        0.0f, static_cast<float>(windowWidth),
        static_cast<float>(windowHeight), 0.0f,
        -10.0f, 10.0f
    );

    glm::mat4 identityView(1.0f);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(orthoProj)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(identityView)
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 1
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    float cx = windowWidth * 0.5f;
    float cy = windowHeight * 0.5f;

    draw2DRect(
        cx, cy, 460.0f, 240.0f,
        glm::vec3(0.14f, 0.18f, 0.30f), -2.0f
    );

    draw2DRect(
        cx, cy - 55.0f, 320.0f, 52.0f,
        glm::vec3(0.75f, 0.15f, 0.15f), -1.0f
    );

    drawBlockLabel(
        cx, cy - 55.0f, 30.0f,
        "TIME UP", glm::vec3(1.0f), -0.5f
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 0
    );

    glEnable(GL_DEPTH_TEST);
}

void mouseClick(int button, int state, int x, int y)
{
    if (!gameWon && !gameLost)
        return;

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        ButtonRect btn = getPlayAgainButton();
        if (isPointInButton(x, y, btn)) {
            resetGame();
        }
    }
}

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
    
    if (isUI) {
    FragColor = vec4(objectColor, 1.0);
    return;
}

    // Blinn-Phong OFF: show original texture color
    if (!lightEnabled) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    // Blinn-Phong ON: apply calculated lighting
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

        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

         0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
         0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,

         -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,

          0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
          0.5f,-0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,

          -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
          -0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,

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

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        8 * sizeof(float), (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        8 * sizeof(float),
        (void*)(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

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
    drawKeyPart(center, glm::vec3(0.0f, 0.07f, ringZ), glm::vec3(0.05f, 0.035f, 0.14f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f, -0.07f, ringZ), glm::vec3(0.05f, 0.035f, 0.14f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f, 0.0f, ringZ - 0.055f), glm::vec3(0.05f, 0.17f, 0.035f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f, 0.0f, ringZ + 0.055f), glm::vec3(0.05f, 0.17f, 0.035f), gold, spinDeg);

    drawKeyPart(center, glm::vec3(0.0f, 0.0f, 0.02f), glm::vec3(0.045f, 0.045f, 0.30f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f, 0.0f, -0.06f), glm::vec3(0.052f, 0.055f, 0.025f), darkGold, spinDeg);

    drawKeyPart(center, glm::vec3(0.0f, -0.055f, 0.13f), glm::vec3(0.04f, 0.08f, 0.035f), gold, spinDeg);
    drawKeyPart(center, glm::vec3(0.0f, -0.045f, 0.07f), glm::vec3(0.04f, 0.06f, 0.035f), gold, spinDeg);
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
    drawCube(toWorld(glm::vec3(0.36f, exitPos.y + 0.75f, 0.0f)), glm::vec3(0.12f, 1.50f, 0.16f), frameColor, yawDeg);

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

    if (row == doorCell.x && col == doorCell.y)
    {

        return !hasKey;
    }

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
    if (gameWon || gameLost || freeCamera)
        return;

    glm::vec2 playerXZ(cameraPos.x, cameraPos.z);
    glm::vec2 keyXZ(keyPos.x, keyPos.z);

    if (!hasKey && glm::length(playerXZ - keyXZ) <= keyPickupRadius)
    {
        hasKey = true;
        printStatus(STATUS_KEY_COLLECTED);
    }



    if (hasKey && doorCell.x >= 0)
    {
        glm::vec2 doorXZ(exitPos.x, exitPos.z);
        glm::vec2 playerFromDoor = playerXZ - doorXZ;

        float side = glm::dot(playerFromDoor, doorOpenDir);
        float distanceToDoor = glm::length(playerFromDoor);

        if (side > 0.35f && distanceToDoor < 1.25f)
        {
            wasInFrontOfDoor = true;
        }

        if (wasInFrontOfDoor &&
            side < -0.35f &&
            distanceToDoor < 1.0f)
        {
            gameWon = true;
            mouseCaptured = false;

            glutSetCursor(GLUT_CURSOR_INHERIT);
            printStatus(STATUS_WIN);
        }
    }

}



void processMovement(float deltaTime)
{
    if (manualLightMode)
    {
        float step = lightMoveSpeed * deltaTime;

        if (keys['j']) movableLightPos.x -= step;
        if (keys[';']) movableLightPos.x += step;
        if (keys['i']) movableLightPos.z -= step;
        if (keys['k']) movableLightPos.z += step;
        if (keys['u']) movableLightPos.y += step;
        if (keys['o']) movableLightPos.y -= step;
    }
    if (freeCamera)
    {
        float velocity = cameraSpeed * deltaTime * 2.0f;

        glm::vec3 right = glm::normalize(
            glm::cross(cameraFront, cameraUp)
        );

        glm::vec3 movement(0.0f);

        if (keys['w']) movement += cameraFront;
        if (keys['s']) movement -= cameraFront;

        if (keys['a']) movement -= right;
        if (keys['d']) movement += right;

        if (keys['e']) movement += cameraUp;
        if (keys['q']) movement -= cameraUp;

        if (glm::length(movement) > 0.0f)
        {
            movement = glm::normalize(movement);
            cameraPos += movement * velocity;
        }

        return;
    }

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

void drawStatusHUD()
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 projection2D = glm::ortho(
        0.0f, static_cast<float>(windowWidth),
        static_cast<float>(windowHeight), 0.0f,
        -10.0f, 10.0f
    );

    glm::mat4 identity(1.0f);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection2D)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(identity)
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 1
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    const float panelWidth = 310.0f;
    const float panelHeight = 230.0f;
    const float panelX = windowWidth - panelWidth / 2.0f - 20.0f;
    const float panelY = panelHeight / 2.0f + 20.0f;

    draw2DRect(
        panelX, panelY,
        panelWidth, panelHeight,
        glm::vec3(0.08f, 0.09f, 0.13f),
        -2.0f
    );

    std::string fpsText =
        "FPS: " + std::to_string(static_cast<int>(currentFPS));

    std::string keyText =
        hasKey ? "KEY: FOUND" : "KEY: NOT FOUND";

    std::string shaderText =
        lightEnabled ? "SHADER: ON" : "SHADER: OFF";

    std::string cameraText =
        freeCamera ? "CAMERA: FREE" : "CAMERA: GAME";

    std::string lightText =
        manualLightMode ? "LIGHT: MANUAL" : "LIGHT: PLAYER";

    std::string statusText =
        gameWon ? "STATUS: WIN" :
        gameLost ? "STATUS: LOST" :
        gamePaused ? "STATUS: PAUSED" :
        "STATUS: PLAYING";

    const float textHeight = 13.0f;
    const float lineSpacing = 34.0f;
    const float firstY = panelY - 85.0f;

    glm::vec3 white(1.0f);
    glm::vec3 gold(1.0f, 0.85f, 0.45f);
    glm::vec3 green(0.35f, 1.0f, 0.45f);

    drawBlockLabel(
        panelX, firstY,
        textHeight, fpsText, white
    );

    drawBlockLabel(
        panelX, firstY + lineSpacing,
        textHeight, keyText,
        hasKey ? green : white
    );

    drawBlockLabel(
        panelX, firstY + lineSpacing * 2,
        textHeight, shaderText, gold
    );

    drawBlockLabel(
        panelX, firstY + lineSpacing * 3,
        textHeight, cameraText, white
    );

    drawBlockLabel(
        panelX, firstY + lineSpacing * 4,
        textHeight, lightText, white
    );

    drawBlockLabel(
        panelX, firstY + lineSpacing * 5,
        textHeight, statusText,
        gamePaused ? gold : white
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 0
    );

    glEnable(GL_DEPTH_TEST);
}

void drawTimerHUD()
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 projection2D = glm::ortho(
        0.0f, static_cast<float>(windowWidth),
        static_cast<float>(windowHeight), 0.0f,
        -10.0f, 10.0f
    );

    glm::mat4 identity(1.0f);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection2D)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(identity)
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 1
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    draw2DRect(
        125.0f, 50.0f,
        220.0f, 65.0f,
        glm::vec3(0.10f, 0.10f, 0.13f),
        -2.0f
    );

    int remaining = static_cast<int>(
        std::ceil(std::max(0.0f, GAME_TIME_LIMIT - gameTime))
        );

    int minutes = remaining / 60;
    int seconds = remaining % 60;

    std::string timerText =
        std::to_string(minutes) + ":" +
        (seconds < 10 ? "0" : "") +
        std::to_string(seconds);

    glm::vec3 timerColor =
        remaining <= 30
        ? glm::vec3(1.0f, 0.25f, 0.25f)
        : glm::vec3(1.0f, 0.85f, 0.45f);

    drawBlockLabel(
        125.0f, 50.0f,
        28.0f,
        timerText,
        timerColor,
        -0.5f
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 0
    );

    glEnable(GL_DEPTH_TEST);
}


void drawHelpOverlay()
{
    if (!showHelp)
        return;

    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    glm::mat4 projection2D = glm::ortho(
        0.0f, static_cast<float>(windowWidth),
        static_cast<float>(windowHeight), 0.0f,
        -10.0f, 10.0f
    );

    glm::mat4 identity(1.0f);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection2D)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(identity)
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 1
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    float cx = windowWidth * 0.5f;
    float cy = windowHeight * 0.5f;

    draw2DRect(
        cx, cy, 620.0f, 560.0f,
        glm::vec3(0.08f, 0.09f, 0.13f),
        -2.0f
    );

    draw2DRect(
        cx, cy - 225.0f, 550.0f, 55.0f,
        glm::vec3(0.35f, 0.25f, 0.12f),
        -1.0f
    );

    glm::vec3 white(1.0f);
    glm::vec3 gold(1.0f, 0.85f, 0.45f);

    drawBlockLabel(cx, cy - 225.0f, 27.0f,
        "CONTROLS", gold);

    const float fontSize = 16.0f;
    const float spacing = 42.0f;
    const float firstY = cy - 160.0f;

    drawBlockLabel(cx, firstY, fontSize,
        "WASD MOVE", white);

    drawBlockLabel(cx, firstY + spacing, fontSize,
        "MOUSE LOOK", white);

    drawBlockLabel(cx, firstY + spacing * 2, fontSize,
        "F FREE CAMERA", white);

    drawBlockLabel(cx, firstY + spacing * 3, fontSize,
        "Q E CAMERA UP DOWN", white);

    drawBlockLabel(cx, firstY + spacing * 4, fontSize,
        "L SHADER ON OFF", white);

    drawBlockLabel(cx, firstY + spacing * 5, fontSize,
        "T LIGHT MODE", white);

    drawBlockLabel(cx, firstY + spacing * 6, fontSize,
        "I J K U O LIGHT MOVE", white);

    drawBlockLabel(cx, firstY + spacing * 7, fontSize,
        "P PAUSE", white);

    drawBlockLabel(cx, firstY + spacing * 8, fontSize,
        "R RESTART", white);

    drawBlockLabel(cx, firstY + spacing * 9, fontSize,
        "H CLOSE HELP", gold);

    glUniform1i(
        glGetUniformLocation(shaderProgram, "isUI"), 0
    );

    glEnable(GL_DEPTH_TEST);
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

    fpsFrames++;

    int fpsElapsed = currentTime - fpsLastTime;

    if (fpsElapsed >= 1000)
    {
        currentFPS = fpsFrames * 1000.0f / fpsElapsed;

        fpsFrames = 0;
        fpsLastTime = currentTime;
    }

    if (!gameWon && !gameLost && !gamePaused)
    {
        animationTime += deltaTime;
        gameTime += deltaTime;

        if (gameTime >= GAME_TIME_LIMIT)
        {
            gameTime = GAME_TIME_LIMIT;
            gameLost = true;

            std::cout << "TIME UP! You lost!" << std::endl;
        }
        else
        {
            processMovement(deltaTime);
            updateGameplayLogic();
        }
    }

    glClearColor(0.08f, 0.09f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glm::vec3 lightPos = manualLightMode
        ? movableLightPos
        : cameraPos + glm::vec3(0.0f, 0.5f, 0.0f);
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


    glActiveTexture(GL_TEXTURE0);

    glUniform1i(
        glGetUniformLocation(shaderProgram, "ourTexture"),
        0
    );

    for (int row = 0; row < static_cast<int>(maze.size()); row++)
    {
        for (int col = 0; col < static_cast<int>(maze[row].size()); col++)
        {

            glBindTexture(GL_TEXTURE_2D, floorTexture);

            drawCube(
                glm::vec3(col + 0.5f, -0.08f, row + 0.5f),
                glm::vec3(1.0f, 0.16f, 1.0f),
                glm::vec3(1.0f, 1.0f, 1.0f)
            );

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

    drawDoor();

    if (!hasKey)
    {
        drawKey(animationTime);
    }
    drawTimerHUD();
    drawStatusHUD();
    if (gameWon)
    {
        drawWinOverlay();
    }
    if (gameLost)
    {
        drawLoseOverlay();
    }
    drawHelpOverlay();
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
    if (key == 't')
    {
        manualLightMode = !manualLightMode;

        std::cout << "Light mode: "
            << (manualLightMode ? "MANUAL" : "PLAYER")
            << std::endl;
    }
    if (key >= 'A' && key <= 'Z')
        key = key - 'A' + 'a';

    keys[key] = true;

    if (key == 'l') {
        lightEnabled = !lightEnabled;
        std::cout << "Light toggled: " << (lightEnabled ? "Light : ON" : "Light : OFF") << std::endl;
    }
    if (key == 'h')
    {
        showHelp = !showHelp;

        std::cout << "Help: "
            << (showHelp ? "ON" : "OFF")
            << std::endl;
    }
    if (key == 'p' && !gameWon && !gameLost)
    {
        gamePaused = !gamePaused;

        keys['w'] = false;
        keys['a'] = false;
        keys['s'] = false;
        keys['d'] = false;

        std::cout << (gamePaused ? "PAUSED" : "RESUMED")
            << std::endl;
    }
    if (key == 'f' && !gameWon && !gameLost && !gamePaused)
    {
        freeCamera = !freeCamera;

        if (freeCamera)
        {
      
            savedCameraPos = cameraPos;
            savedYaw = yaw;
            savedPitch = pitch;

            std::cout << "FREE CAMERA ON" << std::endl;
        }
        else
        {

            cameraPos = savedCameraPos;
            yaw = savedYaw;
            pitch = savedPitch;

            updateCameraDirection();

            std::cout << "FREE CAMERA OFF" << std::endl;
        }

        keys['w'] = false;
        keys['a'] = false;
        keys['s'] = false;
        keys['d'] = false;
        keys['q'] = false;
        keys['e'] = false;
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
    if (gameWon || gameLost || gamePaused || !mouseCaptured)
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
    if (wallTexture == 0 || floorTexture == 0)
    {
        std::cerr << "ERROR: Failed to load game textures!" << std::endl;
        std::cerr << "Check wall and floor texture paths." << std::endl;
        return 1;
    }
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
