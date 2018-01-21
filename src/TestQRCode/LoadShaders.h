/**
* @file LoadShaders.h
* @brief opengl�����ڼ�����ɫ������
*/
#ifndef __LOAD_SHADERS_H__
#define __LOAD_SHADERS_H__

#include <gl/GL.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


    /**
    * @brief ��ɫ��������Ϣ
    */
    typedef struct {
        GLenum       type;      ///< ��ɫ�����ͣ�����GL_VERTEX_SHADER����GL_FRAGMENT_SHADER�ȡ�������һ����ӦΪGL_NONE
        const char*  filename;  ///< ����ɫ����Ӧ���ļ����ƣ�����"triangles.vert"����"triangles.frag"�ȣ���������һ����ӦΪNULL
        GLuint       shader;    ///< ����ɫ�����غ��Ӧ��ID
    } ShaderInfo;

    /**
    * @brief ���ز����롢������ɫ������
    * @detail ����һ����ɫ���б�ÿ��Ԫ�ض�Ӧһ����ɫ�����ֱ������ɫ�����͡���ӦGLSL��Դ���ļ����ƣ��ı��ļ���
    * @param[in,out] ��Ҫ���ص���ɫ���Ļ�����Ϣ
    * @return ����ɹ���������ɫ�������ID(��glCreateProgram()���ص���ͬ)�����ʧ�ܣ�����0
    */
    GLuint loadShaders(ShaderInfo*);


#ifdef __cplusplus
};
#endif // __cplusplus

#endif // __LOAD_SHADERS_H__