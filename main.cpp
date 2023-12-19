#include <cstdio>
#include <string>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <fstream>
#include <cassert>
#include <cmath>
#include "my_math.h"
#include "objects.h"

#define Window_Width 1024
#define Window_Height 768

using namespace std;

GLuint VAO, VBO, IBO;
vector<float> vertices;
Vector3f Camera;
const Vector3f zero = {0, 0, 0};
const Vector3f one = {1, 1, 1};
Vector3f image[Window_Width * Window_Height];

const int piece = 10;
struct Light {            // 定义光源
    // Vector3f direction; // 方向
    Vector3f lightIntensity;            // 光照强度
    Vector3f position; // 位置(中心)
    Vector3f dLightIntensity;
    float r; // 半长(正方形)
    Light(Vector3f _lightIntensity, Vector3f _position, float _r) {
        CopyVector3(lightIntensity, _lightIntensity);
        CopyVector3(position, _position);
        r = _r;
        DivVector3andFloat(dLightIntensity, lightIntensity, piece*piece);
    }
};

vector<Light *> lights;
Vector3f AmbientLight;
vector<MyObject *> orbs;

const char *pVSFileName = "shader.vs";
const char *pFSFileName = "shader.fs";


bool ReadFile(const char *pFileName, string &outFile) {
    ifstream f(pFileName);

    bool ret = false;

    if (f.is_open()) {
        string line;
        while (getline(f, line)) {
            outFile.append(line);
            outFile.append("\n");
        }
        f.close();
        ret = true;
    } else {
        fprintf(stderr, "%s:%d: unable to open file `%s`\n", __FILE__, __LINE__, pFileName);
    }
    return ret;
}


void Render() {

    glClear(GL_COLOR_BUFFER_BIT);

    // 位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    // 颜色
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_POINTS, 0, vertices.size() / 6);
    glDisableVertexAttribArray(0);
    glutSwapBuffers();
}

void calLightIntensity(Vector3f position, Light light, Vector3f res) //计算该光源的每一个光照元能否照射到他
{
    for (int i = 0; i < piece; i++) {
        for (int j = 0; j < piece; j++) {
            Vector3f start, dir, temp = {-light.r / 2 + light.r / piece * i, 0, -light.r / 2 + light.r / piece * j};
            AddVector3(start, light.position, temp);
            SubVector3(dir, position, start);
            Ray shadowRay(start, dir);
            bool flag = true;
            for (MyObject *orb: orbs) {
                Hit hit = orb->intersect(shadowRay);
                if (hit.t > 0 && (hit.position[0] - position[0]) * (hit.position[0] - start[0]) < 0) {
                    flag = false;
                }
            }
            if (flag) {
                AddVector3(res, res, light.dLightIntensity);
            }
        }
    }

}

