#include <iostream>
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <random>
#include <sstream>

#include "Utility.h"
#include "Texture.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;

class Shader {
public:
    GLuint Program;
    Shader(){};
    // Constructor generates the shader on the fly
    Shader(const GLchar* vertexPath, const GLchar* fragmentPath) {
        // 1. Retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensures ifstream objects can throw exceptions:
        vShaderFile.exceptions (std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::badbit);
        try {
            // Open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // Read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // Convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const GLchar* vShaderCode = vertexCode.c_str();
        const GLchar * fShaderCode = fragmentCode.c_str();
        // 2. Compile shaders
        GLuint vertex, fragment;
        GLint success;
        GLchar infoLog[512];
        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        // Print compile errors if any
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        // Print compile errors if any
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // Shader Program
        this->Program = glCreateProgram();
        glAttachShader(this->Program, vertex);
        glAttachShader(this->Program, fragment);
        glLinkProgram(this->Program);
        // Print linking errors if any
        glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        // Delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        
    }
    // Uses the current shader
    void Use() {
        glUseProgram(this->Program);
    }
};


struct Vertex {
    VM::vec3 Position;
    VM::vec3 Normal;
    VM::vec2 TexCoords;
};

struct Texture {
    GLuint id;
    string type;
    aiString path; // We store the path of the texture to compare with other textures
};

class Mesh {
public:
    /* Mesh Data */
    vector<Vertex> vertices;
    vector<GLuint> indices;
    vector<Texture> textures;
    /* Functions */
    Mesh(vector<Vertex> vertices, vector<GLuint> indices, vector<Texture> textures) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->setupMesh();
    };
    void Draw(Shader shader, int instance_count) {
        GLuint diffuseNr = 1;
        GLuint specularNr = 1;
        for(GLuint i = 0; i < this->textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // Activate proper texture unit before binding
            // Retrieve texture number (the N in diffuse_textureN)
            stringstream ss;
            string number;
            string name = this->textures[i].type;
            if(name == "texture_diffuse")
                ss << diffuseNr++; // Transfer GLuint to stream
            else if(name == "texture_specular")
                ss << specularNr++; // Transfer GLuint to stream
            number = ss.str();
            glUniform1f(glGetUniformLocation(shader.Program, ("material." + name + number).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
        }
        glActiveTexture(GL_TEXTURE0);
        // Draw mesh
        glBindVertexArray(this->VAO);
        glDrawElementsInstanced(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0, instance_count);
        glBindVertexArray(0);
    };
    /* Render data */
    GLuint VAO, VBO, EBO;
private:
    /* Functions    */
    void setupMesh() {
        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &this->VBO);
        glGenBuffers(1, &this->EBO);
        glBindVertexArray(this->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);
        // Vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
        // Vertex Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
        // Vertex Texture Coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));
        glBindVertexArray(0);
    };
};

GLint TextureFromFile(const char* path, string directory) {
    //Generate texture ID and load texture data
    string filename = string(path);
    filename = directory + '/' + filename;
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width,height;
    unsigned char* image = SOIL_load_image(filename.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
    // Assign texture to ID
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Parameters
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);
    return textureID;
}

class Model {
public:
    /*  Functions   */
    // Constructor, expects a filepath to a 3D model.
    Model(){};
    Model(GLchar* path) {
        this->loadModel(path);
    }
    
    // Draws the model, and thus all its meshes
    void Draw(Shader shader, int instance_count = 1) {
        for(GLuint i = 0; i < this->meshes.size(); i++)
        this->meshes[i].Draw(shader, instance_count);
    }
    
    /*  Model Data  */
    vector<Mesh> meshes;
private:
    string directory;
    vector<Texture> textures_loaded;	// Stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    
    /*  Functions   */
    // Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string path) {
        // Read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
        // Check for errors
        if(!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) { // if is Not Zero
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // Retrieve the directory path of the filepath
        this->directory = path.substr(0, path.find_last_of('/'));
        
        // Process ASSIMP's root node recursively
        this->processNode(scene->mRootNode, scene);
    }
    
    // Processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene) {
        // Process each mesh located at the current node
        for(GLuint i = 0; i < node->mNumMeshes; i++) {
            // The node object only contains indices to index the actual objects in the scene.
            // The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            this->meshes.push_back(this->processMesh(mesh, scene));
        }
        // After we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(GLuint i = 0; i < node->mNumChildren; i++) {
            this->processNode(node->mChildren[i], scene);
        }
        
    }
    
    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        // Data to fill
        vector<Vertex> vertices;
        vector<GLuint> indices;
        vector<Texture> textures;
        
        // Walk through each of the mesh's vertices
        for(GLuint i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            VM::vec3 vector; // We declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to VM's vec3 class so we transfer the data to this placeholder VM::vec3 first.
            // Positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // Normals
            if (mesh->mNormals) {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
            }
            vertex.Normal = vector;
            // Texture Coordinates
            if(mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates?
            {
                VM::vec2 vec;
                // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = VM::vec2(0.0f, 0.0f);
            vertices.push_back(vertex);
        }
        // Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(GLuint i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // Retrieve all indices of the face and store them in the indices vector
            for(GLuint j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // Process materials
        if(mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            // We assume a convention for sampler names in the shaders. Each diffuse texture should be named
            // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
            // Same applies to other texture as the following list summarizes:
            // Diffuse: texture_diffuseN
            // Specular: texture_specularN
            // Normal: texture_normalN
            
            // 1. Diffuse maps
            vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            // 2. Specular maps
            vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }
        
        // Return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }
    
    // Checks all material textures of a given type and loads the textures if they're not loaded yet.
    // The required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName) {
        vector<Texture> textures;
        for(GLuint i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            // Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            GLboolean skip = false;
            for(GLuint j = 0; j < textures_loaded.size(); j++) {
                if(textures_loaded[j].path == str)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip) {   // If texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str;
                textures.push_back(texture);
                this->textures_loaded.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};

double mod(double x, double y) {
    int sign = 1;
    if (x<0) {
        x = -x;
        sign = -1;
    }
    return  sign * (int( x / y ) % 2 ? y - (x - y * int( x / y )) : (x - y * int( x / y ))) ;
}

const uint GRASS_INSTANCES = 30000; // Количество травинок
const uint ROSES_INSTANCES = 50; // Количество роз
const uint GROUND_SIDE = 1000;
const uint GRASS_SIDE = 100;
const uint TITLE_MAP_SIDE = 50;

GL::Camera camera;               // Мы предоставляем Вам реализацию камеры. В OpenGL камера - это просто 2 матрицы. Модельно-видовая матрица и матрица проекции. // ###
                                 // Задача этого класса только в том чтобы обработать ввод с клавиатуры и правильно сформировать эти матрицы.
                                 // Вы можете просто пользоваться этим классом для расчёта указанных матриц.
VM::vec3 light_source(1,0.3,-0.35);
Shader treeShader;
Model treeModel;

Shader stoneShader;
Model stoneModel;

Shader roseShader;
Model roseModel;

GLuint grassPointsCount; // Количество вершин у модели травинки
GLuint grassShader;      // Шейдер, рисующий траву
GLuint grassVAO;         // VAO для травы (что такое VAO почитайте в доках)
GLuint grassRegularTiltingBuffer;    // Буфер для смещения координат травинок
GLuint grassRandomTiltingBuffer;    // Буфер для смещения координат травинок
vector<VM::vec2> longTilting(GRASS_INSTANCES);
vector<VM::vec2> grassRandomTilting(GRASS_INSTANCES); // Вектор с углами наклона травинок

vector<GLfloat> grassRotations(GRASS_INSTANCES);
vector<VM::vec3> grassPositions(GRASS_INSTANCES);
vector<VM::vec3> rosesPositions(ROSES_INSTANCES);

GLuint groundShader; // Шейдер для земли
GLuint skyboxShader; // Шейдер для земли
GLuint butterflyShader; // Шейдер для земли
GLuint groundVAO; // VAO для земли
GLuint skyboxVAO; // VAO для земли
GLuint butterflyVAO; // VAO для земли
GLuint texture;   // текстура земли
GLuint grassTexture;   // текстура травы
GLuint SkyBoxTexture;
GLuint butterflyTexture[3];
GLuint groundPointsCount;

GLfloat altitudeMap[GROUND_SIDE][GROUND_SIDE];

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
    // Находим локацию юниформа 'cameraPos' в шейдере
    GLint cameraPosLocation = glGetUniformLocation(groundShader, "cameraPos");         CHECK_GL_ERRORS
    // Устанавливаем юниформ
    glUniform3fv(cameraPosLocation, 1, (GLfloat *)&(camera.position)); CHECK_GL_ERRORS
    // Подключаем текстуру
    // Находим локацию юниформа 'source_coord' в шейдере
    GLint source_coordLocation = glGetUniformLocation(groundShader, "source_coord");         CHECK_GL_ERRORS
    // Устанавливаем юниформ
    glUniform3fv(source_coordLocation, 1, (GLfloat *)&light_source); CHECK_GL_ERRORS
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

void DrawSkyBox() {
    glDepthMask(GL_FALSE);
    glUseProgram(skyboxShader);                                                 CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(skyboxShader, "camera");         CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBoxTexture);
    glBindVertexArray(skyboxVAO);                                                CHECK_GL_ERRORS
    glDrawArrays(GL_TRIANGLES, 0, 36);                       CHECK_GL_ERRORS
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
    glDepthMask(GL_TRUE);
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
    static timeval tv2;
    gettimeofday(&tv2, 0);
    GLfloat time = (tv2.tv_sec %1000 * 1e6 + tv2.tv_usec)/0.5e6;

    // Тут то же самое, что и в рисовании земли
    glUseProgram(grassShader);                                                   CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(grassShader, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    GLint source_coordLocation = glGetUniformLocation(grassShader, "source_coord");         CHECK_GL_ERRORS
    glUniform3fv(source_coordLocation, 1, (GLfloat *)&light_source); CHECK_GL_ERRORS
    GLint cameraPosLocation = glGetUniformLocation(grassShader, "cameraPos");         CHECK_GL_ERRORS
    glUniform3fv(cameraPosLocation, 1, (GLfloat *)&(camera.position)); CHECK_GL_ERRORS
    GLint timePosLocation = glGetUniformLocation(grassShader, "time");         CHECK_GL_ERRORS
    glUniform1f(timePosLocation, time); CHECK_GL_ERRORS

    // Подключаем текстуру
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS
    // Обновляем смещения для травы
    UpdateGrassRandomTilting();
    // Отрисовка травинок в количестве GRASS_INSTANCES
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

void DrawButterfly() {
    static timeval tv2;
    gettimeofday(&tv2, 0);
    GLfloat time = (tv2.tv_sec %1000 * 1e6 + tv2.tv_usec)/0.5e5;
    
    glUseProgram(butterflyShader);                                                   CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(butterflyShader, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    GLint source_coordLocation = glGetUniformLocation(butterflyShader, "source_coord");         CHECK_GL_ERRORS
    glUniform3fv(source_coordLocation, 1, (GLfloat *)&light_source); CHECK_GL_ERRORS
    GLint cameraPosLocation = glGetUniformLocation(butterflyShader, "cameraPos");         CHECK_GL_ERRORS
    glUniform3fv(cameraPosLocation, 1, (GLfloat *)&(camera.position)); CHECK_GL_ERRORS
    GLint timePosLocation = glGetUniformLocation(butterflyShader, "time");         CHECK_GL_ERRORS
    glUniform1f(timePosLocation, time); CHECK_GL_ERRORS
    
    // Подключаем текстуру
    for (int i=0; i<3; i++) {
        glActiveTexture(GL_TEXTURE0+i);                                                 CHECK_GL_ERRORS
        glBindTexture(GL_TEXTURE_2D, butterflyTexture[i]);                                                 CHECK_GL_ERRORS
    }
    glBindVertexArray(butterflyVAO);                                                 CHECK_GL_ERRORS
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, 3);   CHECK_GL_ERRORS
    for (int i=0; i<3; i++) {
        glActiveTexture(GL_TEXTURE0+i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

void DrawModel(Model & model, Shader & shader, VM::vec3 transl) {
    shader.Use();   // <-- Don't forget this one!
    GLint cameraLocation = glGetUniformLocation(shader.Program, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    GLint source_coordLocation = glGetUniformLocation(shader.Program, "source_coord");         CHECK_GL_ERRORS
    glUniform3fv(source_coordLocation, 1, (GLfloat *)&light_source); CHECK_GL_ERRORS
    GLint cameraPosLocation = glGetUniformLocation(shader.Program, "cameraPos");         CHECK_GL_ERRORS
    glUniform3fv(cameraPosLocation, 1, (GLfloat *)&(camera.position)); CHECK_GL_ERRORS
    GLint translationPosLocation = glGetUniformLocation(shader.Program, "translation");         CHECK_GL_ERRORS
    glUniform3fv(translationPosLocation, 1, (GLfloat *)&(transl)); CHECK_GL_ERRORS

    model.Draw(shader);
}

void DrawRose() {
    static timeval tv2;
    gettimeofday(&tv2, 0);
    GLfloat time = (tv2.tv_sec %1000 * 1e6 + tv2.tv_usec)/0.5e6;
    
    roseShader.Use();
    GLint cameraLocation = glGetUniformLocation(roseShader.Program, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
    GLint source_coordLocation = glGetUniformLocation(roseShader.Program, "source_coord");         CHECK_GL_ERRORS
    glUniform3fv(source_coordLocation, 1, (GLfloat *)&light_source); CHECK_GL_ERRORS
    GLint cameraPosLocation = glGetUniformLocation(roseShader.Program, "cameraPos");         CHECK_GL_ERRORS
    glUniform3fv(cameraPosLocation, 1, (GLfloat *)&(camera.position)); CHECK_GL_ERRORS
    GLint timePosLocation = glGetUniformLocation(roseShader.Program, "time");         CHECK_GL_ERRORS
    glUniform1f(timePosLocation, time); CHECK_GL_ERRORS
    
    roseModel.Draw(roseShader, ROSES_INSTANCES);
}

// Эта функция вызывается для обновления экрана
void RenderLayouts() {
    glEnable(GL_MULTISAMPLE);
    // Включение буфера глубины
    glEnable(GL_DEPTH_TEST);
    // Очистка буфера глубины и цветового буфера
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Рисуем меши
    DrawSkyBox();
    DrawGround();
    DrawGrass();
    DrawModel(treeModel, treeShader, VM::vec3(0.5, altitudeMap[int(0.5*(GROUND_SIDE-1))][int(0.5*(GROUND_SIDE-1))]*0.1, 0.5));
    DrawModel(stoneModel, stoneShader, VM::vec3(0.7, altitudeMap[int(0.7*(GROUND_SIDE-1))][int(0.3*(GROUND_SIDE-1))]*0.1, 0.3));
    DrawRose();
    DrawButterfly();

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
void GenerateGrassPositions(vector<VM::vec3> & grassPositions) {
    for (uint i = 0; i < grassPositions.size(); ++i) {
        grassPositions[i] = VM::vec3((float)rand() / RAND_MAX, 0, (float)rand() / RAND_MAX);
        if (pow((grassPositions[i].x-0.75),2)+pow((grassPositions[i].z-0.75),2)<0.0003 ||
            pow((grassPositions[i].x-0.85),2)+pow((grassPositions[i].z-0.65),2)<0.003    ) {
            i--;
            continue;
        }
        grassPositions[i].y = altitudeMap[int(grassPositions[i].x*(GROUND_SIDE-1))][int(grassPositions[i].z*(GROUND_SIDE-1))]*0.1;
    }
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
    GenerateGrassPositions(grassPositions);
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

    glGenTextures(1, &grassTexture);
    glBindTexture(GL_TEXTURE_2D, grassTexture);

    int width,height;
    unsigned char* image;
    image = SOIL_load_image("../Texture/grass2.png", &width, &height, 0, SOIL_LOAD_RGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                              CHECK_GL_ERRORS
    glGenerateMipmap(GL_TEXTURE_2D);                                              CHECK_GL_ERRORS
    glBindTexture(GL_TEXTURE_2D, 0);                                              CHECK_GL_ERRORS
    SOIL_free_image_data(image);
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
    camera.direction = VM::vec3(-0.775125861, 0.011804048, -0.631696522);
    camera.position = VM::vec3(-0.814950823, 0.113559671, -0.809640347);
    camera.screenRatio = (float)screenWidth / screenHeight;
    camera.up = VM::vec3(0, 1, 0);
    camera.zfar = 50.0f;
    camera.znear = 0.05f;
}

vector <VM::vec3> meshPoints;
vector <VM::vec3> normls;

// Создаём меш земли
void GenGroundMesh() {
    size_t size;
    VM::vec3 tmp;
    for (int i=0; i<GROUND_SIDE-1; i++) {
        if (i!=0) {
            meshPoints.pop_back();
            normls.pop_back();
        }
        for (int j=(i%2==0 ? 0 : GROUND_SIDE-1); (i%2==0 ? j<GROUND_SIDE : j>=0); j+=(i%2==0 ? 1 : -1)) {
            meshPoints.push_back(VM::vec3(GLfloat(i  ) / GROUND_SIDE, altitudeMap[i  ][j]*0.1, GLfloat(j) / GROUND_SIDE));
            if (meshPoints.size() == 3) {
                tmp = normalize(cross(meshPoints[0]-meshPoints[1], meshPoints[0]-meshPoints[2]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);
                tmp = normalize(cross(meshPoints[0]-meshPoints[1], meshPoints[0]-meshPoints[2]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);
                tmp = normalize(cross(meshPoints[0]-meshPoints[1], meshPoints[0]-meshPoints[2]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);

            } else if ((size = meshPoints.size()) > 3) {
                tmp = normalize(cross(meshPoints[size-3]-meshPoints[size-2], meshPoints[size-3]-meshPoints[size-1]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);
            }
            
            meshPoints.push_back(VM::vec3(GLfloat(i+1) / GROUND_SIDE, altitudeMap[i+1][j]*0.1, GLfloat(j) / GROUND_SIDE));
            if (meshPoints.size() == 3) {
                tmp = normalize(cross(meshPoints[0]-meshPoints[1], meshPoints[0]-meshPoints[2]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);
                tmp = normalize(cross(meshPoints[0]-meshPoints[1], meshPoints[0]-meshPoints[2]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);
                tmp = normalize(cross(meshPoints[0]-meshPoints[1], meshPoints[0]-meshPoints[2]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);
                
            } else if ((size = meshPoints.size()) > 3) {
                tmp = normalize(cross(meshPoints[size-3]-meshPoints[size-2], meshPoints[size-3]-meshPoints[size-1]));
                if (tmp.y < 0) tmp *= -1;
                normls.push_back(tmp);
            }
        }
    }
}

// Создаём замлю
void CreateGround() {
    // Земля состоит из двух треугольников
    groundPointsCount = 2*GROUND_SIDE*GROUND_SIDE - 3*GROUND_SIDE + 2;
    GenAlt();
    GenGroundMesh();
    int width,height;
    unsigned char* image;
    image = SOIL_load_image("../Texture/ground2.png", &width, &height, 0, SOIL_LOAD_RGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);                                              CHECK_GL_ERRORS
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                              CHECK_GL_ERRORS
    glGenerateMipmap(GL_TEXTURE_2D);                                              CHECK_GL_ERRORS
    glBindTexture(GL_TEXTURE_2D, 0);                                              CHECK_GL_ERRORS
    SOIL_free_image_data(image);
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
    
    GLuint normalsBuffer;
    glGenBuffers(1, &normalsBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec3) * normls.size(), normls.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    
    GLuint normal_location = glGetAttribLocation(groundShader, "normal");                  CHECK_GL_ERRORS
    glEnableVertexAttribArray(normal_location);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, 0, 0);                   CHECK_GL_ERRORS
    
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

void CreateSkyBox() {
    
    /* ---------------------------- SKY BOX TEXTURE ---------------------------- */
    glGenTextures(1, &SkyBoxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBoxTexture);
    int width,height;
    const char* textures_faces[] = {"../Texture/skybox/FullMoonLeft2048.png",
        "../Texture/skybox/FullMoonRight2048.png",
        "../Texture/skybox/FullMoonTop2048.png",
        "../Texture/skybox/FullMoonBottom2048.png",
        "../Texture/skybox/FullMoonFront2048.png",
        "../Texture/skybox/FullMoonBack2048.png"};
    unsigned char* image;
    for(GLuint i = 0; i < 6; i++) {
        image = SOIL_load_image(textures_faces[i], &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        SOIL_free_image_data(image);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    /* ---------------------------- SKY BOX VERTICES ---------------------------- */
    GLfloat skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f, 1.0f, -1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, 1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, 1.0f, -1.0f,  1.0f
    };
    
    skyboxShader = GL::CompileShaderProgram("skybox");
    
    GLuint pointsBuffer;
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW); CHECK_GL_ERRORS
    
    glGenVertexArrays(1, &skyboxVAO);                                            CHECK_GL_ERRORS
    glBindVertexArray(skyboxVAO);                                                CHECK_GL_ERRORS
    
    GLuint index = glGetAttribLocation(skyboxShader, "point");                   CHECK_GL_ERRORS
    glEnableVertexAttribArray(index);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, 0, 0);                   CHECK_GL_ERRORS
    
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

void CreateButterfly() {
    butterflyShader = GL::CompileShaderProgram("butterfly");

    /* ---------------------------- BUTTERFLY TEXTURE ---------------------------- */
    glGenTextures(3, butterflyTexture);                                              CHECK_GL_ERRORS
    vector<string> ind = {"1","2","3"};
    glUseProgram(butterflyShader);
    for (int i=0; i<3; i++) {
        GLuint textLocation = glGetUniformLocation(butterflyShader, (string("text")+ind[i]).c_str());                                           CHECK_GL_ERRORS
        glUniform1i(textLocation, i);                                           CHECK_GL_ERRORS
        
        glActiveTexture(GL_TEXTURE0+i);                                           CHECK_GL_ERRORS
        glBindTexture(GL_TEXTURE_2D, butterflyTexture[i]);                                              CHECK_GL_ERRORS
        
        int width,height;
        unsigned char* image;
        image = SOIL_load_image((string("../Texture/butterflies/butterfly")+ind[i]+string(".png")).c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);                                              CHECK_GL_ERRORS
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);                                              CHECK_GL_ERRORS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);                                              CHECK_GL_ERRORS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);                                              CHECK_GL_ERRORS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                              CHECK_GL_ERRORS
        glGenerateMipmap(GL_TEXTURE_2D);                                              CHECK_GL_ERRORS
        glBindTexture(GL_TEXTURE_2D, 0);                                              CHECK_GL_ERRORS
        SOIL_free_image_data(image);
    }
    glActiveTexture(GL_TEXTURE0);                                           CHECK_GL_ERRORS
    glUseProgram(0);

    
    /* ---------------------------- BUTTERFLY VERTICES ---------------------------- */
    GLfloat vertices[] = {-1,0,-1,1,0,0,0,1,1,0,1,1};
    GLfloat rads[] = {0.8, 0.4, 0.5};
    GLfloat center[] = { 0,  0.1,   0,
                         0.5,  0.15,  0.5,
                         -0.4,  0.125, -0.4 };
    
    glGenVertexArrays(1, &butterflyVAO);                                            CHECK_GL_ERRORS
    glBindVertexArray(butterflyVAO);                                                CHECK_GL_ERRORS

    GLuint pointsBuffer;
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);   CHECK_GL_ERRORS
    
    GLuint index = glGetAttribLocation(butterflyShader, "point");                   CHECK_GL_ERRORS
    glEnableVertexAttribArray(index);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, 0, 0);                   CHECK_GL_ERRORS
    
    GLuint radiusBuffer;
    glGenBuffers(1, &radiusBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, radiusBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(rads), rads, GL_STATIC_DRAW);   CHECK_GL_ERRORS
    
    index = glGetAttribLocation(butterflyShader, "radius");                   CHECK_GL_ERRORS
    glEnableVertexAttribArray(index);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, 0, 0);                   CHECK_GL_ERRORS
    glVertexAttribDivisor(index, 1);                                  CHECK_GL_ERRORS

    GLuint centerBuffer;
    glGenBuffers(1, &centerBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, centerBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(center), center, GL_STATIC_DRAW);   CHECK_GL_ERRORS
    
    index = glGetAttribLocation(butterflyShader, "center");                   CHECK_GL_ERRORS
    glEnableVertexAttribArray(index);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, 0, 0);                   CHECK_GL_ERRORS
    glVertexAttribDivisor(index, 1);                                  CHECK_GL_ERRORS

    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}

void CreateRoses() {
    GenerateGrassPositions(rosesPositions);
    
    roseShader = Shader("shaders/rose.vert", "shaders/rose.frag");
    roseModel = Model("../Texture/rose/rose.obj");
    
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, rosesPositions.size() * sizeof(VM::vec3), rosesPositions.data(), GL_STATIC_DRAW);

    for(GLuint i = 0; i < roseModel.meshes.size(); i++) {
        GLuint VAO = roseModel.meshes[i].VAO;
        glBindVertexArray(VAO);
        // Set attribute pointers for matrix (4 times vec4)
        GLuint location = glGetAttribLocation(roseShader.Program, "translation");      CHECK_GL_ERRORS
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(location, 1);
        
        glBindVertexArray(0);
    }

}

int main(int argc, char **argv)
{
    try {
        cout << "Start" << endl;
        InitializeGLUT(argc, argv);
        cout << "GLUT inited" << endl;
#ifndef __APPLE__
        glewInit();
        cout << "glew inited" << endl;
#endif
        glEnable(GL_MULTISAMPLE);
        CreateSkyBox();
        cout << "SkyBox created" << endl;
        CreateCamera();
        cout << "Camera created" << endl;
        CreateGround();
        cout << "Ground created" << endl;
        CreateGrass();
        cout << "Grass created" << endl;
        // CREATE TREE
        treeShader = Shader("shaders/tree.vert", "shaders/tree.frag");
        treeModel = Model("../Texture/Tree-1/Tree.obj");
        cout << "Tree created" << endl;
        // CREATE STONE
        stoneShader = Shader("shaders/stone.vert", "shaders/stone.frag");
        stoneModel = Model("../Texture/Rock1/Rock1.obj");
        cout << "Stone created" << endl;
        CreateRoses();
        cout << "Roses created" << endl;
        CreateButterfly();
        cout << "Butterfly created" << endl;
        
        glutMainLoop();
    } catch (string s) {
        cout << s << endl;
    }
}
