#include <iostream>
#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

const char* vertexShaderSrc = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
void main(){
    gl_Position = vec4(aPos, 1.0);
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
out vec4 FragColor;
void main(){
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // RED
}
)glsl";

void checkCompile(GLuint shader) {
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512]; glGetShaderInfoLog(shader, 512, NULL, buf);
        std::cerr << "SHADER COMPILE ERROR:\n" << buf << std::endl;
    }
}

int main(){
    if (!glfwInit()){ std::cerr << "Failed to init GLFW\n"; return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 

    GLFWwindow* win = glfwCreateWindow(800, 600, "Red Triangle", NULL, NULL);
    if (!win){ std::cerr << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr << "Failed to initialize GLAD\n"; return -1;
    }

    float vertices[] = {
       -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSrc,NULL);
    glCompileShader(vs); checkCompile(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSrc,NULL);
    glCompileShader(fs); checkCompile(fs);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs); glDeleteShader(fs);

    while (!glfwWindowShouldClose(win)){
        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}