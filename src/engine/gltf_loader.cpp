#include "gltf_loader.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <nlohmann/json.hpp>

#include "../../vendor/tinygltf/tiny_gltf.h"
#include "../vendor_include.h"

namespace {

struct LoadedTexture {
    bool valid = false;
    raylib::Texture2D texture{};
};

struct MeshBuffers {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texcoords;
    std::vector<unsigned short> indices;
};

inline raylib::Matrix to_matrix(const tinygltf::Node& node) {
    if (node.matrix.size() == 16) {
        raylib::Matrix m{};
        m.m0 = static_cast<float>(node.matrix[0]);
        m.m1 = static_cast<float>(node.matrix[1]);
        m.m2 = static_cast<float>(node.matrix[2]);
        m.m3 = static_cast<float>(node.matrix[3]);
        m.m4 = static_cast<float>(node.matrix[4]);
        m.m5 = static_cast<float>(node.matrix[5]);
        m.m6 = static_cast<float>(node.matrix[6]);
        m.m7 = static_cast<float>(node.matrix[7]);
        m.m8 = static_cast<float>(node.matrix[8]);
        m.m9 = static_cast<float>(node.matrix[9]);
        m.m10 = static_cast<float>(node.matrix[10]);
        m.m11 = static_cast<float>(node.matrix[11]);
        m.m12 = static_cast<float>(node.matrix[12]);
        m.m13 = static_cast<float>(node.matrix[13]);
        m.m14 = static_cast<float>(node.matrix[14]);
        m.m15 = static_cast<float>(node.matrix[15]);
        return m;
    }

    raylib::Matrix translation = raylib::MatrixIdentity();
    if (node.translation.size() == 3) {
        translation =
            raylib::MatrixTranslate(static_cast<float>(node.translation[0]),
                                    static_cast<float>(node.translation[1]),
                                    static_cast<float>(node.translation[2]));
    }

    raylib::Matrix rotation = raylib::MatrixIdentity();
    if (node.rotation.size() == 4) {
        raylib::Quaternion q{
            static_cast<float>(node.rotation[0]),
            static_cast<float>(node.rotation[1]),
            static_cast<float>(node.rotation[2]),
            static_cast<float>(node.rotation[3]),
        };
        rotation = raylib::QuaternionToMatrix(q);
    }

    raylib::Matrix scale = raylib::MatrixIdentity();
    if (node.scale.size() == 3) {
        scale = raylib::MatrixScale(static_cast<float>(node.scale[0]),
                                    static_cast<float>(node.scale[1]),
                                    static_cast<float>(node.scale[2]));
    }

    return raylib::MatrixMultiply(translation,
                                  raylib::MatrixMultiply(rotation, scale));
}

template<typename T>
const T* accessor_data(const tinygltf::Model& model,
                       const tinygltf::Accessor& accessor) {
    const tinygltf::BufferView& view =
        model.bufferViews[static_cast<size_t>(accessor.bufferView)];
    const tinygltf::Buffer& buffer =
        model.buffers[static_cast<size_t>(view.buffer)];
    size_t offset = static_cast<size_t>(accessor.byteOffset) +
                    static_cast<size_t>(view.byteOffset);
    return reinterpret_cast<const T*>(buffer.data.data() + offset);
}

bool fill_mesh_buffers(const tinygltf::Model& model,
                       const tinygltf::Primitive& primitive,
                       const raylib::Matrix& transform, MeshBuffers& out,
                       std::string& err_out) {
    if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
        err_out += "Non-triangle primitive encountered\n";
        return false;
    }

    auto pos_it = primitive.attributes.find("POSITION");
    if (pos_it == primitive.attributes.end()) {
        err_out += "Primitive missing POSITION\n";
        return false;
    }

    const tinygltf::Accessor& pos_acc =
        model.accessors[static_cast<size_t>(pos_it->second)];
    if (pos_acc.type != TINYGLTF_TYPE_VEC3 ||
        pos_acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        err_out += "Unsupported position format\n";
        return false;
    }

