#include <iostream>
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <random>

#include "Utility.h"
#include "Texture.h"

using namespace std;

double mod(double x, double y) {
    int sign = 1;
    if (x<0) {
        x = -x;
        sign = -1;
    }
    return  sign * (int( x / y ) % 2 ? y - (x - y * int( x / y )) : (x - y * int( x / y ))) ;
}

const uint GRASS_INSTANCES = 10000; // Количество травинок
const uint GROUND_SIDE = 1000;
const uint GRASS_SIDE = 100;
const uint TITLE_MAP_SIDE = 50;

GL::Camera camera;               // Мы предоставляем Вам реализацию камеры. В OpenGL камера - это просто 2 матрицы. Модельно-видовая матрица и матрица проекции. // ###
                                 // Задача этого класса только в том чтобы обработать ввод с клавиатуры и правильно сформировать эти матрицы.
                                 // Вы можете просто пользоваться этим классом для расчёта указанных матриц.


GLuint grassPointsCount; // Количество вершин у модели травинки
GLuint grassShader;      // Шейдер, рисующий траву
GLuint grassVAO;         // VAO для травы (что такое VAO почитайте в доках)
GLuint grassRegularTiltingBuffer;    // Буфер для смещения координат травинок
GLuint grassRandomTiltingBuffer;    // Буфер для смещения координат травинок
vector<GLfloat> grassRegularTilting(GRASS_INSTANCES); // Вектор с углами наклона травинок

vector<VM::vec2> longTilting(GRASS_INSTANCES);
vector<VM::vec2> grassRandomTilting(GRASS_INSTANCES); // Вектор с углами наклона травинок

vector<GLfloat> grassRotations(GRASS_INSTANCES);
vector<VM::vec3> grassPositions;

GLuint groundShader; // Шейдер для земли
GLuint groundVAO; // VAO для земли
GLuint texture;   // текстура земли
GLuint grassTexture;   // текстура травы
GLuint groundPointsCount;

GLfloat altitudeMap[GROUND_SIDE][GROUND_SIDE];
GLfloat titleMapBound[TITLE_MAP_SIDE][TITLE_MAP_SIDE];
GLfloat titleMap[TITLE_MAP_SIDE][TITLE_MAP_SIDE];

// Размеры экрана
uint screenWidth = 800;
uint screenHeight = 600;

// Это для захвата мышки. Вам это не потребуется (это не значит, что нужно удалять эту строку)
bool captureMouse = true;

