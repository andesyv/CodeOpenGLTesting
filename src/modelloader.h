#ifndef MODELLOADER_H
#define MODELLOADER_H

#include "json.hpp" // https://github.com/nlohmann/json
#include "components.h"
#include <fstream>
#include <iostream>
#include <functional>
#include <string_view>
#include <algorithm>
#include <glad/glad.h>
#include <filesystem>

class ModelLoader
{
private:
    std::vector<std::pair<std::string, component::mesh>> mLoadedModels;
    ModelLoader() = default;
    ModelLoader(const ModelLoader&) = delete;
    ModelLoader(ModelLoader&&) = delete;

public:    
    typedef std::pair<std::vector<vertex>, std::vector<unsigned>> container;

private:
    template <typename T>
    static void iterateIndex(std::vector<std::vector<std::byte>> &cachedBuffers, nlohmann::basic_json<>::value_type &bufferView, unsigned int pointCount, container &result)
    {
        auto begin = reinterpret_cast<T *>(&cachedBuffers.at(bufferView["buffer"].get<int>()).at(bufferView["byteOffset"].get<int>()));
        for (unsigned int i{0}; i < pointCount; ++i)
            result.second[i] = *(begin + i); // begin[i]
    }

    static std::string_view getcwd(std::string_view cwd) {
        if (cwd.find_last_of('/') != cwd.npos)
            cwd = cwd.substr(0, cwd.find_last_of('/') + 1);
        else if (cwd.find_last_of('\\') != cwd.npos)
            cwd = cwd.substr(0, cwd.find_last_of('\\') + 1);
        else
            cwd = {};

        return cwd;
    }

    static std::string_view getname(std::string_view file) { return getname(std::move(file), getcwd(file)); }
    static std::string_view getname(std::string_view file, std::string_view cwd) {
        auto cwdL = cwd.size();
        return file.substr(cwdL, file.find_last_of('.') - cwdL);
    }

    // Cheaty reflection
    template<typename T>
    static std::string getTypeName() { return "NULL"; }
    template<> static std::string getTypeName<glm::vec2>() { return "VEC2"; }
    template<> static std::string getTypeName<glm::vec3>() { return "VEC3"; }
    template<> static std::string getTypeName<glm::vec4>() { return "VEC4"; }
    template<> static std::string getTypeName<int>() { return "SCALAR"; }

    // Other hacky(ish) reflection
    template <typename T> static std::size_t getCount() = delete;
    template <> static std::size_t getCount<glm::vec1>() { return 1; };
    template <> static std::size_t getCount<glm::vec2>() { return 2; };
    template <> static std::size_t getCount<glm::vec3>() { return 3; };
    template <> static std::size_t getCount<glm::vec4>() { return 4; };
    
    auto& initObj(const std::string& file) {
        auto obj = load(file);
        auto& mesh = mLoadedModels.emplace_back(file, component::mesh{}).second;
        // Do OpenGL stuff

        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);
        glGenBuffers(1, &mesh.IBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(mesh.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, obj.first.size() * sizeof(vertex), obj.first.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.second.size() * sizeof(unsigned int), obj.second.data(), GL_STATIC_DRAW);
        // GLint bufferSize;
        // glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
        // std::cout << "buffer size: " << bufferSize << std::endl;

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        mesh.vertexCount = static_cast<unsigned int>(obj.first.size());
        mesh.bIndices = true;
        // std::tuple_size to get length of array. Surprisingly that it doesn't already exist in std::array
        // https://stackoverflow.com/questions/21936507/why-isnt-stdarraysize-static
        mesh.indexCount = static_cast<unsigned int>(obj.second.size());

        return mesh;
    }

public:

    static std::vector<std::byte> readFileB(const std::string& file) {
        std::basic_ifstream<std::byte> ifs{file, std::ifstream::in | std::ifstream::binary | std::ifstream::ate};
        if (!ifs) {
            std::cout << "ModelLoader could'nt open the specified file: " << file << std::endl;
            return {};
        }

        std::vector<std::byte> str{};
        const auto size = ifs.tellg();
        ifs.seekg(0);
        str.resize(size);
        ifs.read(str.data(), size);

        return str;
    }

    static std::string readFile(const std::string& file) {
        std::ifstream ifs{file, std::ifstream::in};
        if (!ifs)
        {
            std::cout << "ModelLoader could'nt open the specified file: " << file << std::endl;
            return {};
        }

        std::string str{};
        str.insert(str.begin(), std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{});
        return str;
    }

