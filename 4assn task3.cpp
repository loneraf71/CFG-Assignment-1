// bezier_patch_modern.cpp
// Compile with: g++ bezier_patch_modern.cpp -lGLEW -lGL -lGLU -lglut -std=c++17 -O2

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
};
static Vec3 crossp(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static float dotp(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 normalize(const Vec3& v) {
    float L = sqrtf(dotp(v, v));
    return (L > 1e-6f) ? Vec3(v.x / L, v.y / L, v.z / L) : v;
}

// Control points
Vec3 ctrl[4][4];

bool loadControlPointsFromFile(const char* fname) {
    std::ifstream in(fname);
    if (!in.is_open()) return false;
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++) {
            float x, y, z;
            if (!(in >> x >> y >> z)) return false;
            ctrl[i][j] = Vec3(x, y, z);
        }
    return true;
}

void setDefaultControlPoints() {
    float d[16][3] = {
        {-1.5f,-1.5f,0},{-0.5f,-1.5f,0},{0.5f,-1.5f,0},{1.5f,-1.5f,0},
        {-1.5f,-0.5f,0},{-0.5f,-0.5f,1.2f},{0.5f,-0.5f,1.2f},{1.5f,-0.5f,0},
        {-1.5f,0.5f,0},{-0.5f,0.5f,1.2f},{0.5f,0.5f,1.2f},{1.5f,0.5f,0},
        {-1.5f,1.5f,0},{-0.5f,1.5f,0},{0.5f,1.5f,0},{1.5f,1.5f,0}
    };
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            ctrl[i][j] = Vec3(d[j * 4 + i][0], d[j * 4 + i][1], d[j * 4 + i][2]);
}

void bernstein3(float u, float B[4]) {
    float om = 1 - u;
    B[0] = om * om * om;
    B[1] = 3 * u * om * om;
    B[2] = 3 * u * u * om;
    B[3] = u * u * u;
}
void bernstein3_deriv(float u, float dB[4]) {
    float om = 1 - u;
    dB[0] = -3 * om * om;
    dB[1] = 3 * om * om - 6 * u * om;
    dB[2] = 6 * u * om - 3 * u * u;
    dB[3] = 3 * u * u;
}

Vec3 evalP(float u, float v) {
    float Bu[4], Bv[4];
    bernstein3(u, Bu);
    bernstein3(v, Bv);
    Vec3 P;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            P = P + ctrl[i][j] * (Bu[i] * Bv[j]);
    return P;
}
Vec3 evalPu(float u, float v) {
    float dBu[4], Bv[4];
    bernstein3_deriv(u, dBu);
    bernstein3(v, Bv);
    Vec3 P;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            P = P + ctrl[i][j] * (dBu[i] * Bv[j]);
    return P;
}
Vec3 evalPv(float u, float v) {
    float Bu[4], dBv[4];
    bernstein3(u, Bu);
    bernstein3_deriv(v, dBv);
    Vec3 P;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            P = P + ctrl[i][j] * (Bu[i] * dBv[j]);
    return P;
}

struct Vertex { float px, py, pz, nx, ny, nz, u, v; };
std::vector<Vertex> verts;
std::vector<unsigned int> inds;
int RES = 32;

void buildMesh() {
    verts.clear(); inds.clear();
    for (int j = 0; j < RES; j++) {
        float v = j / float(RES - 1);
        for (int i = 0; i < RES; i++) {
            float u = i / float(RES - 1);
            Vec3 P = evalP(u, v);
            Vec3 Pu = evalPu(u, v);
            Vec3 Pv = evalPv(u, v);
            Vec3 N = normalize(crossp(Pu, Pv));
            Vertex vt = { P.x, P.y, P.z, N.x, N.y, N.z, u, v };
            verts.push_back(vt);
        }
    }
    for (int j = 0; j < RES - 1; j++) {
        for (int i = 0; i < RES - 1; i++) {
            unsigned int i00 = j * RES + i;
            unsigned int i10 = i00 + 1;
            unsigned int i01 = i00 + RES;
            unsigned int i11 = i01 + 1;
            inds.insert(inds.end(), { i00, i10, i11, i00, i11, i01 });
        }
    }
}

// GL objects
GLuint vao = 0, vbo = 0, ebo = 0, tex = 0, prog = 0;
bool useTex = true;

// Camera (single set of vars)
float camYawDeg = 45.0f, camPitchDeg = 20.0f, camDistVal = 6.0f;

// Shader helpers
GLuint compile(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(sh, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
        std::cerr << "Shader source:\n" << src << std::endl;
        exit(1);
    }
    return sh;
}
GLuint link(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
        exit(1);
    }
    return p;
}

const char* vsSrc = R"(
#version 330 core
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inUV;
uniform mat4 uModel, uView, uProj;
uniform mat3 uNormalMat;
out vec3 vPosView;
out vec3 vNormalView;
out vec2 vUV;
void main(){
    vec4 w = uModel * vec4(inPos, 1.0);
    vec4 pv = uView * w;
    vPosView = pv.xyz;
    vNormalView = normalize(uNormalMat * inNormal);
    vUV = inUV;
    gl_Position = uProj * pv;
})";