// Функция, рисующая замлю
void DrawGround() {
    // Используем шейдер для земли
    glUseProgram(groundShader);                                                  CHECK_GL_ERRORS

    // Устанавливаем юниформ для шейдера. В данном случае передадим перспективную матрицу камеры
    // Находим локацию юниформа 'camera' в шейдере
    GLint cameraLocation = glGetUniformLocation(groundShader, "camera");         CHECK_GL_ERRORS
    // Устанавливаем юниформ (загружаем на GPU матрицу проекции?)
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    // Подключаем текстуру
    glBindTexture(GL_TEXTURE_2D, texture);
    // Подключаем VAO, который содержит буферы, необходимые для отрисовки земли
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

    // Рисуем землю
    glDrawArrays(GL_TRIANGLE_STRIP, 0, groundPointsCount);                       CHECK_GL_ERRORS

    // Отсоединяем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    // Отключаем шейдер
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Обновление смещения травинок
void UpdateGrassRegularTilting() {
    // Генерация случайных смещений
    static timeval tv, tv2;
    
    if (tv2.tv_usec == 0)
        gettimeofday(&tv2, 0);
    tv = tv2;
    gettimeofday(&tv2, 0);
    
    
    int SIN_COUNT = int (float(rand()) / RAND_MAX * 10);
    int n = TITLE_MAP_SIDE * 100;
    
    for (int k=1; k<=SIN_COUNT; k++) {
        srand(clock());
        float koef = float(rand()) / RAND_MAX;
        float mov = float(rand()) / RAND_MAX * 2*M_PI;
        for (int i=0; i<TITLE_MAP_SIDE; i++) {
            titleMapBound[i][0] += fabs(sin(M_PI / (TITLE_MAP_SIDE-1) * i * k + mov) / SIN_COUNT * koef) * 0.1
             * fabs(tv2.tv_usec == tv.tv_usec ? 1 : (tv2.tv_sec * 1e6 + tv2.tv_usec - (tv.tv_sec * 1e6 + tv.tv_usec))/30000.0);
        }
    }
    SIN_COUNT = int (float(rand()) / RAND_MAX * 10);

    for (int k=1; k<SIN_COUNT; k++) {
        srand(clock());
        float koef = float(rand()) / RAND_MAX;
        float mov = float(rand()) / RAND_MAX * 2*M_PI;
        for (int i=0; i<TITLE_MAP_SIDE; i++) {
            titleMapBound[i][TITLE_MAP_SIDE-1] += fabs(sin(M_PI / (TITLE_MAP_SIDE-1) * i * k + mov) / SIN_COUNT * koef) * 0.1
             * fabs(tv2.tv_usec == tv.tv_usec ? 1 : (tv2.tv_sec * 1e6 + tv2.tv_usec - (tv.tv_sec * 1e6 + tv.tv_usec))/30000.0);
        }
    }
    for (int i=0; i<TITLE_MAP_SIDE; i++) {
        titleMap[i][0               ] = M_PI/12 + mod(titleMapBound[i][0               ], M_PI/4);
        titleMap[i][TITLE_MAP_SIDE-1] = M_PI/12 + mod(titleMapBound[i][TITLE_MAP_SIDE-1], M_PI/4);

    }
    for (int i=0; i<TITLE_MAP_SIDE; i++) {
        titleMap[0               ][i] = titleMap[0               ][0] + ( titleMap[0               ][TITLE_MAP_SIDE-1] - titleMap[0               ][0] ) / TITLE_MAP_SIDE * i;
        titleMap[TITLE_MAP_SIDE-1][i] = titleMap[TITLE_MAP_SIDE-1][0] + ( titleMap[TITLE_MAP_SIDE-1][TITLE_MAP_SIDE-1] - titleMap[TITLE_MAP_SIDE-1][0] ) / TITLE_MAP_SIDE * i;
    }
    for (int k=0; k<n; k++)
#pragma omp parallel for shared(titleMap)
        for (int i=1; i<TITLE_MAP_SIDE-1; i++)
            for (int j=1+(i%2+k%2)%2; j<TITLE_MAP_SIDE-1; j+=2)
                titleMap[i][j]=(0.2*titleMap[i-1][j]+0.2*titleMap[i+1][j]+0.8*titleMap[i][j-1]+0.8*titleMap[i][j+1])/2;

    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassRegularTilting[i] = titleMap[int(grassPositions[i].x*(TITLE_MAP_SIDE-1))][int(grassPositions[i].z*(TITLE_MAP_SIDE-1))];
    }
    
    // Привязываем буфер, содержащий смещения
    glBindBuffer(GL_ARRAY_BUFFER, grassRegularTiltingBuffer);                                CHECK_GL_ERRORS
    // Загружаем данные в видеопамять
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * GRASS_INSTANCES, grassRegularTilting.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

void UpdateGrassRandomTilting() {
    // Генерация случайных смещений
    static timeval tv, tv2;
    
    if (tv2.tv_usec == 0)
        gettimeofday(&tv2, 0);
    tv = tv2;
    gettimeofday(&tv2, 0);
    
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        longTilting[i].x += float(rand()) / RAND_MAX / 1000
                        * fabs(tv2.tv_usec == tv.tv_usec ? 1 : (tv2.tv_sec * 1e6 + tv2.tv_usec - (tv.tv_sec * 1e6 + tv.tv_usec))/30000.0);
        grassRandomTilting[i].x = mod(longTilting[i].x, M_PI/96);
        longTilting[i].y += float(rand()) / RAND_MAX / 200
                        * fabs(tv2.tv_usec == tv.tv_usec ? 1 : (tv2.tv_sec * 1e6 + tv2.tv_usec - (tv.tv_sec * 1e6 + tv.tv_usec))/30000.0);
        grassRandomTilting[i].y = mod(longTilting[i].y, M_PI/24)+M_PI/12;
    }
    // Привязываем буфер, содержащий смещения
    glBindBuffer(GL_ARRAY_BUFFER, grassRandomTiltingBuffer);                                CHECK_GL_ERRORS
    // Загружаем данные в видеопамять
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassRandomTilting.size(), grassRandomTilting.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}


