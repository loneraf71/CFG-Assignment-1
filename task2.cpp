#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>

const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

const char* vertexShaderSrc = R"glsl(
#version 330 core
layout (location = 0) in vec2 vPosition;
layout (location = 1) in vec3 vColor;
out vec3 fragColor;
void main() {
    gl_Position = vec4(vPosition, 0.0, 1.0);
    fragColor = vColor;
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
in vec3 fragColor;
out vec4 outColor;
void main() {
    outColor = vec4(fragColor, 1.0);
}
)glsl";

GLuint compileShaderProgram() {
    auto compile = [](const char* src, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(shader, 512, nullptr, info);
            std::cerr << "Shader compile error:\n" << info << std::endl;
        }
        return shader;
    };

    GLuint vert = compile(vertexShaderSrc, GL_VERTEX_SHADER);
    GLuint frag = compile(fragmentShaderSrc, GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void createEllipse(std::vector<float>& data, int segments = 50) {
    float cx = -0.6f, cy = 0.5f;
    float rx = 0.2f, ry = 0.15f;
    data.insert(data.end(), {cx, cy, 1.0f, 0.0f, 0.0f});
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + std::cos(angle) * rx;
        float y = cy + std::sin(angle) * ry * 0.6f;
        data.insert(data.end(), {x, y, 1.0f, 0.0f, 0.0f});
    }
}

void createTriangle(std::vector<float>& data) {
    float size = 0.3f;
    float height = size * std::sqrt(3.0f) / 2.0f;
    data.insert(data.end(), {0.0f, 0.5f + height / 2, 1.0f, 0.0f, 0.0f});
    data.insert(data.end(), {-size / 2, 0.5f - height / 2, 0.0f, 1.0f, 0.0f});
    data.insert(data.end(), {size / 2, 0.5f - height / 2, 0.0f, 0.0f, 1.0f});
}

void createCircle(std::vector<float>& data, int segments = 50) {
    float cx = 0.6f, cy = 0.5f, radius = 0.18f;
    data.insert(data.end(), {cx, cy, 1.0f, 0.0f, 0.0f});
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + std::cos(angle) * radius;
        float y = cy + std::sin(angle) * radius;
        float r, g, b;
        if (angle >= M_PI && angle <= 1.5f * M_PI) {
            r = g = b = 0.0f;
        } else {
            float normalized = angle / (2 * M_PI);
            float grad = (normalized > 0.75f || normalized < 0.25f) ? 0.3f : 0.7f;
            r = grad; g = 0.0f; b = 0.0f;
        }
        data.insert(data.end(), {x, y, r, g, b});
    }
}

void createNestedSquares(std::vector<float>& data) {
    std::vector<float> sizes = {0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f};
    float cx = 0.0f, cy = -0.3f;
    for (size_t i = 0; i < sizes.size(); i++) {
        float size = sizes[i];
        float half = size / 2.0f;
        std::vector<float> verts;
        for (int j = 0; j < 4; j++) {
            float angle = M_PI / 4 + j * M_PI / 2;
            float x = cx + std::cos(angle) * half;
            float y = cy + std::sin(angle) * half;
            verts.push_back(x);
            verts.push_back(y);
        }
        int order[4] = {0, 1, 3, 2};
        float r = (i % 2 == 0) ? 1.0f : 0.0f;
        float g = r, b = r;
        for (int idx : order)
            data.insert(data.end(), {verts[idx * 2], verts[idx * 2 + 1], r, g, b});
    }
}

struct Shape {
    GLuint vao, vbo;
    GLsizei vertexCount;
    GLenum mode;
};

Shape setupVAO(const std::vector<float>& data, GLenum mode) {
    Shape s{};
    glGenVertexArrays(1, &s.vao);
    glGenBuffers(1, &s.vbo);
    s.vertexCount = data.size() / 5;
    s.mode = mode;

    glBindVertexArray(s.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return s;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for macOS

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Task 2 - Four Objects Scene", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    GLuint program = compileShaderProgram();
    glUseProgram(program);

    std::vector<float> ellipseData, triData, circleData, squaresData;
    createEllipse(ellipseData);
    createTriangle(triData);
    createCircle(circleData);
    createNestedSquares(squaresData);

    Shape ellipse = setupVAO(ellipseData, GL_TRIANGLE_FAN);
    Shape triangle = setupVAO(triData, GL_TRIANGLES);
    Shape circle = setupVAO(circleData, GL_TRIANGLE_FAN);
    Shape squares = setupVAO(squaresData, GL_TRIANGLE_STRIP);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        glBindVertexArray(ellipse.vao);
        glDrawArrays(ellipse.mode, 0, ellipse.vertexCount);

        glBindVertexArray(triangle.vao);
        glDrawArrays(triangle.mode, 0, triangle.vertexCount);

        glBindVertexArray(circle.vao);
        glDrawArrays(circle.mode, 0, circle.vertexCount);

        glBindVertexArray(squares.vao);
        for (int i = 0; i < 6; i++)
            glDrawArrays(squares.mode, i * 4, 4);

        glfwSwapBuffers(window);
    }

    GLuint vaos[] = {ellipse.vao, triangle.vao, circle.vao, squares.vao};
    GLuint vbos[] = {ellipse.vbo, triangle.vbo, circle.vbo, squares.vbo};
    glDeleteVertexArrays(4, vaos);
    glDeleteBuffers(4, vbos);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}