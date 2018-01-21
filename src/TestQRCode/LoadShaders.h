/**
* @file LoadShaders.h
* @brief opengl中用于加载着色器程序
*/
#ifndef __LOAD_SHADERS_H__
#define __LOAD_SHADERS_H__

#include <gl/GL.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


    /**
    * @brief 着色器基本信息
    */
    typedef struct {
        GLenum       type;      ///< 着色器类型，例如GL_VERTEX_SHADER或者GL_FRAGMENT_SHADER等。如果最后一个，应为GL_NONE
        const char*  filename;  ///< 该着色器对应的文件名称，例如"triangles.vert"或者"triangles.frag"等，如果是最后一个，应为NULL
        GLuint       shader;    ///< 该着色器加载后对应的ID
    } ShaderInfo;

    /**
    * @brief 加载并编译、链接着色器程序
    * @detail 接受一个着色器列表，每个元素对应一个着色器，分别包含着色器类型、对应GLSL的源码文件名称（文本文件）
    * @param[in,out] 需要加载的着色器的基本信息
    * @return 如果成功，返回着色器程序的ID(如glCreateProgram()返回的相同)；如果失败，返回0
    */
    GLuint loadShaders(ShaderInfo*);


#ifdef __cplusplus
};
#endif // __cplusplus

#endif // __LOAD_SHADERS_H__