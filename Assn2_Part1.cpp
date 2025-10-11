#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <map>
#include <random>
#include <string>

const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// Global variables for animation and state
bool animationEnabled = true;
float squareRotation = 0.0f;
float triangleRotation = 0.0f;
float circleScale = 1.0f;
bool circleGrowing = true;
float circleColor[3] = {1.0f, 1.0f, 1.0f}; // White by default

// Square colors
enum SquareColor { WHITE, RED, GREEN };
SquareColor currentSquareColor = WHITE;

// Window references (declared early for callbacks)
GLFWwindow* mainWindow = nullptr;
GLFWwindow* subWindow = nullptr;
GLFWwindow* window2 = nullptr;

// Subwindow background color
float subWindowBgColor[3] = {0.2f, 0.2f, 0.5f}; // Blue-gray

// Menu state
bool showMenu = false;
float menuX = 0.0f, menuY = 0.0f;
const float MENU_WIDTH = 0.4f;
const float MENU_HEIGHT = 0.3f;
const float ITEM_HEIGHT = 0.07f;

struct MenuItem {
    std::string label;
    float yPosition;
    bool isSubmenu;
    int action;
    float color[3]; // Color for the menu item block
};

std::vector<MenuItem> menuItems;
bool squareColorsSubmenu = false;

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
    
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(prog, 512, nullptr, info);
        std::cerr << "Program link error:\n" << info << std::endl;
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void createEllipse(std::vector<float>& data, int segments = 50) {
    float cx = 0.0f, cy = 0.0f;
    float rx = 0.2f, ry = 0.15f;
    data.insert(data.end(), {cx, cy, 1.0f, 0.0f, 0.0f});
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + std::cos(angle) * rx;
        float y = cy + std::sin(angle) * ry;
        data.insert(data.end(), {x, y, 1.0f, 0.0f, 0.0f});
    }
}

void createTriangle(std::vector<float>& data, float rotation = 0.0f, float offsetX = 0.0f, float offsetY = 0.0f) {
    float size = 0.3f;
    float height = size * std::sqrt(3.0f) / 2.0f;
    
    float cosRot = std::cos(rotation);
    float sinRot = std::sin(rotation);
    
    // Top vertex
    float x1 = 0.0f, y1 = height / 2;
    float rotatedX1 = x1 * cosRot - y1 * sinRot + offsetX;
    float rotatedY1 = x1 * sinRot + y1 * cosRot + offsetY;
    
    // Bottom left vertex
    float x2 = -size / 2, y2 = -height / 2;
    float rotatedX2 = x2 * cosRot - y2 * sinRot + offsetX;
    float rotatedY2 = x2 * sinRot + y2 * cosRot + offsetY;
    
    // Bottom right vertex
    float x3 = size / 2, y3 = -height / 2;
    float rotatedX3 = x3 * cosRot - y3 * sinRot + offsetX;
    float rotatedY3 = x3 * sinRot + y3 * cosRot + offsetY;
    
    data.insert(data.end(), {rotatedX1, rotatedY1, circleColor[0], circleColor[1], circleColor[2]});
    data.insert(data.end(), {rotatedX2, rotatedY2, circleColor[0], circleColor[1], circleColor[2]});
    data.insert(data.end(), {rotatedX3, rotatedY3, circleColor[0], circleColor[1], circleColor[2]});
}

void createCircle(std::vector<float>& data, float scale = 1.0f, int segments = 50, float offsetX = 0.0f, float offsetY = 0.0f) {
    float cx = offsetX, cy = offsetY, radius = 0.18f * scale;
    data.insert(data.end(), {cx, cy, circleColor[0], circleColor[1], circleColor[2]});
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + std::cos(angle) * radius;
        float y = cy + std::sin(angle) * radius;
        data.insert(data.end(), {x, y, circleColor[0], circleColor[1], circleColor[2]});
    }
}