static void trace(Ray ray, int depth, Vector3f ret) {
    if (depth > 5) { // 到达最大递归层数
        CopyVector3(ret, AmbientLight);
        return;
    }
    Hit nearHit = *new Hit();
    nearHit.t = INFINITY;
    MyObject *nearOrb = nullptr;
    for (auto i: orbs) { // 确定最近的交点
        Hit hit = i->intersect(ray);
        if (hit.t > 0 && hit.t < nearHit.t) {
            nearHit = hit;
            nearOrb = i;
        }
    }
    for (Light* l:lights) { // 与光源相交，返回光源亮度
        float x = (l->position[1]-ray.start[1])/ray.dir[1]*ray.dir[0]+ray.start[0];
        float z = (l->position[1]-ray.start[1])/ray.dir[1]*ray.dir[2]+ray.start[2];
        if(fabs(x-l->position[0])<l->r && fabs(z-l->position[2])<l->r) {
            float dis = sqrt((x-ray.start[0])*(x-ray.start[0])
                    +(l->position[1]-ray.start[1])*(l->position[1]-ray.start[1])
                    +(z-ray.start[2])*(z-ray.start[2]));
            if(dis < nearHit.t) {
                CopyVector3(ret, l->lightIntensity);
                return;
            }
        }
    }
    if (nearOrb != nullptr) { // 与物体相交
        if (nearHit.material->type == ROUGH) {
            Vector3f outRadiance;
            MultiplyVector3ByElement(outRadiance, nearHit.material->ka, AmbientLight); // 初始化返回光线（利用环境光）
            for (Light *light: lights) { // fixed 改成有限面光源
                Vector3f temp;
                MultiplyVector3andFloat(temp, nearHit.normal, epsilon);
                AddVector3(temp, nearHit.position, temp);
                Vector3f direction;
                SubVector3(direction, light->position, nearHit.position); // direction = position->light
                Vector3f nowLightIntensity = {0, 0, 0};
//                if(fabs(nearHit.position[1]-1)<epsilon) {
//                    printf("%f,%f,%f\n",nearHit.position[0], nearHit.position[1], nearHit.position[2]);
//                }
//                if (fabs(nearHit.position[0] + 0.72) < 100*epsilon &&
//                        fabs(nearHit.position[1] + 1) < 100*epsilon &&
//                        fabs(nearHit.position[2] + -0.86) < 100*epsilon) { // 被蓝色球遮挡
//                    printf("%f,%f,%f\n",nearHit.position[0], nearHit.position[1], nearHit.position[2]);
//                }
                calLightIntensity(nearHit.position, *light, nowLightIntensity);
//                if(fabs(nearHit.position[1]-1)<epsilon) {
//                    printf("%f,%f,%f\n",nowLightIntensity[0], nowLightIntensity[1], nowLightIntensity[2]);
//                }
                NormalizeVector3(direction);
//                printf("%f,%f,%f\n",nowLightIntensity[0], nowLightIntensity[1], nowLightIntensity[2]);
//                printf("%f,%f,%f\n",nearHit.normal[0], nearHit.normal[1], nearHit.normal[2]);
//                printf("%f,%f,%f\n",direction[0], direction[1], direction[2]);
                float cosTheta = DotVector3(nearHit.normal, direction);
//                printf("%f\n",cosTheta);
//                if (fabs(nearHit.position[0] + 0.72) < epsilon &&
//                    (nearHit.position[1] + 1) < epsilon &&
//                    (nearHit.position[2] + -0.86) < epsilon) { // 被蓝色球遮挡
//                    printf("%f\n", cosTheta);
//                }
                if (cosTheta > 0)    // 如果cos小于0（钝角），说明光照到的是物体背面，相机看不到
                {
                    if (!(nowLightIntensity[0] == 0 && nowLightIntensity[1] == 0 &&
                          nowLightIntensity[2] == 0))    // 有亮度
                    {
                        MultiplyVector3ByElement(temp, nowLightIntensity, nearHit.material->kd);
                        MultiplyVector3andFloat(temp, temp, cosTheta);
                        AddVector3(outRadiance, outRadiance, temp); // 漫反射成分
//                        printf("%f,%f,%f\n",outRadiance[0], outRadiance[1], outRadiance[2]);
                        //outRadiance = outRadiance + light->lightIntensity * nearHit.material->kd * cosTheta;
                        SubVector3(temp, zero, ray.dir);
                        Vector3f halfway;
                        AddVector3(halfway, temp, direction);
                        NormalizeVector3(halfway);
                        //Vector3f halfway = normalize(-ray.dir + light->direction);
                        float cosDelta = DotVector3(nearHit.normal, halfway);
//                        printf("%f\n", cosDelta);
//                        if(fabs(nearHit.position[1]-1)<epsilon) {
//                            printf("%f\n", cosDelta);
//                        }
                        if (cosDelta > 0) { // 镜面反射成分
                            MultiplyVector3ByElement(temp, light->dLightIntensity, nearHit.material->ks);
                            MultiplyVector3andFloat(temp, temp, powf(cosDelta, nearHit.material->shininess));
                            AddVector3(outRadiance, outRadiance, temp);
//                             outRadiance = outRadiance + light->lightIntensity * nearHit.material->ks * powf(cosDelta, nearHit.material->shininess);
                        }
                    }
                }
//                if (fabs(nearHit.position[2]+1) < epsilon) { // 后墙
//                    printf("%f,%f,%f\n",outRadiance[0], outRadiance[1], outRadiance[2]);
//                }
//                if(fabs(nearHit.position[1]-1)<epsilon) { // 打到顶端
//                    printf("%f,%f,%f\n",outRadiance[0], outRadiance[1], outRadiance[2]);
//                }
//                printf("%f,%f,%f\n",outRadiance[0], outRadiance[1], outRadiance[2]);
                CopyVector3(ret, outRadiance);
            }
//            printf("return\n");
            return;
        } else {
            Vector3f temp;
            float cosa = -DotVector3(ray.dir, nearHit.normal);        // 镜面反射（继续追踪）
            Vector3f one = {1, 1, 1};
            Vector3f F;
            SubVector3(temp, one, nearHit.material->F0);
            MultiplyVector3andFloat(temp, temp, pow(1 - cosa, 5));
            AddVector3(F, nearHit.material->F0, temp);
            //F = nearHit.material->F0 + (one - nearHit.material->F0) * pow(1 - cosa, 5);
            Vector3f reflectedDir;
            MultiplyVector3andFloat(temp, nearHit.normal, DotVector3(nearHit.normal, ray.dir) * 2.0f);
            SubVector3(reflectedDir, ray.dir, temp);
            //Vector3f reflectedDir = ray.dir - nearHit.normal * DotVector3(nearHit.normal, ray.dir) * 2.0f;		// 反射光线R = v + 2Ncosa
            MultiplyVector3andFloat(temp, nearHit.normal, epsilon);
            AddVector3(temp, nearHit.position, temp);
            trace(Ray(temp, reflectedDir), depth + 1, ret);
            Vector3f outRadiance;
            MultiplyVector3ByElement(outRadiance, ret, F);

            if (nearHit.material->type == REFRACTIVE)     // 对于透明物体，计算折射（继续追踪）
            {
                float disc = 1 - (1 - cosa * cosa) / nearHit.material->ior / nearHit.material->ior;
                if (disc >= 0) {
                    Vector3f refractedDir;
                    Vector3f temp2;
                    MultiplyVector3andFloat(temp, ray.dir, 1.0 / nearHit.material->ior);
                    MultiplyVector3andFloat(temp2, nearHit.normal, cosa / nearHit.material->ior - sqrt(disc));
                    AddVector3(refractedDir, temp, temp2);
                    //refractedDir = ray.dir / nearHit.material->ior + nearHit.normal * (cosa / nearHit.material->ior - sqrt(disc));
                    MultiplyVector3andFloat(temp, nearHit.normal, epsilon);
                    SubVector3(temp, nearHit.position, temp);
                    trace(Ray(temp, refractedDir), depth + 1, ret);
                    SubVector3(temp, one, F);
                    MultiplyVector3ByElement(temp, ret, temp);
                    AddVector3(outRadiance, outRadiance, temp);
                    //outRadiance = outRadiance + trace(Ray(nearHit.position - nearHit.normal * epsilon, refractedDir), depth + 1, ret) * (one - F);
                }
            }
            CopyVector3(ret, outRadiance);
            return;
        }
    } else {
        CopyVector3(ret, AmbientLight);
    }
}