// Рисование травы
void DrawGrass() {
    // Тут то же самое, что и в рисовании земли
    glUseProgram(grassShader);                                                   CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(grassShader, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    // Подключаем текстуру
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS
    // Обновляем смещения для травы
    UpdateGrassRegularTilting();
    UpdateGrassRandomTilting();
    // Отрисовка травинок в количестве GRASS_INSTANCES
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Эта функция вызывается для обновления экрана
void RenderLayouts() {
    glEnable(GL_MULTISAMPLE);
    // Включение буфера глубины
    glEnable(GL_DEPTH_TEST);
    // Очистка буфера глубины и цветового буфера
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Рисуем меши
    DrawGround();
    DrawGrass();
    glutSwapBuffers();
}

// Завершение программы
void FinishProgram() {
    glutDestroyWindow(glutGetWindow());
}

// Обработка события нажатия клавиши (специальные клавиши обрабатываются в функции SpecialButtons)
void KeyboardEvents(unsigned char key, int x, int y) {
    if (key == 27) {
        FinishProgram();
    } else if (key == 'w') {
        camera.goForward();
    } else if (key == 's') {
        camera.goBack();
    } else if (key == 'm') {
        captureMouse = !captureMouse;
        if (captureMouse) {
            glutWarpPointer(screenWidth / 2, screenHeight / 2);
            glutSetCursor(GLUT_CURSOR_NONE);
        } else {
            glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
        }
    }
}

// Обработка события нажатия специальных клавиш
void SpecialButtons(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT) {
        camera.rotateY(0.02);
    } else if (key == GLUT_KEY_LEFT) {
        camera.rotateY(-0.02);
    } else if (key == GLUT_KEY_UP) {
        camera.rotateTop(-0.02);
    } else if (key == GLUT_KEY_DOWN) {
        camera.rotateTop(0.02);
    }
}

void IdleFunc() {
    glutPostRedisplay();
}

// Обработка события движения мыши
void MouseMove(int x, int y) {
    if (captureMouse) {
        int centerX = screenWidth / 2,
            centerY = screenHeight / 2;
        if (x != centerX || y != centerY) {
            camera.rotateY((x - centerX) / 1000.0f);
            camera.rotateTop((y - centerY) / 1000.0f);
            glutWarpPointer(centerX, centerY);
        }
    }
}

// Обработка нажатия кнопки мыши
void MouseClick(int button, int state, int x, int y) {
}

// Событие изменение размера окна
void windowReshapeFunc(GLint newWidth, GLint newHeight) {
    glViewport(0, 0, newWidth, newHeight);
    screenWidth = newWidth;
    screenHeight = newHeight;

    camera.screenRatio = (float)screenWidth / screenHeight;
}

// Инициализация окна
void InitializeGLUT(int argc, char **argv) {
    glutInit(&argc, argv);
#ifdef __APPLE__
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
#endif
#ifndef __APPLE__
    glutInitContextVersion(3, 3);
    glutInitContextFlags(GLUT_CORE_PROFILE);
    glutInitContextProfile(GLUT_CORE_PROFILE);
#endif
    glutInitWindowPosition(-1, -1);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Computer Graphics 3");
    glutWarpPointer(400, 300);
    glutSetCursor(GLUT_CURSOR_NONE);

    glutDisplayFunc(RenderLayouts);
    glutKeyboardFunc(KeyboardEvents);
    glutSpecialFunc(SpecialButtons);
    glutIdleFunc(IdleFunc);
    glutPassiveMotionFunc(MouseMove);
    glutMouseFunc(MouseClick);
    glutReshapeFunc(windowReshapeFunc);
}

// Генерация позиций травинок (эту функцию вам придётся переписать)
vector<VM::vec3> GenerateGrassPositions() {
    vector<VM::vec3> grassPositions(GRASS_INSTANCES);
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassPositions[i] = VM::vec3((float)rand() / RAND_MAX, 0, (float)rand() / RAND_MAX);
        grassPositions[i].y = altitudeMap[int(grassPositions[i].x*(GROUND_SIDE-1))][int(grassPositions[i].z*(GROUND_SIDE-1))]*0.1;
    }
    return grassPositions;
}

// Генерация матриц поворота травинок вокруг своей оси
void GenerateGrassRotations() {
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassRotations[i] = rand()/float(RAND_MAX)*2*M_PI;
    }
}

// Здесь вам нужно будет генерировать меш
vector<VM::vec4> GenMesh(uint n) {
    vector<VM::vec4> mesh;
    for (int i=0; i<GRASS_SIDE; i++) {
        mesh.push_back(VM::vec4(-0.5, float(i)/(GRASS_SIDE-1), 0, 1));
        mesh.push_back(VM::vec4( 0.5, float(i)/(GRASS_SIDE-1), 0, 1));
    }
    return mesh;
}