const char* fsSrc = R"(
#version 330 core
in vec3 vPosView, vNormalView;
in vec2 vUV;
out vec4 frag;
uniform vec3 uLightPosView, uLightColor, uSpecular, uAmbient;
uniform float uShininess;
uniform sampler2D uTex;
uniform bool uUseTexture;
void main(){
    vec3 N = normalize(vNormalView);
    vec3 L = normalize(uLightPosView - vPosView);
    vec3 V = normalize(-vPosView);
    float NdotL = max(dot(N, L), 0.0);
    vec3 texCol = uUseTexture ? texture(uTex, vUV).rgb : vec3(1.0, 1.0, 1.0);
    vec3 amb = uAmbient * texCol;
    vec3 diff = texCol * uLightColor * NdotL;
    vec3 R = normalize(2.0 * NdotL * N - L);
    float s = pow(max(dot(R, V), 0.0), uShininess);
    vec3 spec = uSpecular * s;
    frag = vec4(amb + diff + spec, 1.0);
})";

// Simple mat4 (column-major), mat3 utilities
struct Mat4 { float m[16]; Mat4(){ std::fill(m, m+16, 0.0f); } };
Mat4 mat_identity() { Mat4 I; I.m[0]=I.m[5]=I.m[10]=I.m[15]=1.0f; return I; }

Mat4 mat_mul(const Mat4& A, const Mat4& B) {
    Mat4 R;
    for (int r=0;r<4;r++){
        for (int c=0;c<4;c++){
            float s=0;
            for (int k=0;k<4;k++) s += A.m[k*4 + r] * B.m[c*4 + k]; // column-major index
            R.m[c*4 + r] = s;
        }
    }
    return R;
}

Mat4 perspective(float fovyDeg, float aspect, float znear, float zfar) {
    float f = 1.0f / tanf(fovyDeg * (float)M_PI / 360.0f);
    Mat4 M;
    std::fill(M.m, M.m+16, 0.0f);
    M.m[0] = f / aspect;
    M.m[5] = f;
    M.m[10] = (zfar + znear) / (znear - zfar);
    M.m[11] = -1.0f;
    M.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return M;
}

Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = normalize(center - eye);
    Vec3 s = normalize(crossp(f, up));
    Vec3 u = crossp(s, f);

    Mat4 M = mat_identity();
    // column-major: m[col*4 + row]
    M.m[0] = s.x; M.m[4] = s.y; M.m[8]  = s.z;
    M.m[1] = u.x; M.m[5] = u.y; M.m[9]  = u.z;
    M.m[2] = -f.x;M.m[6] = -f.y;M.m[10] = -f.z;
    M.m[12] = -dotp(s, eye);
    M.m[13] = -dotp(u, eye);
    M.m[14] =  dotp(f, eye);
    return M;
}

// extract 3x3 (column-major) upper-left from mat4 into float[9]
void mat4_to_mat3(const Mat4& M, float out3[9]) {
    out3[0] = M.m[0]; out3[1] = M.m[1]; out3[2] = M.m[2];
    out3[3] = M.m[4]; out3[4] = M.m[5]; out3[5] = M.m[6];
    out3[6] = M.m[8]; out3[7] = M.m[9]; out3[8] = M.m[10];
}

// compute inverse of 3x3 matrix (column-major) into out (also column-major)
// returns false if singular
bool invert_mat3(const float m[9], float out[9]) {
    // treat as column-major: index (c*3 + r), but determinant formula same for values
    // convert to row-major for easier computation:
    float a00 = m[0], a10 = m[1], a20 = m[2];
    float a01 = m[3], a11 = m[4], a21 = m[5];
    float a02 = m[6], a12 = m[7], a22 = m[8];

    float det = a00*(a11*a22 - a21*a12) - a01*(a10*a22 - a20*a12) + a02*(a10*a21 - a20*a11);
    if (fabs(det) < 1e-12f) return false;
    float invDet = 1.0f / det;

    // compute adjugate (row-major) then multiply by invDet
    float r00 =  (a11*a22 - a21*a12) * invDet;
    float r01 = -(a01*a22 - a21*a02) * invDet;
    float r02 =  (a01*a12 - a11*a02) * invDet;

    float r10 = -(a10*a22 - a20*a12) * invDet;
    float r11 =  (a00*a22 - a20*a02) * invDet;
    float r12 = -(a00*a12 - a10*a02) * invDet;

    float r20 =  (a10*a21 - a20*a11) * invDet;
    float r21 = -(a00*a21 - a20*a01) * invDet;
    float r22 =  (a00*a11 - a10*a01) * invDet;

    // write back as column-major
    out[0] = r00; out[1] = r01; out[2] = r02;
    out[3] = r10; out[4] = r11; out[5] = r12;
    out[6] = r20; out[7] = r21; out[8] = r22;
    return true;
}

