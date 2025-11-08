#include <GL/glew.h>
#include <GL/glut.h>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// Window size
int winW = 900, winH = 700;

// Camera (spherical)
float camAz = 30.0f;   
float camEl = 10.0f;   
float camDist = 8.0f;
float camCenterX = 0.0f, camCenterY = 0.0f, camCenterZ = 0.0f;

bool useAA = true; 

// Object diffuse colors (rgb)
float objColor[3][3] = {
    {0.8f, 0.2f, 0.2f}, // obj 0
    {0.2f, 0.8f, 0.2f}, // obj 1
    {0.2f, 0.2f, 0.8f}  // obj 2
};

// Unique pick colors
unsigned char pickColorBytes[3][3] = {
    { 10,  20,  30}, // id 0
    { 40,  50,  60}, // id 1
    { 70,  80,  90}  // id 2
};


GLuint pickFBO = 0;
GLuint pickColorTex = 0;
GLuint pickDepthRB = 0;

// some helper
void randizeObjectColor(int id) {
    objColor[id][0] = 0.2f + 0.8f * (rand() / (float)RAND_MAX);
    objColor[id][1] = 0.2f + 0.8f * (rand() / (float)RAND_MAX);
    objColor[id][2] = 0.2f + 0.8f * (rand() / (float)RAND_MAX);
}

// Set camera and light 
void setupCameraAndLight() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)winW / (double)winH, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // compute camera position
    float az = camAz * (float)M_PI / 180.0f;
    float el = camEl * (float)M_PI / 180.0f;
    float cx = camCenterX, cy = camCenterY, cz = camCenterZ;
    float camX = cx + camDist * cosf(el) * cosf(az);
    float camY = cy + camDist * sinf(el);
    float camZ = cz + camDist * cosf(el) * sinf(az);
    gluLookAt(camX, camY, camZ, cx, cy, cz, 0.0f, 1.0f, 0.0f);

    
    GLfloat lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; 
    GLfloat lightDiffuse[4] = { 1.0f,1.0f,1.0f,1.0f };
    GLfloat lightSpecular[4] = { 0.6f,0.6f,0.6f,1.0f };
    GLfloat lightAmbient[4] = { 0.25f, 0.25f, 0.25f, 1.0f };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
}