// Заполнение карты высот
#define SIN_COUNT 10
void GenAlt(void) {
    for (int k=1; k<SIN_COUNT; k++) {
        srand(clock());
        float koef = float(rand()) / RAND_MAX;
        float mov = float(rand()) / RAND_MAX * 2*M_PI;
        for (int i=0; i<GROUND_SIDE; i++) {
            for (int j=0; j<GROUND_SIDE; j++) {
                altitudeMap[i][j] += sin(M_PI / (GROUND_SIDE-1) * i * k + mov) * sin(M_PI / (GROUND_SIDE-1) * j * k+mov) / SIN_COUNT * koef;
            }
        }
    }
    
}

// Заполнение карты высот
#define SIN_COUNT 10
vector<GLfloat> GenSize(void) {
    std::default_random_engine generator;
    std::extreme_value_distribution<double> distribution(0.3,0.15);

    vector<GLfloat> size(GRASS_INSTANCES);
    for (int i=0; i < GRASS_INSTANCES; i++) {
        size[i] = -(distribution(generator)-1);
        if (size[i]<0) {
            size[i]=0;
        } else {
            if (size[i]>1) {
                size[i]=1;
            }
        }
    }
    return size;
}

// Создание травы
void CreateGrass() {
    uint LOD = 1;
    // Создаём меш
    vector<VM::vec4> grassPoints = GenMesh(LOD);
    // Создаём меш
    vector<GLfloat> grassSizes = GenSize();
    // Сохраняем количество вершин в меше травы
    grassPointsCount = grassPoints.size();
    // Создаём позиции для травинок
    grassPositions = GenerateGrassPositions();
    GenerateGrassRotations();
    /* Компилируем шейдеры
    Эта функция принимает на вход название шейдера 'shaderName',
    читает файлы shaders/{shaderName}.vert - вершинный шейдер
    и shaders/{shaderName}.frag - фрагментный шейдер,
    компилирует их и линкует.
    */
    grassShader = GL::CompileShaderProgram("grass");
    
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        longTilting[i].x = float(rand()) / RAND_MAX * (M_PI/12) + (M_PI/24);
        longTilting[i].y = float(rand()) / RAND_MAX * (M_PI/12) + (M_PI/24);
    }

    ILuint	id;
    ilEnable(IL_FORMAT_SET);
    ilSetInteger(IL_FORMAT_MODE,IL_RGBA);
    ilGenImages ( 1, &id );
    ilBindImage ( id );
    ilLoadImage("../Texture/grass2.png");
    int err=ilGetError();
    if (err!=IL_NO_ERROR) {
        cerr << "ERROR during file loading:" << iluErrorString(err) << endl;
        exit(-1);
    }
    glGenTextures(1, &grassTexture);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger ( IL_IMAGE_WIDTH ), ilGetInteger ( IL_IMAGE_HEIGHT ),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);
    ilDeleteImages( 1, &id);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Здесь создаём буфер
    GLuint pointsBuffer;
    // Это генерация одного буфера (в pointsBuffer хранится идентификатор буфера)
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    // Привязываем сгенерированный буфер
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    // Заполняем буфер данными из вектора
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * grassPoints.size(), grassPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    // Создание VAO
    // Генерация VAO
    glGenVertexArrays(1, &grassVAO);                                             CHECK_GL_ERRORS
    // Привязка VAO
    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS

    // Получение локации параметра 'point' в шейдере
    GLuint pointsLocation = glGetAttribLocation(grassShader, "point");           CHECK_GL_ERRORS
    // Подключаем массив атрибутов к данной локации
    glEnableVertexAttribArray(pointsLocation);                                   CHECK_GL_ERRORS
    // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
    glVertexAttribPointer(pointsLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);          CHECK_GL_ERRORS

    // Создаём буфер для позиций травинок
    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    
    GLuint positionLocation = glGetAttribLocation(grassShader, "position");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(positionLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
    glVertexAttribDivisor(positionLocation, 1);                                  CHECK_GL_ERRORS
    
    // Создаём буфер для поворота травинок
    GLuint rotationBuffer;
    glGenBuffers(1, &rotationBuffer);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, rotationBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * grassRotations.size(), grassRotations.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    
    GLuint rotationLocation = glGetAttribLocation(grassShader, "rotationAngle");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(rotationLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(rotationLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(rotationLocation, 1);                                  CHECK_GL_ERRORS

    
    // Создаём буфер для размера травинок
    GLuint sizeBuffer;
    glGenBuffers(1, &sizeBuffer);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, sizeBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * grassSizes.size(), grassSizes.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    
    GLuint sizeLocation = glGetAttribLocation(grassShader, "size");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(sizeLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(sizeLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(sizeLocation, 1);                                  CHECK_GL_ERRORS

    
    // Создаём буфер для смещения травинок
    glGenBuffers(1, &grassRegularTiltingBuffer);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassRegularTiltingBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * GRASS_INSTANCES, grassRegularTilting.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    
    GLuint varianceLocation = glGetAttribLocation(grassShader, "tiltAngle");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(varianceLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(varianceLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(varianceLocation, 1);                                  CHECK_GL_ERRORS
    
    // Создаём буфер для смещения травинок
    glGenBuffers(1, &grassRandomTiltingBuffer);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassRandomTiltingBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassRandomTilting.size(), grassRandomTilting.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    
    GLuint grassRandomTiltingLocation = glGetAttribLocation(grassShader, "randomTiltAngle");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(grassRandomTiltingLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(grassRandomTiltingLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(grassRandomTiltingLocation, 1);                                  CHECK_GL_ERRORS
    
    // Отвязываем VAO
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

// Создаём камеру (Если шаблонная камера вам не нравится, то можете переделать, но я бы не стал)
void CreateCamera() {
    camera.angle = 45.0f / 180.0f * M_PI;
    camera.direction = VM::vec3(0, 0.3, -1);
    camera.position = VM::vec3(0.5, 0.2, 0);
    camera.screenRatio = (float)screenWidth / screenHeight;
    camera.up = VM::vec3(0, 1, 0);
    camera.zfar = 50.0f;
    camera.znear = 0.05f;
}

// Создаём меш земли
vector <VM::vec3> GenGroundMesh() {
    vector <VM::vec3> mesh;
    for (int i=0; i<GROUND_SIDE-1; i++) {
        if (i!=0) mesh.pop_back();
        for (int j=(i%2==0 ? 0 : GROUND_SIDE-1); (i%2==0 ? j<GROUND_SIDE : j>=0); j+=(i%2==0 ? 1 : -1)) {
            mesh.push_back(VM::vec3(GLfloat(i  ) / GROUND_SIDE, altitudeMap[i  ][j]*0.1, GLfloat(j) / GROUND_SIDE));
            mesh.push_back(VM::vec3(GLfloat(i+1) / GROUND_SIDE, altitudeMap[i+1][j]*0.1, GLfloat(j) / GROUND_SIDE));
        }
    }
    return mesh;
}

// Создаём замлю
void CreateGround() {
    // Земля состоит из двух треугольников
    groundPointsCount = 2*GROUND_SIDE*GROUND_SIDE - 3*GROUND_SIDE + 2;
    GenAlt();
    vector <VM::vec3> meshPoints = GenGroundMesh();
    ILuint	id;
    ilGenImages ( 1, &id );
    ilBindImage ( id );
    ilLoadImage("../Texture/ground2.png");
    int err=ilGetError();
    if (err!=IL_NO_ERROR) {
        cerr << "ERROR during file loading:" << iluErrorString(err) << endl;
        exit(-1);
    }
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ilGetInteger ( IL_IMAGE_WIDTH ), ilGetInteger ( IL_IMAGE_HEIGHT ),
                 0, GL_RGB, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);
    ilDeleteImages( 1, &id);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Подробнее о том, как это работает читайте в функции CreateGrass

    groundShader = GL::CompileShaderProgram("ground");

    GLuint pointsBuffer;
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * meshPoints.size(), meshPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    glGenVertexArrays(1, &groundVAO);                                            CHECK_GL_ERRORS
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

    GLuint index = glGetAttribLocation(groundShader, "point");                   CHECK_GL_ERRORS
    glEnableVertexAttribArray(index);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, 0, 0);                   CHECK_GL_ERRORS

    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

int main(int argc, char **argv)
{
    try {
        IL_init();
        cout << "Start" << endl;
        InitializeGLUT(argc, argv);
        cout << "GLUT inited" << endl;
#ifndef __APPLE__
        glewInit();
        cout << "glew inited" << endl;
#endif
        glEnable(GL_MULTISAMPLE);

        CreateCamera();
        cout << "Camera created" << endl;
        CreateGround();
        cout << "Ground created" << endl;
        CreateGrass();
        cout << "Grass created" << endl;
        glutMainLoop();
    } catch (string s) {
        cout << s << endl;
    }
}