    static container load(const std::string& file) {
        auto cwd = getcwd(file);
        
        auto gltf = nlohmann::json::parse(readFile(file));


        container result{};
        // Read all buffers to memory
        std::vector<std::vector<std::byte>> cachedBuffers{};
        for (const auto& buffer : gltf["buffers"]) {
            auto content = readFileB(std::string{cwd}.append(buffer["uri"].get<std::string>()));
            if (content.empty()) {
                std::cout << "Buffer " << buffer["uri"].get<std::string>() << " is empty. Aborting." << std::endl; 
                return result;
            }

            cachedBuffers.push_back(std::move(content));
        }
    
        auto bvs = gltf["bufferViews"];
        auto accessors = gltf["accessors"];
        for (const auto& scene : gltf["scenes"]) {
            for (const auto& nodeIndex : scene["nodes"]) {
                auto nodeInfo = gltf["nodes"][nodeIndex.get<int>()];
                auto meshObj = gltf["meshes"][nodeInfo["mesh"].get<int>()];
                for (const auto& primitive : meshObj["primitives"]) {
                    // Vertices:
                    auto attr = primitive["attributes"];
                    auto readAttr = [&]<typename T>(const std::string& attrName, std::function<void(unsigned int, T*)> func) {
                        // auto buffer = gltf["buffers"][bv["buffer"].get<int>()];
                        auto acc = accessors[attr[attrName].get<int>()];
                        auto bv = bvs[acc["bufferView"].get<int>()];
                        const auto pointCount = acc["count"].get<int>();

                        // Reserve memory
                        if (result.first.size() < pointCount)
                            result.first.resize(pointCount);

                        if (acc["type"].get<std::string>() != getTypeName<T>() || acc["componentType"].get<int>() != GL_FLOAT)
                        {
                            std::cout << "Vertex error: Unhandled format." << std::endl;
                            return false;
                        }
                        auto begin = reinterpret_cast<T *>(&cachedBuffers.at(bv["buffer"].get<int>()).at(bv["byteOffset"].get<int>()));
                        for (unsigned int i{0}; i < pointCount; ++i)
                            func(i, begin);
                            // result.first[i].pos = *(begin + i); // begin[i]

                        return true;
                    };


                    if (attr.contains("POSITION")) {
                        if (!readAttr("POSITION", std::function<void(unsigned int, glm::vec3*)> { [&](unsigned int i, glm::vec3 *begin) {
                                    result.first[i].pos = *(begin + i); // begin[i]
                        }}))
                            continue;
                    }

                    if (attr.contains("NORMAL")) {
                        if (!readAttr("NORMAL", std::function<void(unsigned int, glm::vec3*)> { [&](unsigned int i, glm::vec3 *begin) {
                                    result.first[i].normal = *(begin + i);
                        }}))
                            continue;
                    }

                    if (attr.contains("TEXCOORD_0")) {
                        if (!readAttr("TEXCOORD_0", std::function<void(unsigned int, glm::vec2*)> { [&](unsigned int i, glm::vec2 *begin) {
                                    result.first[i].uv = *(begin + i);
                        }}))
                            continue;
                    }



                    // Indices:
                    // auto buffer = gltf["buffers"][bv["buffer"].get<int>()];
                    auto acc = accessors[primitive["indices"].get<int>()];
                    auto bv = bvs[acc["bufferView"].get<int>()];
                    const auto pointCount = acc["count"].get<int>();

                    // Reserve memory
                    result.second.resize(pointCount);

                    if (acc["type"].get<std::string>() != getTypeName<int>())
                    {
                        std::cout << "Index error: Unhandled format." << std::endl;
                        continue;
                    }

                    switch (acc["componentType"].get<int>()) {
                        case GL_SHORT:
                            iterateIndex<GLshort>(cachedBuffers, bv, pointCount, result);
                            break;
                        case GL_UNSIGNED_SHORT:
                            iterateIndex<GLushort>(cachedBuffers, bv, pointCount, result);
                            break;
                        case GL_INT:
                            iterateIndex<GLint>(cachedBuffers, bv, pointCount, result);
                            break;
                        case GL_UNSIGNED_INT:
                            iterateIndex<GLuint>(cachedBuffers, bv, pointCount, result);
                            break;
                        default:
                            std::cout << "Index error: Unhandled format." << std::endl;
                            continue;
                    }
                }

                // Implement multiple mesh import in future
                break;
            }
        }

        return result;
    }

    template <typename T, unsigned int I>
    static std::streampos writeVertexToBuffer(nlohmann::json& outObj, std::ofstream& ofs, const std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec2>> &vBuffer)
    {
        unsigned int currentBufferPos = ofs.tellp();
        const auto iCount = vBuffer.size();
        const auto iSize = iCount * sizeof(T);

        std::vector<T> buffer{};
        buffer.reserve(iCount);
        for (auto it = vBuffer.begin(); it != vBuffer.end(); ++it)
            buffer.push_back(std::get<I>(*it));

        ofs.write(reinterpret_cast<const char *>(buffer.data()), iSize);
        outObj["accessors"].push_back({
            {"bufferView", I + 1},
            {"componentType", GL_FLOAT},
            {"count", iCount},
            {"max", [&]() {
                nlohmann::json arr{};
                for (unsigned int i{0}; i < T::length(); ++i)
                    arr.push_back((*std::max_element(buffer.begin(), buffer.end(), [i](const T &a, const T &b) {
                        return a[i] < b[i];
                    }))[i]);
                return arr;
            }()},
            {"min", [&]() {
                nlohmann::json arr{};
                for (unsigned int i{0}; i < T::length(); ++i)
                    arr.push_back((*std::min_element(buffer.begin(), buffer.end(), [i](const T &a, const T &b) {
                        return a[i] < b[i];
                    }))[i]);
                return arr;
            }()},
            {"type", getTypeName<T>()}
        });
        outObj["bufferViews"].push_back({
            {"buffer", 0},
            {"byteLength", iSize},
            {"byteOffset", currentBufferPos}
        });

        return ofs.tellp();
    };

