#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <SOIL.h>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "LoadShaders.h"
#include <tuple>
#include <ft2build.h>
#include <map>
#include <string>
#include <sstream>
#include <opencv2/opencv.hpp>

#ifdef _DEBUG
#define lnkLIB(name) name "d"
#else
#define lnkLIB(name) name
#endif

#define CV_VERSION_ID CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#define cvLIB(name) lnkLIB("opencv_" name CV_VERSION_ID)
#pragma comment( lib, cvLIB("core"))
#pragma comment( lib, cvLIB("highgui"))

#include FT_FREETYPE_H 

#ifdef GLEW_STATIC
#   pragma comment(lib,"glew32s.lib")
#else
#   pragma comment(lib,"glew32.lib")
#endif

#ifdef _DEBUG
#   pragma comment(lib,"SOIL_staticd.lib")
#   pragma comment(lib,"freetype262_staticd.lib")
#else
#   pragma comment(lib,"SOIL_static.lib")
#   pragma comment(lib,"freetype262_static.lib")
#endif

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};


//窗口大小
const GLuint WIDTH = 600, HEIGHT = 600;
//顶点数组对象(Vertex Array Object, VAO)
static GLuint qrVAO = 0;
static GLuint qrEBO = 0;
static GLuint labelVAO = 0;
static GLuint labelVBO = 0;
static GLuint qrTexture;
static GLuint labelTexture;
static GLuint qrShaderProgram;
static GLuint labelShaderProgram;
static std::string labelContent;
static cv::VideoWriter outputVideo;
static const int FRAME_RATE = 0;

std::map<GLchar, Character> Characters;

void initText()
{
    // Set OpenGL options
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //处理并预备添加字体
    ShaderInfo labelShaders[] = {
        { GL_VERTEX_SHADER, "label.vert" },
        { GL_FRAGMENT_SHADER, "label.frag" },
        { GL_NONE, NULL }
    };   
    labelShaderProgram = loadShaders(labelShaders);
    if (labelShaderProgram != 0)//加载成功
    {
        //我们这里不再单独使用单独的着色器，所以，直接删除
        for (ShaderInfo *entry = labelShaders; entry->type != GL_NONE; ++entry) {
            glDeleteShader(entry->shader);
            entry->shader = 0;
        }
    }

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(WIDTH), 0.0f, static_cast<GLfloat>(HEIGHT));
    glUseProgram(labelShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(labelShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    FT_Face face;
    if (FT_New_Face(ft, "arial.ttf", 0, &face))
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint txtTexture;
        glGenTextures(1, &txtTexture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, txtTexture);
        glUniform1i(glGetUniformLocation(qrShaderProgram, "txtTexture"), 0);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
            );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            txtTexture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &labelVAO);
    glGenBuffers(1, &labelVBO);
    glBindVertexArray(labelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, labelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_CULL_FACE);
}

void initQRCode(const std::string &imageName)
{
    ShaderInfo qrshaders[] = {
        { GL_VERTEX_SHADER, "qrcode.vert" },
        { GL_FRAGMENT_SHADER, "qrcode.frag" },
        { GL_NONE, NULL }
    };

    qrShaderProgram = loadShaders(qrshaders);
    if (qrShaderProgram != 0)//加载成功
    {
        //我们这里不再单独使用单独的着色器，所以，直接删除
        for (ShaderInfo *entry = qrshaders; entry->type != GL_NONE; ++entry) {
            glDeleteShader(entry->shader);
            entry->shader = 0;
        }
    }
    glUseProgram(qrShaderProgram);

    // Set up vertex data (and buffer(s)) and attribute pointers
    GLfloat vertices[] = {
        // Positions          // Texture Coords
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f, // Top Right
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // Bottom Right
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // Bottom Left
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f  // Top Left 
    };

    GLuint indices[] = {  // Note that we start from 0!
        0, 1, 3, // First Triangle
        1, 2, 3  // Second Triangle
    };

    glGenVertexArrays(1, &qrVAO);
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &qrEBO);

    glBindVertexArray(qrVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qrEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Load and create a texture
    glGenTextures(1, &qrTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, qrTexture);
    glUniform1i(glGetUniformLocation(qrShaderProgram, "qrTexture"), 0);

    // Set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT (usually basic wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Load image, create texture and generate mipmaps
    int width, height;
    unsigned char* image = SOIL_load_image(imageName.c_str(), &width, &height, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.
    glUseProgram(0);
}


void renderText(const GLuint shaderProgram, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(labelVAO);
    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos, ypos + h, 0.0, 0.0 },
            { xpos, ypos, 0.0, 1.0 },
            { xpos + w, ypos, 1.0, 1.0 },

            { xpos, ypos + h, 0.0, 0.0 },
            { xpos + w, ypos, 1.0, 1.0 },
            { xpos + w, ypos + h, 1.0, 0.0 }
        };
        // Render glyph texture over quad
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, labelVBO);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qrEBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}


