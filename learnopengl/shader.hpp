#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID;

    Shader(const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr,
            const int varyingsCount = 0, const char **varyings = nullptr, const char *tessContPath = nullptr,const char *tessEvalPath = nullptr) {
        std::string vertexCode;
        std::string fragmentCode;
        std::string tessContCode;
        std::string tessEvalCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream tContShaderFile;
        std::ifstream tEvalShaderFile;
        std::ifstream gShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        tContShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        tEvalShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            vShaderFile.close();
            fShaderFile.close();
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();

            if(tessContPath != nullptr) {
                tContShaderFile.open(tessContPath);
                std::stringstream tShaderStream;
                tShaderStream << tContShaderFile.rdbuf();
                tContShaderFile.close();
                tessContCode = tShaderStream.str();
            }
            if(tessEvalPath != nullptr) {
                tEvalShaderFile.open(tessEvalPath);
                std::stringstream tShaderStream;
                tShaderStream << tEvalShaderFile.rdbuf();
                tEvalShaderFile.close();
                tessEvalCode = tShaderStream.str();
            }
            if(geometryPath != nullptr) {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }

        }
        catch (std::ifstream::failure e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
        }

        const char *vShaderCode = vertexCode.c_str();
        const char *fShaderCode = fragmentCode.c_str();
        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileError(vertex, "VERTEX");
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileError(fragment, "FRAGMENT");
        unsigned int tess_cont;
        if(tessContPath != nullptr) {
            const char *tShaderCode = tessContCode.c_str();
            tess_cont = glCreateShader(GL_TESS_CONTROL_SHADER);
            glShaderSource(tess_cont, 1, &tShaderCode, NULL);
            glCompileShader(tess_cont);
            checkCompileError(tess_cont, "TESSELATION CONTROL");
        }
        unsigned int tess_eval;
        if(tessEvalPath != nullptr) {
            const char *tShaderCode = tessEvalCode.c_str();
            tess_eval = glCreateShader(GL_TESS_EVALUATION_SHADER);
            glShaderSource(tess_eval, 1, &tShaderCode, NULL);
            glCompileShader(tess_eval);
            checkCompileError(tess_eval, "TESSELATION EVALUATE");
        }
        unsigned int geometry;
        if(geometryPath != nullptr) {
            const char *gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileError(geometry, "GEOMETRY");
        }

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if(tessContPath != nullptr)
            glAttachShader(ID, tess_cont);
        if(tessEvalPath != nullptr)
            glAttachShader(ID, tess_eval);
        if(geometryPath != nullptr)
            glAttachShader(ID, geometry);
        if(varyingsCount > 0)
            glTransformFeedbackVaryings(ID, varyingsCount, varyings, GL_INTERLEAVED_ATTRIBS);
        glLinkProgram(ID);
        checkCompileError(ID, "PROGRAM");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if(tessContPath != nullptr)
            glDeleteShader(tess_cont);
        if(tessEvalPath != nullptr)
            glDeleteShader(tess_eval);
        if(geometryPath != nullptr)
            glDeleteShader(geometry);
    }

    Shader(const std::string &vertexSource, const std::string &fragmentSource) {
        const char *vShaderCode = vertexSource.c_str();
        const char *fShaderCode = fragmentSource.c_str();
        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileError(vertex, "VERTEX");
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileError(fragment, "FRAGMENT");
        
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileError(ID, "PROGRAM");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    void use() {
        glUseProgram(ID);
    }

    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string &name, const glm::vec2 &value) const {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec2(const std::string &name, float x, float y) const {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    void setVec3(const std::string &name, const glm::vec3 &value) const { 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const { 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }
    void setVec4(const std::string &name, const glm::vec4 &value) const { 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) { 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
private:
    void checkCompileError(GLuint shader, std::string type) {
        GLint success;
        GLchar infoLog[1024];
        if(type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if(!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};