void createNestedSquares(std::vector<float>& data, float rotation = 0.0f) {
    std::vector<float> sizes = {0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f};
    float cx = 0.0f, cy = 0.0f;
    
    float cosRot = std::cos(rotation);
    float sinRot = std::sin(rotation);
    
    for (size_t i = 0; i < sizes.size(); i++) {
        float size = sizes[i];
        float half = size / 2.0f;
        std::vector<float> verts;
        for (int j = 0; j < 4; j++) {
            float angle = M_PI / 4 + j * M_PI / 2;
            float x = std::cos(angle) * half;
            float y = std::sin(angle) * half;
            
            // Apply rotation
            float rotatedX = x * cosRot - y * sinRot;
            float rotatedY = x * sinRot + y * cosRot;
            
            verts.push_back(rotatedX + cx);
            verts.push_back(rotatedY + cy);
        }
        int order[4] = {0, 1, 3, 2};
        
        // Set colors based on currentSquareColor
        float r, g, b;
        if (currentSquareColor == WHITE) {
            r = g = b = (i % 2 == 0) ? 1.0f : 0.0f;
        } else if (currentSquareColor == RED) {
            r = (i % 2 == 0) ? 1.0f : 0.5f;
            g = b = 0.0f;
        } else if (currentSquareColor == GREEN) {
            g = (i % 2 == 0) ? 1.0f : 0.5f;
            r = b = 0.0f;
        }
        
        for (int idx : order)
            data.insert(data.end(), {verts[idx * 2], verts[idx * 2 + 1], r, g, b});
    }
}

// Initialize menu items with color blocks
void initMenu() {
    menuItems.clear();
    if (!squareColorsSubmenu) {
        // Main menu - using meaningful colors for each action
        menuItems.push_back({"Stop", 0.08f, false, 1, {1.0f, 0.0f, 0.0f}});        // Red for stop
        menuItems.push_back({"Start", 0.0f, false, 2, {0.0f, 1.0f, 0.0f}});        // Green for start
        menuItems.push_back({"Colors", -0.08f, true, 3, {0.0f, 0.0f, 1.0f}});      // Blue for colors submenu
    } else {
        // Square colors submenu - using the actual colors
        menuItems.push_back({"White", 0.08f, false, 4, {1.0f, 1.0f, 1.0f}});       // White
        menuItems.push_back({"Red", 0.0f, false, 5, {1.0f, 0.0f, 0.0f}});          // Red
        menuItems.push_back({"Green", -0.08f, false, 6, {0.0f, 1.0f, 0.0f}});      // Green
        menuItems.push_back({"Back", -0.16f, false, 7, {0.5f, 0.5f, 0.5f}});       // Gray for back
    }
}