void drawQRCode(GLuint shaderProgram)
{
    glUseProgram(shaderProgram);
    //Create transformation
    glm::mat4 view;
    glm::mat4 projection;
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    projection = glm::perspective(glm::radians(45.0f), (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
    // Get their uniform location
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    //Pass them to the shaders
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(qrVAO);
    //bind Texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, qrTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qrEBO);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}


/**
* @brief 初始化程序
*/
void init(const std::string &imageName, const std::string &outVideoName)
{
    initText();
    initQRCode(imageName);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    bool r = outputVideo.open(outVideoName, CV_FOURCC('M', 'J', 'P', 'G'), 20.0f, cv::Size(WIDTH, HEIGHT), true);
    if (r == false)
    {
        std::cerr << "Video open error!" << std::endl;
        exit(0);
    }
}

/**
 * @brief 绘制刷新
 */
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawQRCode(qrShaderProgram);
    renderText(labelShaderProgram, labelContent, 25.0f, 25.0f, 0.5f, glm::vec3(0.5, 0.8f, 0.2f));
    glFlush();
}

bool getNextXYZ(int & outx, int & outy, int & outz)
{
    const int minx = -60, maxx = 60, deltax = 5;
    const int miny = -60, maxy = 60, deltay = 5;
    const int minz = 0, maxz = 359, deltaz = 5;
    static int x = minx;
    static int y = miny;
    static int z = minz;
    if (z > maxz)
    {
        y = y + deltay;
        z = minz;
    }
    if (y > maxy)
    {
        x = x + deltax;
        y = miny;
    }
    if (x > maxx)
    {
        return false;
    }
    outx = x;
    outy = y;
    outz = z;
    z = z + deltaz;
    return true;
}


void timerProc(int id)
{
    int x, y, z;
    if (getNextXYZ(x, y, z))
    {
        glm::mat4 model;//默认为4*4的单位矩阵
        model = glm::rotate(model, glm::radians(static_cast<float>(x)), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(static_cast<float>(y)), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(static_cast<float>(z)), glm::vec3(0.0f, 0.0f, 1.0f));
        // Get matrix's uniform location and set matrix
        glUseProgram(qrShaderProgram);
        GLint modelLoc = glGetUniformLocation(qrShaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUseProgram(0);

        std::stringstream content;
        content << "Angle: x=" << x << ", y=" << y <<", z=" << z;
        labelContent = content.str();
        glutPostRedisplay();
        cv::Mat img(HEIGHT, WIDTH, CV_8UC3);
        //use fast 4-byte alignment (default anyway) if possible
        glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);
        //set length of one complete row in destination data (doesn't need to equal img.cols)
        glPixelStorei(GL_PACK_ROW_LENGTH, img.step / img.elemSize());
        glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
        cv::flip(img, img, 0);
        outputVideo << img;
        glutTimerFunc(FRAME_RATE, timerProc, id + 1);//需要在函数中再调用一次，才能保证循环
    }
    else
    {
        //终止
        outputVideo.release();
        std::cout << "successful!" << std::endl;
        exit(0);
    }
}




/**
 * @brief 使用opengl绘制一个三角形
 */
int main(int argc,char *argv[])
{
    if (argc != 3)
    {
        std::cout << "Need argument!" << std::endl;
        std::cout << "TestQRCodeMain <qrImageName> <outVideoName>" << std::endl;
        return 0;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitContextVersion(4, 2);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutCreateWindow("Sample.exe");
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Unable to initialize GlEW ... exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
    glViewport(0, 0, WIDTH, HEIGHT);

    init(argv[1],argv[2]);

    glutDisplayFunc(display);
    glutTimerFunc(FRAME_RATE, timerProc, 1);
    
    glutMainLoop();
    return 0;
}