    size_t vertex_count = static_cast<size_t>(pos_acc.count);
    out.vertices.reserve(vertex_count * 3);
    out.normals.reserve(vertex_count * 3);
    out.texcoords.reserve(vertex_count * 2);

    const float* pos_data = accessor_data<float>(model, pos_acc);

    const float* norm_data = nullptr;
    auto norm_it = primitive.attributes.find("NORMAL");
    bool has_normals = norm_it != primitive.attributes.end();
    tinygltf::Accessor norm_acc;
    if (has_normals) {
        norm_acc = model.accessors[static_cast<size_t>(norm_it->second)];
        if (norm_acc.type == TINYGLTF_TYPE_VEC3 &&
            norm_acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            norm_data = accessor_data<float>(model, norm_acc);
        } else {
            has_normals = false;
        }
    }

    const float* uv_data = nullptr;
    auto uv_it = primitive.attributes.find("TEXCOORD_0");
    bool has_uvs = uv_it != primitive.attributes.end();
    tinygltf::Accessor uv_acc;
    if (has_uvs) {
        uv_acc = model.accessors[static_cast<size_t>(uv_it->second)];
        if (uv_acc.type == TINYGLTF_TYPE_VEC2 &&
            uv_acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            uv_data = accessor_data<float>(model, uv_acc);
        } else {
            has_uvs = false;
        }
    }

    for (size_t i = 0; i < vertex_count; ++i) {
        raylib::Vector3 p{pos_data[i * 3 + 0], pos_data[i * 3 + 1],
                          pos_data[i * 3 + 2]};
        p = raylib::Vector3Transform(p, transform);
        out.vertices.push_back(p.x);
        out.vertices.push_back(p.y);
        out.vertices.push_back(p.z);

        if (has_normals && norm_data != nullptr) {
            raylib::Vector3 n{norm_data[i * 3 + 0], norm_data[i * 3 + 1],
                              norm_data[i * 3 + 2]};
            n = raylib::Vector3Normalize(n);
            out.normals.push_back(n.x);
            out.normals.push_back(n.y);
            out.normals.push_back(n.z);
        } else {
            out.normals.push_back(0.f);
            out.normals.push_back(1.f);
            out.normals.push_back(0.f);
        }

        if (has_uvs && uv_data != nullptr) {
            out.texcoords.push_back(uv_data[i * 2 + 0]);
            out.texcoords.push_back(uv_data[i * 2 + 1]);
        } else {
            out.texcoords.push_back(0.f);
            out.texcoords.push_back(0.f);
        }
    }