// Function to draw menu with color blocks only (no text)
void drawMenu() {
    if (!showMenu) return;
    
    std::vector<float> menuData;
    
    // Menu background position
    float x1 = menuX - MENU_WIDTH/2;
    float x2 = menuX + MENU_WIDTH/2;
    float y1 = menuY - MENU_HEIGHT/2;
    float y2 = menuY + MENU_HEIGHT/2;
    
    // Background rectangle (semi-transparent dark gray)
    menuData.insert(menuData.end(), {x1, y1, 0.2f, 0.2f, 0.2f});
    menuData.insert(menuData.end(), {x2, y1, 0.2f, 0.2f, 0.2f});
    menuData.insert(menuData.end(), {x2, y2, 0.2f, 0.2f, 0.2f});
    menuData.insert(menuData.end(), {x1, y2, 0.2f, 0.2f, 0.2f});
    
    // Draw menu items as color blocks
    for (const auto& item : menuItems) {
        float itemY = menuY + item.yPosition;
        float itemY1 = itemY - ITEM_HEIGHT/2;
        float itemY2 = itemY + ITEM_HEIGHT/2;
        
        // Color block background (using the item's assigned color)
        float r = item.color[0];
        float g = item.color[1];
        float b = item.color[2];
        
        // Item background (color block)
        menuData.insert(menuData.end(), {x1, itemY1, r, g, b});
        menuData.insert(menuData.end(), {x2, itemY1, r, g, b});
        menuData.insert(menuData.end(), {x2, itemY2, r, g, b});
        menuData.insert(menuData.end(), {x1, itemY2, r, g, b});
        
        // Item border (white for visibility)
        menuData.insert(menuData.end(), {x1, itemY1, 1.0f, 1.0f, 1.0f});
        menuData.insert(menuData.end(), {x2, itemY1, 1.0f, 1.0f, 1.0f});
        menuData.insert(menuData.end(), {x2, itemY2, 1.0f, 1.0f, 1.0f});
        menuData.insert(menuData.end(), {x1, itemY2, 1.0f, 1.0f, 1.0f});
        menuData.insert(menuData.end(), {x1, itemY1, 1.0f, 1.0f, 1.0f});
    }
    
    static GLuint menuVAO = 0, menuVBO = 0;
    if (menuVAO == 0) {
        glGenVertexArrays(1, &menuVAO);
        glGenBuffers(1, &menuVBO);
    }
    
    glBindVertexArray(menuVAO);
    glBindBuffer(GL_ARRAY_BUFFER, menuVBO);
    glBufferData(GL_ARRAY_BUFFER, menuData.size() * sizeof(float), menuData.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Draw menu background
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Draw menu items
    int currentVertex = 4; // After background
    for (size_t i = 0; i < menuItems.size(); i++) {
        // Draw item color block
        glDrawArrays(GL_TRIANGLE_FAN, currentVertex, 4);
        currentVertex += 4;
        
        // Draw item border
        glDrawArrays(GL_LINE_STRIP, currentVertex, 5);
        currentVertex += 5;
    }
    
    glBindVertexArray(0);
    
    // Print menu info to console for user guidance
    if (showMenu) {
        static bool menuPrinted = false;
        if (!menuPrinted) {
            std::cout << "\n=== COLOR MENU ===" << std::endl;
            if (!squareColorsSubmenu) {
                std::cout << "RED block: Stop Animation" << std::endl;
                std::cout << "GREEN block: Start Animation" << std::endl;
                std::cout << "BLUE block: Square Colors" << std::endl;
            } else {
                std::cout << "WHITE block: White squares" << std::endl;
                std::cout << "RED block: Red squares" << std::endl;
                std::cout << "GREEN block: Green squares" << std::endl;
                std::cout << "GRAY block: Back to main menu" << std::endl;
            }
            std::cout << "Click on color blocks to select" << std::endl;
            menuPrinted = true;
        }
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

void updateVAO(Shape& shape, const std::vector<float>& data, GLenum mode) {
    shape.vertexCount = data.size() / 5;
    shape.mode = mode;
    
    glBindVertexArray(shape.vao);
    glBindBuffer(GL_ARRAY_BUFFER, shape.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

// Check if a point is inside a menu item
int getMenuItemAt(float x, float y) {
    if (!showMenu) return -1;
    
    float menuLeft = menuX - MENU_WIDTH/2;
    float menuRight = menuX + MENU_WIDTH/2;
    
    for (size_t i = 0; i < menuItems.size(); i++) {
        float itemTop = menuY + menuItems[i].yPosition + ITEM_HEIGHT/2;
        float itemBottom = menuY + menuItems[i].yPosition - ITEM_HEIGHT/2;
        
        if (x >= menuLeft && x <= menuRight && y <= itemTop && y >= itemBottom) {
            return i;
        }
    }
    return -1;
}

// Handle menu item selection
void handleMenuSelection(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= menuItems.size()) return;
    
    int action = menuItems[itemIndex].action;
    
    switch(action) {
        case 1: // Stop Animation
            animationEnabled = false;
            std::cout << "Animation STOPPED" << std::endl;
            showMenu = false;
            break;
        case 2: // Start Animation
            animationEnabled = true;
            std::cout << "Animation STARTED" << std::endl;
            showMenu = false;
            break;
        case 3: // Square Colors submenu
            squareColorsSubmenu = true;
            initMenu();
            std::cout << "Opening Square Colors submenu..." << std::endl;
            break;
        case 4: // White squares
            currentSquareColor = WHITE;
            std::cout << "Squares changed to WHITE" << std::endl;
            showMenu = false;
            squareColorsSubmenu = false;
            break;
        case 5: // Red squares
            currentSquareColor = RED;
            std::cout << "Squares changed to RED" << std::endl;
            showMenu = false;
            squareColorsSubmenu = false;
            break;
        case 6: // Green squares
            currentSquareColor = GREEN;
            std::cout << "Squares changed to GREEN" << std::endl;
            showMenu = false;
            squareColorsSubmenu = false;
            break;
        case 7: // Back from submenu
            squareColorsSubmenu = false;
            initMenu();
            std::cout << "Returning to main menu..." << std::endl;
            break;
    }
}

// Mouse click callback for main window
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (window == mainWindow) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            
            // Convert to normalized device coordinates
            float ndcX = (xpos / WINDOW_WIDTH) * 2.0f - 1.0f;
            float ndcY = 1.0f - (ypos / WINDOW_HEIGHT) * 2.0f;
            
            if (showMenu) {
                // Check if click is on a menu item
                int itemIndex = getMenuItemAt(ndcX, ndcY);
                if (itemIndex != -1) {
                    handleMenuSelection(itemIndex);
                } else {
                    // Click outside menu - close it
                    showMenu = false;
                    squareColorsSubmenu = false;
                    std::cout << "Menu closed" << std::endl;
                }
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            // Show menu at right-click position
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            
            // Convert to normalized device coordinates for menu positioning
            menuX = (xpos / WINDOW_WIDTH) * 2.0f - 1.0f;
            menuY = 1.0f - (ypos / WINDOW_HEIGHT) * 2.0f;
            
            // Initialize menu and show it
            squareColorsSubmenu = false;
            initMenu();
            showMenu = true;
            std::cout << "Color menu opened at (" << menuX << ", " << menuY << ")" << std::endl;
        }
    }
}

// Key callback for window2
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && window == window2) {
        switch(key) {
            case GLFW_KEY_R: 
                circleColor[0] = 1.0f; circleColor[1] = 0.0f; circleColor[2] = 0.0f; 
                std::cout << "Circle/Triangle color: RED" << std::endl;
                break;
            case GLFW_KEY_G: 
                circleColor[0] = 0.0f; circleColor[1] = 1.0f; circleColor[2] = 0.0f; 
                std::cout << "Circle/Triangle color: GREEN" << std::endl;
                break;
            case GLFW_KEY_B: 
                circleColor[0] = 0.0f; circleColor[1] = 0.0f; circleColor[2] = 1.0f; 
                std::cout << "Circle/Triangle color: BLUE" << std::endl;
                break;
            case GLFW_KEY_Y: 
                circleColor[0] = 1.0f; circleColor[1] = 1.0f; circleColor[2] = 0.0f; 
                std::cout << "Circle/Triangle color: YELLOW" << std::endl;
                break;
            case GLFW_KEY_O: 
                circleColor[0] = 1.0f; circleColor[1] = 0.5f; circleColor[2] = 0.0f; 
                std::cout << "Circle/Triangle color: ORANGE" << std::endl;
                break;
            case GLFW_KEY_P: 
                circleColor[0] = 0.5f; circleColor[1] = 0.0f; circleColor[2] = 0.5f; 
                std::cout << "Circle/Triangle color: PURPLE" << std::endl;
                break;
            case GLFW_KEY_W: 
                circleColor[0] = 1.0f; circleColor[1] = 1.0f; circleColor[2] = 1.0f; 
                std::cout << "Circle/Triangle color: WHITE" << std::endl;
                break;
        }
    }
}

