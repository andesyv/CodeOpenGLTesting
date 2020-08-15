#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>
#include <algorithm> // std::find_if

class Shader
{
public:
    typedef typename std::vector<std::pair<std::string, std::string>> envVarsT;

    template <typename T>
    static bool caseInsensitiveCompare(const T& lhs, const T& rhs) {
        if (lhs.size() != rhs.size())
            return false;

        for (auto ilhs{lhs.begin()}, irhs{rhs.begin()}; ilhs != lhs.end(); ++ilhs, ++irhs) {
            if (std::toupper(*ilhs) != std::toupper(*irhs))
                return false;
        }

        return true;
    }

private:
    bool bValid = false;
    int program;

public:
    Shader(const std::string& vPath, const std::string& fPath, envVarsT environmentVariables = {})
    {
        std::string vertexSource{}, fragmentSource{};
        // std::ifstream input{vPath, std::ifstream::in | std::ifstream::ate};
        if (!appendFile(vertexSource, vPath, environmentVariables))
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
        if (!appendFile(fragmentSource, fPath, environmentVariables))
        {
            std::cout << "SHADER ERROR: Fragment path not found." << std::endl;
            return;
        }
        
        auto fSourcePtr = fragmentSource.c_str();
        
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

        std::cout << "SHADERINFO: Shader with " << vPath.substr(vPath.find_last_of('/') + 1)
            << " and " << fPath.substr(fPath.find_last_of('/') + 1) << " created with program id: "
            << program << std::endl;
    }

    int get() const { return program; }
    int operator* () const { return get(); }

    bool appendFile(std::string& str, std::string_view filename, const envVarsT& envVars)
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
                if (0 < path.size() && appendFile(str, path, envVars))
                    continue;
                else
                    return false;
            }

            // environment variable substitution
            if (!envVars.empty()) {
                // Loop through all substitutions in line
                for (std::size_t pos{line.find_first_of('$')}; pos != line.npos; pos = line.find_first_of('$', pos + 1)) {
                    auto substStart{line.begin() + pos};
                    auto substEnd = std::find_if(substStart + 1, line.end(), [](const char& c){
                        return c < '0' || ('9' < c && c < 'A') || ('Z' < c && c < 'a') || 'z' < c;
                    });

                    // if the substitution string couldn't be found, just skip it
                    if (substEnd == line.end() || substEnd - substStart <= 0) {
                        continue;
                    }

                    auto subst{std::string_view{line}.substr(pos + 1, substEnd - substStart - 1)};
                    for (const auto& var : envVars) {
                        if (caseInsensitiveCompare(std::string_view{var.first}, subst)) {
                            line.replace(substStart, substEnd, var.second.begin(), var.second.end());
                            break;
                        }
                    }
                }
            }

            str.append(line).append(1, '\n');
        }

        return true;
    }
};

#endif // SHADER_H