// transpose 3x3 (column-major)
void transpose3(const float in[9], float out[9]) {
    // in[c*3 + r]
    out[0] = in[0]; out[1] = in[3]; out[2] = in[6];
    out[3] = in[1]; out[4] = in[4]; out[5] = in[7];
    out[6] = in[2]; out[7] = in[5]; out[8] = in[8];
}

void makeTex(int N = 256) {
    std::vector<unsigned char> img(N * N * 3);
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            float u = i / float(N - 1);
            float v = j / float(N - 1);
            unsigned char R = (unsigned char)(255 * u);
            unsigned char G = (unsigned char)(255 * v);
            unsigned char B = (unsigned char)(255 * (1 - u));
            int idx = (j * N + i) * 3;
            img[idx] = R; img[idx + 1] = G; img[idx + 2] = B;
        }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, N, N, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void upload() {
    if (vao == 0) glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    if (vbo == 0) glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    if (ebo == 0) glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

void display() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Camera math (compute matrices manually)
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    float pitchR = camPitchDeg * M_PI / 180.0f;
    float yawR   = camYawDeg * M_PI / 180.0f;
    Vec3 eye( camDistVal * cosf(pitchR) * cosf(yawR),
             camDistVal * sinf(pitchR),
             camDistVal * cosf(pitchR) * sinf(yawR) );
    Vec3 center(0,0,0), up(0,1,0);

    Mat4 proj = perspective(45.0f, (float)w/(float)h, 0.1f, 100.0f);
    Mat4 view = lookAt(eye, center, up);
    Mat4 model = mat_identity();
    Mat4 viewModel = mat_mul(view, model);

    // compute normal matrix (inverse-transpose of upper-left 3x3 of viewModel)
    float vm3[9]; mat4_to_mat3(viewModel, vm3);
    float inv3[9];
    bool ok = invert_mat3(vm3, inv3);
    float normalMat[9];
    if (ok) transpose3(inv3, normalMat); // inverse-transpose -> transpose(inverse(M))
    else {
        // fallback: use upper-left 3x3 of viewModel (not ideal for non-uniform scale)
        std::copy(vm3, vm3+9, normalMat);
    }

    glUseProgram(prog);
    GLint locModel = glGetUniformLocation(prog, "uModel");
    GLint locView = glGetUniformLocation(prog, "uView");
    GLint locProj = glGetUniformLocation(prog, "uProj");
    GLint locNormal = glGetUniformLocation(prog, "uNormalMat");

    glUniformMatrix4fv(locModel, 1, GL_FALSE, model.m);
    glUniformMatrix4fv(locView,  1, GL_FALSE, view.m);
    glUniformMatrix4fv(locProj,  1, GL_FALSE, proj.m);
    glUniformMatrix3fv(locNormal, 1, GL_FALSE, normalMat);

    // lighting & material
    glUniform3f(glGetUniformLocation(prog, "uLightPosView"), 0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(prog, "uLightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(prog, "uSpecular"), 0.6f, 0.6f, 0.6f);
    glUniform3f(glGetUniformLocation(prog, "uAmbient"), 0.12f, 0.12f, 0.12f);
    glUniform1f(glGetUniformLocation(prog, "uShininess"), 32.0f);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    glUniform1i(glGetUniformLocation(prog, "uUseTexture"), useTex ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)inds.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glutSwapBuffers();
}

void keys(unsigned char k, int, int) {
    if (k == 27 || k == 'q') exit(0);
    if (k == 'w') camDistVal = std::max(0.5f, camDistVal - 0.3f);
    if (k == 's') camDistVal += 0.3f;
    if (k == 't') { useTex = !useTex; std::cout << "Texture " << (useTex ? "ON" : "OFF") << "\n"; }
    if (k == '+' || k == '=') { RES = std::min(128, RES + 4); buildMesh(); upload(); }
    if (k == '-' || k == '_') { RES = std::max(4, RES - 4); buildMesh(); upload(); }
    glutPostRedisplay();
}
void special(int key, int, int) {
    if (key == GLUT_KEY_LEFT) camYawDeg -= 5;
    if (key == GLUT_KEY_RIGHT) camYawDeg += 5;
    if (key == GLUT_KEY_UP) camPitchDeg = std::min(89.0f, camPitchDeg + 5);
    if (key == GLUT_KEY_DOWN) camPitchDeg = std::max(-89.0f, camPitchDeg - 5);
    glutPostRedisplay();
}

void init() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW init error: " << glewGetErrorString(err) << std::endl;
        exit(1);
    }

    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);
    prog = link(vs, fs);

    makeTex();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
}

// main
int main(int argc, char** argv) {
    if (!loadControlPointsFromFile("patchPoints.txt"))
        setDefaultControlPoints();
    buildMesh();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Bezier Patch (modern matrices)");

    init();
    upload();
    glutDisplayFunc(display);
    glutKeyboardFunc(keys);
    glutSpecialFunc(special);

    std::cout << "Controls:\n"
        << "  Arrow keys: rotate camera\n"
        << "  W/S: zoom in/out\n"
        << "  +/- : increase/decrease tessellation\n"
        << "  T: toggle texture\n"
        << "  Q or Esc: quit\n";

    glutMainLoop();
    return 0;
}