static void initScene() {
    // 相机位置
    Vector3f temp, t1, t2;
    temp[0] = 0, temp[1] = 0, temp[2] = 4 - epsilon;
    CopyVector3(Camera, temp);
    // 环境光
    temp[0] = 0.4, temp[1] = 0.4, temp[2] = 0.4;
    CopyVector3(AmbientLight, temp);
    // 光源
    temp[0] = 0.3, temp[1] = 1 - 0.05, temp[2] = -0.3; // 位置
    t1[0] = 1.5, t1[1] = 1.5, t1[2] = 1.5; // 光照强度
    lights.push_back(new Light(t1, temp, 0.2));
    temp[0] = -0.2, temp[1] = 1 - 0.05, temp[2] = 0.4; // 位置
    t1[0] = 2, t1[1] = 2, t1[2] = 2; // 光照强度
    lights.push_back(new Light(t1, temp, 0.3));
    // 物体
    t2[0] = 0.2, t2[1] = 0.2, t2[2] = 0.2;
    temp[0] = 0.3, temp[1] = 0.2, temp[2] = 0.1;
    Material *yellowRough = new RoughMaterial(temp, t2, 10);
    temp[0] = 0.1, temp[1] = 0.2, temp[2] = 0.3;
    Material *blueRough = new RoughMaterial(temp, t2, 10);
    temp[0] = 3, temp[1] = 0, temp[2] = 0.2;
    Material *pinkRough = new RoughMaterial(temp, t2, 10);
    temp[0] = 0.03, temp[1] = 0.03, temp[2] = 0.03;
    Material *blackRough = new RoughMaterial(temp, t2, 10);
    temp[0] = 0.8, temp[1] = 0.8, temp[2] = 0.8;
    Material *whiteRough = new RoughMaterial(temp, t2, 10);
    temp[0] = 0.3, temp[1] = 0, temp[2] = 0;
    Material *redRough = new RoughMaterial(temp, t2, 10);
    // 构建上下左右前后面
//    t1[0] = 0, t1[1] = 0, t1[2] = 5, t2[0] = 0, t2[1] = 0, t2[2] = -1;
//    orbs.push_back(new Plane(t1, t2, blackRough));
    t1[0] = 0, t1[1] = 0, t1[2] = -1, t2[0] = 0, t2[1] = 0, t2[2] = 1;
    orbs.push_back(new Plane(t1, t2, yellowRough));
    t1[0] = 0, t1[1] = 1, t1[2] = 0, t2[0] = 0, t2[1] = -1, t2[2] = 0;
    orbs.push_back(new Plane(t1, t2, blueRough));
    t1[0] = 0, t1[1] = -1, t1[2] = 0, t2[0] = 0, t2[1] = 1, t2[2] = 0;
    orbs.push_back(new Plane(t1, t2, blueRough));
    t1[0] = 1, t1[1] = 0, t1[2] = 0, t2[0] = -1, t2[1] = 0, t2[2] = 0;
    orbs.push_back(new Plane(t1, t2, pinkRough));
    t1[0] = -1, t1[1] = 0, t1[2] = 0, t2[0] = 1, t2[1] = 0, t2[2] = 0;
    orbs.push_back(new Plane(t1, t2, pinkRough));
    t1[0] = 0.5, t1[1] = -0.7, t1[2] = 0.5;
    orbs.push_back(new Sphere(t1, 0.3, yellowRough));
    t1[0] = -0.6, t1[1] = -0.4, t1[2] = 0.6;
    orbs.push_back(new Sphere(t1, 0.3, blueRough));
    t1[0] = -0, t1[1] = -0.3, t1[2] = 0.6;
    orbs.push_back(new Sphere(t1, 0.2, redRough));
    t1[0] = -0.4, t1[1] = -0.75, t1[2] = 0.3;
    orbs.push_back(new Sphere(t1, 0.2, pinkRough));
    t1[0] = -0.65, t1[1] = 0.3, t1[2] = 0, temp[0] = 0.14, temp[1] = 0.16, temp[2] = 0.13, t2[0] = 4.1, t2[1] = 2.3, t2[2] = 3.1;
    orbs.push_back(new Sphere(t1, 0.2,
                              new ReflectiveMaterial(temp, t2)));
    t1[0] = 0, t1[1] = -0.6, t1[2] = 1;
    orbs.push_back(new Sphere(t1, 0.1,
                              new ReflectiveMaterial(temp, t2)));
}