    // Indices
    if (primitive.indices < 0) {
        // Create a sequential index buffer
        out.indices.reserve(vertex_count);
        for (unsigned int i = 0; i < vertex_count; ++i) {
            out.indices.push_back(static_cast<unsigned short>(i));
        }
    } else {
        const tinygltf::Accessor& idx_acc =
            model.accessors[static_cast<size_t>(primitive.indices)];
        size_t index_count = static_cast<size_t>(idx_acc.count);
        out.indices.reserve(index_count);

        switch (idx_acc.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                const unsigned char* data =
                    accessor_data<unsigned char>(model, idx_acc);
                for (size_t i = 0; i < index_count; ++i) {
                    out.indices.push_back(static_cast<unsigned short>(data[i]));
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                const unsigned short* data =
                    accessor_data<unsigned short>(model, idx_acc);
                for (size_t i = 0; i < index_count; ++i) {
                    out.indices.push_back(data[i]);
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                const unsigned int* data =
                    accessor_data<unsigned int>(model, idx_acc);
                for (size_t i = 0; i < index_count; ++i) {
                    unsigned int value = data[i];
                    if (value > std::numeric_limits<unsigned short>::max()) {
                        err_out += "Index too large for raylib mesh\n";
                        return false;
                    }
                    out.indices.push_back(static_cast<unsigned short>(value));
                }
                break;
            }
            default:
                err_out += "Unsupported index component type\n";
                return false;
        }
    }

    return true;
}

raylib::Mesh make_raylib_mesh(const MeshBuffers& buffers) {
    raylib::Mesh mesh{};
    mesh.vertexCount = static_cast<int>(buffers.vertices.size() / 3);
    mesh.triangleCount = static_cast<int>(buffers.indices.size() / 3);

    mesh.vertices = static_cast<float*>(raylib::MemAlloc(
        static_cast<unsigned int>(sizeof(float) * buffers.vertices.size())));
    mesh.normals = static_cast<float*>(raylib::MemAlloc(
        static_cast<unsigned int>(sizeof(float) * buffers.normals.size())));
    mesh.texcoords = static_cast<float*>(raylib::MemAlloc(
        static_cast<unsigned int>(sizeof(float) * buffers.texcoords.size())));
    mesh.indices =
        static_cast<unsigned short*>(raylib::MemAlloc(static_cast<unsigned int>(
            sizeof(unsigned short) * buffers.indices.size())));

    std::memcpy(mesh.vertices, buffers.vertices.data(),
                sizeof(float) * buffers.vertices.size());
    std::memcpy(mesh.normals, buffers.normals.data(),
                sizeof(float) * buffers.normals.size());
    std::memcpy(mesh.texcoords, buffers.texcoords.data(),
                sizeof(float) * buffers.texcoords.size());
    std::memcpy(mesh.indices, buffers.indices.data(),
                sizeof(unsigned short) * buffers.indices.size());

    raylib::UploadMesh(&mesh, false);
    return mesh;
}

std::string mime_to_ext(const std::string& mime) {
    if (mime == "image/png") return "png";
    if (mime == "image/jpeg" || mime == "image/jpg") return "jpg";
    if (mime == "image/bmp") return "bmp";
    if (mime == "image/tga") return "tga";
    return "";
}

LoadedTexture load_texture_from_image(const tinygltf::Image& image,
                                      const std::string& base_dir) {
    LoadedTexture result{};

    // External image referenced by URI
    if (!image.uri.empty()) {
        const std::string path = base_dir + "/" + image.uri;
        raylib::Image img = raylib::LoadImage(path.c_str());
        if (img.data == nullptr) {
            return result;
        }
        result.texture = raylib::LoadTextureFromImage(img);
        raylib::UnloadImage(img);
        result.valid = result.texture.id != 0;
        return result;
    }

    // Embedded image data (GLB/base64). Prefer encoded bytes; fall back to raw.
    std::vector<unsigned char> bytes;
    if (!image.image.empty()) {
        bytes = image.image;
    }

    // Try encoded load first (common for GLB).
    if (!bytes.empty()) {
        std::string mime = image.mimeType;
        std::vector<std::string> exts_to_try;
        const std::string hinted = mime_to_ext(mime);
        if (!hinted.empty()) {
            exts_to_try.push_back(hinted);
        }
        // Fallback attempts
        exts_to_try.push_back("png");
        exts_to_try.push_back("jpg");
        exts_to_try.push_back("jpeg");
        exts_to_try.push_back("bmp");
        exts_to_try.push_back("tga");

        for (const auto& ext : exts_to_try) {
            raylib::Image img = raylib::LoadImageFromMemory(
                ext.c_str(), bytes.data(), static_cast<int>(bytes.size()));
            if (img.data != nullptr) {
                result.texture = raylib::LoadTextureFromImage(img);
                raylib::UnloadImage(img);
                result.valid = result.texture.id != 0;
                if (result.valid) {
                    return result;
                }
            }
        }
    }

    // Raw pixel fallback: tinygltf may have already decoded into RGBA
    if (!image.image.empty() && image.width > 0 && image.height > 0 &&
        (image.component == 3 || image.component == 4)) {
        raylib::Image img{};
        img.data =
            raylib::MemAlloc(static_cast<unsigned int>(image.image.size()));
        if (img.data == nullptr) {
            return result;
        }
        std::memcpy(img.data, image.image.data(), image.image.size());
        img.width = image.width;
        img.height = image.height;
        img.mipmaps = 1;
        img.format = (image.component == 4)
                         ? raylib::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
                         : raylib::PIXELFORMAT_UNCOMPRESSED_R8G8B8;

        result.texture = raylib::LoadTextureFromImage(img);
        raylib::MemFree(img.data);
        result.valid = result.texture.id != 0;
        return result;
    }

    return result;
}

raylib::Color to_color(const std::vector<double>& v) {
    auto clamp01 = [](double x) {
        if (x < 0.0) return 0.0;
        if (x > 1.0) return 1.0;
        return x;
    };
    double r_d = v.size() > 0 ? v[0] : 1.0;
    double g_d = v.size() > 1 ? v[1] : 1.0;
    double b_d = v.size() > 2 ? v[2] : 1.0;
    double a_d = v.size() > 3 ? v[3] : 1.0;
    unsigned char r =
        static_cast<unsigned char>(std::lround(clamp01(r_d) * 255.0));
    unsigned char g =
        static_cast<unsigned char>(std::lround(clamp01(g_d) * 255.0));
    unsigned char b =
        static_cast<unsigned char>(std::lround(clamp01(b_d) * 255.0));
    unsigned char a =
        static_cast<unsigned char>(std::lround(clamp01(a_d) * 255.0));
    return raylib::Color{r, g, b, a};
}

}  // namespace