// Draw the scene
void drawScene(bool pickMode = false) {
    
    if (pickMode) {
        glDisable(GL_LIGHTING);
        glShadeModel(GL_FLAT);
        glDisable(GL_DITHER);
        glDisable(GL_POLYGON_SMOOTH);
        glDisable(GL_BLEND);
    }
    else {
        glEnable(GL_LIGHTING);
        glShadeModel(GL_SMOOTH);
        glEnable(GL_DITHER);
    }

    // Common material settings when not in pick mode
    for (int id = 0; id < 3; ++id) {
        glPushMatrix();

        if (id == 0) glTranslatef(-2.2f, 0.0f, 0.0f);
        if (id == 1) glTranslatef(0.0f, 0.0f, 0.0f);
        if (id == 2) glTranslatef(2.2f, 0.0f, 0.0f);

        
        glRotatef(-20.0f, 1.0f, 0.0f, 0.0f);
        glRotatef((float)(id * 30), 0.0f, 1.0f, 0.0f);

        if (pickMode) {
            
            glColor3ub(pickColorBytes[id][0], pickColorBytes[id][1], pickColorBytes[id][2]);
            if (id == 0) glutSolidSphere(0.9, 48, 48);
            else if (id == 1) glutSolidTorus(0.25, 0.85, 48, 48);
            else glutSolidTeapot(0.8);
        }
        else {
            
            GLfloat diffuse[4] = { objColor[id][0], objColor[id][1], objColor[id][2], 1.0f };
            GLfloat spec[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
            GLfloat ambient[4] = { 0.08f, 0.08f, 0.08f, 1.0f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);

            // draw primitive
            if (id == 0) glutSolidSphere(0.9, 48, 48);
            else if (id == 1) glutSolidTorus(0.25, 0.85, 48, 48);
            else glutSolidTeapot(0.8);
        }

        glPopMatrix();
    }
}


bool buildPickingFBO(int w, int h) {
    // Delete old resources if present
    if (pickFBO) {
        glDeleteFramebuffers(1, &pickFBO);
        pickFBO = 0;
    }
    if (pickColorTex) {
        glDeleteTextures(1, &pickColorTex);
        pickColorTex = 0;
    }
    if (pickDepthRB) {
        glDeleteRenderbuffers(1, &pickDepthRB);
        pickDepthRB = 0;
    }

    
    glGenFramebuffers(1, &pickFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pickFBO);

    // color texture (RGB8)
    glGenTextures(1, &pickColorTex);
    glBindTexture(GL_TEXTURE_2D, pickColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // attach color texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickColorTex, 0);

    // depth renderbuffer
    glGenRenderbuffers(1, &pickDepthRB);
    glBindRenderbuffer(GL_RENDERBUFFER, pickDepthRB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pickDepthRB);

    // set draw buffers
    GLenum drawBufs[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBufs);
    // read buffer also
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Check completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "ERROR: Picking FBO incomplete, status = 0x" << std::hex << status << std::dec << endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

   
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void pickAt(int mx, int my) {
    // Convert screen Y coordinate to OpenGL coordinates
    int readY = winH - 1 - my;
    unsigned char pixel[3] = { 0,0,0 };

    glBindFramebuffer(GL_FRAMEBUFFER, pickFBO);
    glViewport(0, 0, winW, winH);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    setupCameraAndLight();

    drawScene(true);

    glFlush();
    glFinish();

    // Ensure we read from the color attachment
    glReadBuffer(GL_COLOR_ATTACHMENT0);

   
    glReadPixels(mx, readY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    std::cout << "Picked color = ("
        << (int)pixel[0] << ", "
        << (int)pixel[1] << ", "
        << (int)pixel[2] << ")\n";

    // Identify which object was clicked
    int picked = -1;
    if (pixel[0] == pickColorBytes[0][0] && pixel[1] == pickColorBytes[0][1] && pixel[2] == pickColorBytes[0][2]) picked = 0;
    else if (pixel[0] == pickColorBytes[1][0] && pixel[1] == pickColorBytes[1][1] && pixel[2] == pickColorBytes[1][2]) picked = 1;
    else if (pixel[0] == pickColorBytes[2][0] && pixel[1] == pickColorBytes[2][1] && pixel[2] == pickColorBytes[2][2]) picked = 2;

    if (picked >= 0) {
        // Randomize color of picked object 
        objColor[picked][0] = (float)rand() / RAND_MAX;
        objColor[picked][1] = (float)rand() / RAND_MAX;
        objColor[picked][2] = (float)rand() / RAND_MAX;

        std::cout << "Picked object " << picked
            << " new color = ("
            << objColor[picked][0] << ", "
            << objColor[picked][1] << ", "
            << objColor[picked][2] << ")\n";

       
        glutPostRedisplay();
    }
}

void display() {
   
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, winW, winH);

    if (useAA) glEnable(GL_MULTISAMPLE);
    else glDisable(GL_MULTISAMPLE);

    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCameraAndLight();

    // draw axes at center for reference
    glPushMatrix();
    glTranslatef(camCenterX, camCenterY, camCenterZ);
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(0.8f, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 0.8f, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 0.8f);
    glEnd();
    glPopMatrix();

    
    drawScene(false);

    // HUD
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, winW, 0, winH, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glColor3f(1, 1, 1);
    string hud = "AA: (a) toggle     Click to pick object     Camera: arrow keys (rotate), w/s zoom, r reset";
    glRasterPos2i(8, winH - 18);
    for (char c : hud) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    winW = max(1, w);
    winH = max(1, h);
    
    if (!buildPickingFBO(winW, winH)) {
        cerr << "Failed to (re)build picking FBO\n";
    }
    glViewport(0, 0, winW, winH);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 27: case 'q': exit(0); break;
    case 'r':
        camAz = 30.0f; camEl = 10.0f; camDist = 8.0f;
        camCenterX = camCenterY = camCenterZ = 0.0f;
        break;
    case 'a':
        useAA = !useAA;
        cout << "Anti-aliasing " << (useAA ? "ON" : "OFF") << "\n";
        break;
    case 'w':
        camDist = fmaxf(1.0f, camDist - 0.4f); break;
    case 's':
        camDist = fminf(50.0f, camDist + 0.4f); break;
    case 'p': 
        for (int i = 0;i < 3;i++) cout << "obj " << i << " color = " << objColor[i][0] << ", " << objColor[i][1] << ", " << objColor[i][2] << "\n";
        break;
    }
    glutPostRedisplay();
}

void specialKey(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_LEFT: camAz -= 4.0f; break;
    case GLUT_KEY_RIGHT: camAz += 4.0f; break;
    case GLUT_KEY_UP: camEl = min(89.0f, camEl + 4.0f); break;
    case GLUT_KEY_DOWN: camEl = max(-89.0f, camEl - 4.0f); break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        pickAt(x, y);
    }
}

// init GL states
void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
    glDisable(GL_COLOR_MATERIAL);

    
    if (!buildPickingFBO(winW, winH)) {
        cerr << "Initial FBO build failed\n";
    }
    srand((unsigned int)time(NULL));
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
   
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Part 2 ");

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        cerr << "Error: GLEW init failed: " << glewGetErrorString(err) << endl;
        return 1;
    }

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutMouseFunc(mouse);

    cout << "Controls:\n  Arrow keys: rotate camera\n  w/s: zoom  r: reset\n  a: toggle anti-aliasing\n  Click left mouse on objects to pick and randomize their color.\n";

    glutMainLoop();
    return 0;
}