    static bool save(const container& object, const std::string& file) {
        /// json.hpp got nice looking syntaxes.
        nlohmann::json outObj{{
            "asset", {
                {"generator", "Custom glTF exporter"},
                {"version", "1.0"}
            }
        }};

        const auto cwd = getcwd(file);
        const auto name = getname(file, cwd);
        
        outObj["meshes"].push_back({
            {"name", std::string{name}},
            {"primitives", {
                {
                    {"attributes", {
                        {"POSITION", 1},
                        {"NORMAL", 2},
                        {"TEXCOORD_0", 3}
                    }},
                    {"indices", 0}
                }
            }}
        });

        outObj["nodes"].push_back({
            {"mesh", 0},
            {"name", std::string{name}}
        });
        outObj["scenes"].push_back({
            {"nodes", {
                0
            }}
        });
        outObj["scene"] = 0;

        const auto bufferName = std::string{name} + ".bin";
        const auto bufferFile = std::string{cwd} + bufferName;

        outObj["buffers"].push_back({
            {"uri", bufferName}
        });

        std::ofstream ofs{bufferFile, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary};
        if (!ofs) {
            std::cout << "Failed to open file " << bufferFile << " for writing." << std::endl;
            return false;
        }

        unsigned int currentBufferPos{0};

        // std::vector<std::byte> tempBuffer{};
        const auto iCount = object.second.size();
        const auto iSize = iCount * sizeof(unsigned int);
        // tempBuffer.resize(iCount * sizeof(unsigned));
        ofs.write(reinterpret_cast<const char*>(object.second.data()), iSize);

        // Write indices
        outObj["accessors"].push_back({
            {"bufferView", 0},
            {"componentType", GL_UNSIGNED_INT},
            {"count", iCount},
            {"max", {*std::max_element(object.second.begin(), object.second.end())}},
            {"min", {*std::min_element(object.second.begin(), object.second.end())}},
            {"type", "SCALAR"}
        });
        outObj["bufferViews"].push_back({
            {"buffer", 0},
            {"byteLength", iSize},
            {"byteOffset", currentBufferPos}
        });
        currentBufferPos = ofs.tellp();

        std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec2>> tupleBuffer{object.first.begin(), object.first.end()};
        // std::transform(object.first.begin(), object.first.end(), tupleBuffer.begin(), [](const vertex& v) { return static_cast<}
        // It's posible to find some fancy workaround for this too, but in this case it was both easier and less code by just hardcoding.
        currentBufferPos = writeVertexToBuffer<glm::vec3, 0>(outObj, ofs, tupleBuffer);
        currentBufferPos = writeVertexToBuffer<glm::vec3, 1>(outObj, ofs, tupleBuffer);
        currentBufferPos = writeVertexToBuffer<glm::vec2, 2>(outObj, ofs, tupleBuffer);

        // Done with binary data writing.
        ofs.close();

        outObj["buffers"][0]["byteLength"] = currentBufferPos;
        
        ofs.open(file,std::ofstream::out | std::ofstream::trunc);
        if (!ofs) {
            std::cout << "Failed to open file " << file << " for writing." << std::endl;
            return false;
        }

        ofs << outObj.dump(4);
        return true;
    }

    // Singleton interface
    static ModelLoader& get() {
        static ModelLoader instance{};
        return instance;
    }

    component::mesh mesh(const std::string& file) {
        for (const auto& mesh : mLoadedModels)
            return (mesh.first == file) ? mesh.second : initObj(file);
    }

    // Initializes all models in path
    void initModels(std::filesystem::path path) {
        for (const auto& p : path) {
            if (p.extension().string() == ".gltf") {
                bool bAlreadyLoaded{false};
                for (const auto& m : mLoadedModels)
                    if (std::filesystem::path{m.first} == p) {
                        bAlreadyLoaded = true;
                        break;
                    }
                
                if (bAlreadyLoaded)
                    continue;

                initObj(p.string());
            }
        }
    }

    /**
     * Don't really need to ever call this function as by this point all
     * VAOs should already be deleted and the data stored here should be
     * garbage values.
     */
    void deInitModels() {
        for (auto& m : mLoadedModels) {
            auto& mesh = m.second;
            if (mesh.bIndices)
                glDeleteBuffers(1, &mesh.IBO);

            glDeleteBuffers(1, &mesh.VBO);
            glDeleteVertexArrays(1, &mesh.VAO);
        }
    }
};

#endif