namespace gltf_loader {

std::optional<raylib::Model> load_model(const std::string& filename,
                                        std::string& warn_out,
                                        std::string& err_out) {
    tinygltf::Model gltf_model;
    tinygltf::TinyGLTF loader;

    const bool is_glb = filename.size() >= 4 && (filename.ends_with(".glb") ||
                                                 filename.ends_with(".GLB"));

    bool ok = false;
    if (is_glb) {
        ok = loader.LoadBinaryFromFile(&gltf_model, &err_out, &warn_out,
                                       filename);
    } else {
        ok = loader.LoadASCIIFromFile(&gltf_model, &err_out, &warn_out,
                                      filename);
    }

    if (!ok) {
        err_out += "tinygltf failed to load file\n";
        return std::nullopt;
    }

    std::vector<LoadedTexture> textures;
    textures.reserve(gltf_model.images.size());
    std::string base_dir;
    {
        auto last_slash = filename.find_last_of("/\\");
        base_dir = (last_slash == std::string::npos)
                       ? "."
                       : filename.substr(0, last_slash);
    }
    for (const auto& img : gltf_model.images) {
        textures.push_back(load_texture_from_image(img, base_dir));
    }

    // Precompute global transforms per node (scene hierarchy aware)
    std::vector<raylib::Matrix> node_globals(gltf_model.nodes.size(),
                                             raylib::MatrixIdentity());
    std::vector<bool> visited(gltf_model.nodes.size(), false);
    std::function<void(int, raylib::Matrix)> dfs = [&](int node_idx,
                                                       raylib::Matrix parent) {
        if (node_idx < 0 ||
            node_idx >= static_cast<int>(gltf_model.nodes.size())) {
            return;
        }
        const tinygltf::Node& node =
            gltf_model.nodes[static_cast<size_t>(node_idx)];
        raylib::Matrix local = to_matrix(node);
        raylib::Matrix global = raylib::MatrixMultiply(parent, local);
        node_globals[static_cast<size_t>(node_idx)] = global;
        visited[static_cast<size_t>(node_idx)] = true;
        for (int child : node.children) {
            dfs(child, global);
        }
    };
    int scene_index =
        (gltf_model.defaultScene >= 0) ? gltf_model.defaultScene : 0;
    if (scene_index >= 0 &&
        scene_index < static_cast<int>(gltf_model.scenes.size())) {
        for (int root :
             gltf_model.scenes[static_cast<size_t>(scene_index)].nodes) {
            dfs(root, raylib::MatrixIdentity());
        }
    }
    // Fallback: any unvisited nodes get identity parent
    for (size_t i = 0; i < gltf_model.nodes.size(); ++i) {
        if (!visited[i]) {
            dfs(static_cast<int>(i), raylib::MatrixIdentity());
        }
    }

    // Materials
    std::vector<raylib::Material> materials;
    materials.reserve(gltf_model.materials.size());
    for (const auto& mat : gltf_model.materials) {
        raylib::Material material = raylib::LoadMaterialDefault();

        // Base color factor
        if (mat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            auto color = to_color(mat.pbrMetallicRoughness.baseColorFactor);
            material.maps[raylib::MATERIAL_MAP_DIFFUSE].color = color;
        }

        // Metallic / roughness factors
        material.maps[raylib::MATERIAL_MAP_METALNESS].value =
            static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
        material.maps[raylib::MATERIAL_MAP_ROUGHNESS].value =
            static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);

        int image_index = -1;
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            int tex_idx = mat.pbrMetallicRoughness.baseColorTexture.index;
            if (tex_idx < static_cast<int>(gltf_model.textures.size())) {
                image_index =
                    gltf_model.textures[static_cast<size_t>(tex_idx)].source;
            }
        } else if (!gltf_model.textures.empty() &&
                   gltf_model.textures[0].source >= 0) {
            image_index = gltf_model.textures[0].source;
        }

