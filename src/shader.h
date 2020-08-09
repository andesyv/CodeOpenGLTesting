#ifndef SHADER_H
#define SHADER_H

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
        std::string vertexSource{}, fragmentSource{};
        // std::ifstream input{vPath, std::ifstream::in | std::ifstream::ate};
        if (!appendFile(vertexSource, vPath))
        {
            std::cout << "SHADER ERROR: Vertex path not found." << std::endl;
            return;
        }

        // vertexSource.reserve(input.tellg());
        // input.seekg(0, input.beg);
        // std::string line;
        // while (std::getline(input, line)) {
        //     if (line.starts_with("#include")) {
        //         // std::cout << "skipping: " << line << std::endl;
        //         continue;
        //     }
        //     vertexSource.append(line).append(1, '\n');
        // }
        // // vertexSource.insert(vertexSource.begin(), std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
        auto vSourcePtr = vertexSource.c_str();
        // input.close();

        // input.open(fPath, std::ifstream::in | std::ifstream::ate);
        if (!appendFile(fragmentSource, fPath))
        {
            std::cout << "SHADER ERROR: Fragment path not found." << std::endl;
            return;
        }

        // fragmentSource.reserve(input.tellg());
        // input.seekg(0, input.beg);
        // while (std::getline(input, line, '\n'))
        // {
        //     fragmentSource.append(line).append(1, '\n');
        // }
        // // fragmentSource.insert(fragmentSource.begin(), std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
        auto fSourcePtr = fragmentSource.c_str();
        // input.close();
        
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

    bool appendFile(std::string& str, std::string_view filename)
    {
        std::ifstream ifs{std::string{filename}, std::ifstream::in | std::ifstream::ate};
        if (!ifs)
        {
            return false;
        }

        // Reserve more space for string
        str.reserve(str.size() + ifs.tellg());
        ifs.seekg(0, ifs.beg);
        std::string line;
        while (std::getline(ifs, line))
        {
            // Recursive reading of file
            if (line.starts_with("#include"))
            {
                std::string_view path{line};
                const auto offset{path.find_first_of('"') + 1};
                path = path.substr(offset, path.find_last_of('"') - offset);
                if (0 < path.size() && appendFile(str, path))
                    continue;
                else
                    return false;
            }
            str.append(line).append(1, '\n');
        }

        return true;
    }
};

#endif // SHADER_H