#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>

class Shader
{
private:
    bool bValid = false;
    int program;

public:
    Shader(const std::string& vPath, const std::string& fPath)
    {
        std::ifstream input{vPath, std::ifstream::in | std::ifstream::ate};
        if (!input)
        {
            std::cout << "SHADER ERROR: Vertex path not found." << std::endl;
            return;
        }

        std::string vertexSource{}, fragmentSource{};
        vertexSource.reserve(input.tellg());
        input.seekg(0, input.beg);
        vertexSource.insert(vertexSource.begin(), std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
        auto vSourcePtr = vertexSource.c_str();
        input.close();

        input.open(fPath, std::ifstream::in | std::ifstream::ate);
        if (!input)
        {
            std::cout << "SHADER ERROR: Fragment path not found." << std::endl;
            return;
        }

        fragmentSource.reserve(input.tellg());
        input.seekg(0, input.beg);
        fragmentSource.insert(fragmentSource.begin(), std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
        auto fSourcePtr = fragmentSource.c_str();
        input.close();
        
        // build and compile our shader program
        // ------------------------------------
        // vertex shader
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vSourcePtr, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                      << infoLog << std::endl;
            return;
        }
        // fragment shader
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fSourcePtr, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                      << infoLog << std::endl;
            return;
        }
        // link shaders
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        // check for linking errors
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                      << infoLog << std::endl;
            return;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        bValid = true;
    }

    int get() const { return program; }
    int operator* () const { return get(); }
};