        if (image_index >= 0 &&
            image_index < static_cast<int>(textures.size()) &&
            textures[static_cast<size_t>(image_index)].valid) {
            material.maps[raylib::MATERIAL_MAP_DIFFUSE].texture =
                textures[static_cast<size_t>(image_index)].texture;
        }

        materials.push_back(material);
    }

    // Geometry
    std::vector<raylib::Mesh> meshes;
    std::vector<int> mesh_material_indices;

    for (const auto& node : gltf_model.nodes) {
        if (node.mesh < 0) {
            continue;
        }
        const raylib::Matrix transform =
            node_globals[static_cast<size_t>(&node - gltf_model.nodes.data())];
        const tinygltf::Mesh& mesh =
            gltf_model.meshes[static_cast<size_t>(node.mesh)];

        for (const auto& prim : mesh.primitives) {
            MeshBuffers buffers;
            if (!fill_mesh_buffers(gltf_model, prim, transform, buffers,
                                   err_out)) {
                continue;
            }
            meshes.push_back(make_raylib_mesh(buffers));

            int mat_index = prim.material;
            if (mat_index < 0 ||
                mat_index >= static_cast<int>(materials.size())) {
                mat_index = 0;
            }
            mesh_material_indices.push_back(mat_index);
        }
    }

    if (meshes.empty()) {
        err_out += "No meshes found in glTF\n";
        return std::nullopt;
    }

    // Assemble model
    raylib::Model model{};
    model.transform = raylib::MatrixIdentity();

    model.meshCount = static_cast<int>(meshes.size());
    model.meshes = static_cast<raylib::Mesh*>(raylib::MemAlloc(
        static_cast<unsigned int>(sizeof(raylib::Mesh) * meshes.size())));
    for (size_t i = 0; i < meshes.size(); ++i) {
        model.meshes[i] = meshes[i];
    }

    if (materials.empty()) {
        // Ensure at least one material exists
        materials.push_back(raylib::LoadMaterialDefault());
    }

    model.materialCount = static_cast<int>(materials.size());
    model.materials = static_cast<raylib::Material*>(
        raylib::MemAlloc(static_cast<unsigned int>(sizeof(raylib::Material) *
                                                   materials.size())));
    for (size_t i = 0; i < materials.size(); ++i) {
        model.materials[i] = materials[i];
    }

    model.meshMaterial = static_cast<int*>(raylib::MemAlloc(
        static_cast<unsigned int>(sizeof(int) * meshes.size())));
    for (size_t i = 0; i < meshes.size(); ++i) {
        model.meshMaterial[i] = mesh_material_indices[i];
    }

    return model;
}

}  // namespace gltf_loader