static void CreateVertexBuffer() {
    Vector3f *pixel = image;
    float invWidth = 1 / float(Window_Width), invHeight = 1 / float(Window_Height); //计算屏占比
    float fov = 40, aspectratio = Window_Width / float(Window_Height); // 设定视场角（视野范围） 和 纵横比
    float angle = tan(M_PI * 0.5 * fov / 180.0); // 把视场角转化为普通的角度

    // 光线追踪开始，逐像素点进行光线追踪
    for (unsigned y = 0; y < Window_Height; ++y) {
        for (unsigned x = 0; x < Window_Width; ++x, ++pixel) {
            //进行坐标系的转换
            float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
            Vector3f raydir = {xx, yy, -1}; //确定出射光方向向量
            NormalizeVector3(raydir);
            Vector3f color;
            Ray ray(Camera, raydir);
            trace(ray, 0, color);
            CopyVector3(*pixel, color);
        }
    }

    for (unsigned int i = 0; i < Window_Height; i++) {
        for (unsigned int j = 0; j < Window_Width; j++) {
            //坐标转换为屏幕像素的坐标
            auto a = -2 * (float(j) / Window_Width) + 1;
            auto b = -2 * (float(i) / Window_Height) + 1;
            vertices.push_back(a);
            vertices.push_back(b);
            vertices.push_back(0);
            //设置坐标的颜色值
            vertices.push_back(min(image[i * Window_Width + j][0], float(1)));
            vertices.push_back(min(image[i * Window_Width + j][1], float(1)));
            vertices.push_back(min(image[i * Window_Width + j][2], float(1)));
        }
    }
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * 4, &vertices[0], GL_STATIC_DRAW);
}