void updateAnimations() {
    if (!animationEnabled) return;
    
    // Update square rotation (counter-clockwise)
    squareRotation += 0.01f;
    
    // Update triangle rotation (clockwise)
    triangleRotation -= 0.015f;
    
    // Update circle breathing
    if (circleGrowing) {
        circleScale += 0.01f;
        if (circleScale >= 1.5f) circleGrowing = false;
    } else {
        circleScale -= 0.01f;
        if (circleScale <= 0.5f) circleGrowing = true;
    }
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
    
    // Enable double buffering
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

    // Create main window
    mainWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Main Window - Black & White Squares (Right-click for color menu)", nullptr, nullptr);
    if (!mainWindow) {
        std::cerr << "Failed to create main window\n";
        glfwTerminate();
        return -1;
    }

    // Set up main window context first
    glfwMakeContextCurrent(mainWindow);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    // Create subwindow (ellipse window) - share context with main window
    subWindow = glfwCreateWindow(400, 300, "Ellipse SubWindow", nullptr, mainWindow);
    if (!subWindow) {
        std::cerr << "Failed to create sub window\n";
        glfwTerminate();
        return -1;
    }

    // Create window2 (circle and triangle) - share context with main window
    window2 = glfwCreateWindow(400, 300, "Window 2 - Circle & Triangle (Press R,G,B,Y,O,P,W for colors)", nullptr, mainWindow);
    if (!window2) {
        std::cerr << "Failed to create window 2\n";
        glfwTerminate();
        return -1;
    }

    // Set up callbacks
    glfwSetMouseButtonCallback(mainWindow, mouseButtonCallback);
    glfwSetKeyCallback(window2, keyCallback);

    // Compile shader program (using main window context)
    GLuint program = compileShaderProgram();

    // Create shapes data for each window
    std::vector<float> ellipseData, triData, circleData, squaresData;
    
    createEllipse(ellipseData);
    // Create triangle on the left side of window2
    createTriangle(triData, 0.0f, -0.4f, 0.0f);
    // Create circle on the right side of window2  
    createCircle(circleData, 1.0f, 50, 0.4f, 0.0f);
    createNestedSquares(squaresData);

    // Setup VAOs for each window (using main window context)
    Shape ellipse = setupVAO(ellipseData, GL_TRIANGLE_FAN);
    Shape triangle = setupVAO(triData, GL_TRIANGLES);
    Shape circle = setupVAO(circleData, GL_TRIANGLE_FAN);
    Shape squares = setupVAO(squaresData, GL_TRIANGLE_STRIP);

    // Initialize menu
    initMenu();

    std::cout << "=== CONTROLS ===" << std::endl;
    std::cout << "Main Window:" << std::endl;
    std::cout << "  - Right-click: Open color menu" << std::endl;
    std::cout << "  - RED block: Stop animation" << std::endl;
    std::cout << "  - GREEN block: Start animation" << std::endl;
    std::cout << "  - BLUE block: Change square colors" << std::endl;
    std::cout << "Window 2:" << std::endl;
    std::cout << "  - R,G,B,Y,O,P,W: Change circle/triangle colors" << std::endl;
    std::cout << "=================" << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(mainWindow)) {
        updateAnimations();

        // Update shapes with new transformations/colors
        std::vector<float> newTriData, newCircleData, newSquaresData;
        
        // Update triangle on the left with rotation
        createTriangle(newTriData, triangleRotation, -0.4f, 0.0f);
        // Update circle on the right with breathing animation
        createCircle(newCircleData, circleScale, 50, 0.4f, 0.0f);
        createNestedSquares(newSquaresData, squareRotation);
        
        // Update VAOs (using main window context)
        glfwMakeContextCurrent(mainWindow);
        updateVAO(triangle, newTriData, GL_TRIANGLES);
        updateVAO(circle, newCircleData, GL_TRIANGLE_FAN);
        updateVAO(squares, newSquaresData, GL_TRIANGLE_STRIP);

        // Render main window (black & white squares only)
        glfwMakeContextCurrent(mainWindow);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        
        glBindVertexArray(squares.vao);
        for (int i = 0; i < 6; i++)
            glDrawArrays(squares.mode, i * 4, 4);
        
        // Draw menu if visible
        if (showMenu) {
            drawMenu();
        }
        
        glfwSwapBuffers(mainWindow);

        // Render subwindow (ellipse with custom background)
        glfwMakeContextCurrent(subWindow);
        glClearColor(subWindowBgColor[0], subWindowBgColor[1], subWindowBgColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        
        glBindVertexArray(ellipse.vao);
        glDrawArrays(ellipse.mode, 0, ellipse.vertexCount);
        glfwSwapBuffers(subWindow);

        // Render window2 (circle and triangle separated)
        glfwMakeContextCurrent(window2);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark background
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        
        // Draw triangle on the left
        glBindVertexArray(triangle.vao);
        glDrawArrays(triangle.mode, 0, triangle.vertexCount);
        
        // Draw circle on the right
        glBindVertexArray(circle.vao);
        glDrawArrays(circle.mode, 0, circle.vertexCount);
        glfwSwapBuffers(window2);

        // Poll events
        glfwPollEvents();
    }

    // Cleanup
    glDeleteProgram(program);

    glfwDestroyWindow(mainWindow);
    glfwDestroyWindow(subWindow);
    glfwDestroyWindow(window2);
    glfwTerminate();
    return 0;
}