static void AddShader(GLuint ShaderProgram, const char *pShaderText, GLenum ShaderType) {
    GLuint ShaderObj = glCreateShader(ShaderType);
    //check if it is successful
    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }

    //define shader code source
    const GLchar *p[1];
    p[0] = pShaderText;
    GLint Lengths[1];
    Lengths[0] = strlen(pShaderText);
    glShaderSource(ShaderObj, 1, p, Lengths);
    //Compiler shader object
    glCompileShader(ShaderObj);

    //check the error about shader
    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
    //bound the shader object to shader program
    glAttachShader(ShaderProgram, ShaderObj);
}

static void CompilerShaders() {
    //Create Shaders
    GLuint ShaderProgram = glCreateProgram();

    //Check yes or not success
    if (ShaderProgram == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

    //the buffer of shader texts
    string vs, fs;
    //read the text of shader texts to buffer
    if (!ReadFile(pVSFileName, vs)) {
        exit(1);
    }
    if (!ReadFile(pFSFileName, fs)) {
        exit(1);
    }

    //add vertex shader and fragment shader
    AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
    AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

    //Link the shader program, and check the error
    GLint Success = 0;
    GLchar ErrorLog[1024] = {0};
    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    //check if it can be executed
    glValidateProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    //use program
    glUseProgram(ShaderProgram);
//    gWorldLocation = glGetUniformLocation(ShaderProgram, "gWVP");
//    assert(gWorldLocation != 0xFFFFFFFF);

}

static void InitializeGlutCallbacks() {
    glutDisplayFunc(Render);
    glutIdleFunc(Render); // 全局闲置回调
//    glutPassiveMotionFunc(Mouse);
//    glutSpecialFunc(Keyboard);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1024, 768);
    glutInitWindowPosition(10, 10);
    glutInitWindowSize(Window_Width, Window_Height);
    glutCreateWindow("index");

    InitializeGlutCallbacks();

    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return 1;
    }

    printf("GL version: %s\n", glGetString(GL_VERSION));

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    CompilerShaders();

    initScene();

    CreateVertexBuffer();

    glutMainLoop();

    return 0;
}