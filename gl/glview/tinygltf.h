#ifndef TINY_GLTF_H_
#define TINY_GLTF_H_

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <iostream>

static constexpr int
    TINYGLTF_MODE_POINTS = 0,
    TINYGLTF_MODE_LINE = 1,
    TINYGLTF_MODE_LINE_LOOP = 2,
    TINYGLTF_MODE_LINE_STRIP = 3,
    TINYGLTF_MODE_TRIANGLES = 4,
    TINYGLTF_MODE_TRIANGLE_STRIP = 5,
    TINYGLTF_MODE_TRIANGLE_FAN = 6;

#define TINYGLTF_COMPONENT_TYPE_BYTE (5120)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE (5121)
#define TINYGLTF_COMPONENT_TYPE_SHORT (5122)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT (5123)
#define TINYGLTF_COMPONENT_TYPE_INT (5124)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT (5125)
#define TINYGLTF_COMPONENT_TYPE_FLOAT (5126)
#define TINYGLTF_COMPONENT_TYPE_DOUBLE (5130)

#define TINYGLTF_TYPE_VEC2 (2)
#define TINYGLTF_TYPE_VEC3 (3)
#define TINYGLTF_TYPE_VEC4 (4)
#define TINYGLTF_TYPE_MAT2 (32 + 2)
#define TINYGLTF_TYPE_MAT3 (32 + 3)
#define TINYGLTF_TYPE_MAT4 (32 + 4)
#define TINYGLTF_TYPE_SCALAR (64 + 1)
#define TINYGLTF_TYPE_VECTOR (64 + 4)
#define TINYGLTF_TYPE_MATRIX (64 + 16)

static constexpr int TINYGLTF_TARGET_ARRAY_BUFFER = 34962;
static constexpr int TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER = 34963;

#define TINYGLTF_DOUBLE_EPS (1.e-12)
#define TINYGLTF_DOUBLE_EQUAL(a, b) (std::fabs((b) - (a)) < TINYGLTF_DOUBLE_EPS)

namespace tinygltf {

typedef enum {
  PERMISSIVE,
  STRICT
} ParseStrictness;

struct BufferView {
    std::string name;
    int buffer = -1;        // Required
    size_t byteOffset = 0;  // minimum 0, default 0
    size_t byteLength = 0;  // required, minimum 1. 0 = invalid
    size_t byteStride = 0;  // minimum 4, maximum 252 (multiple of 4), default 0 =
    int target = 0;
};

struct Accessor
{
    int bufferView = -1;
    std::string name;
    size_t byteOffset = 0;
    bool normalized = false;
    int componentType = -1;
    size_t count = 0;
    int type = -1;
    std::vector<double> minValues;
    std::vector<double> maxValues;

    struct Sparse {
        int count;
        bool isSparse;
        struct {
            int byteOffset;
            int bufferView;
            int componentType;
        } indices;

        struct {
            int bufferView;
            int byteOffset;
        } values;
    };

    Sparse sparse;
    int ByteStride(const BufferView &bufferViewObject) const;
    Accessor() { sparse.isSparse = false; }
};

struct Primitive {
    std::map<std::string, int> attributes;
    int material{-1};
    int indices{-1};
    int mode{-1};
    std::vector<std::map<std::string, int> > targets;  // array of morph targets,
};

struct Mesh
{
    std::string name;
    std::vector<Primitive> primitives;
    std::vector<double> weights;  // weights to be applied to the Morph Targets
};

class Node {
public:
    int camera = -1;  // the index of the camera referenced by this node
    std::string name;
    int skin = -1;
    int mesh = -1;
    std::vector<int> children;
    std::vector<double> rotation;     // length must be 0 or 4
    std::vector<double> scale;        // length must be 0 or 3
    std::vector<double> translation;  // length must be 0 or 3
    std::vector<double> matrix;       // length must be 0 or 16
    std::vector<double> weights;  // The weights of the instantiated Morph Target
};

struct Buffer {
    std::string name;
    std::vector<uint8_t> data;
    std::string uri;
};

struct Asset {
    std::string version = "2.0";  // required
    std::string generator;
    std::string minVersion;
    std::string copyright;
};

struct Scene {
    std::string name;
    std::vector<int> nodes;
};

class Model {
public:
    std::vector<Accessor> accessors;
    std::vector<Buffer> buffers;
    std::vector<BufferView> bufferViews;
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<Scene> scenes;
    int defaultScene = -1;
    Asset asset;
};

typedef bool (*URIEncodeFunction)(const std::string &in_uri,
                                  const std::string &object_type,
                                  std::string *out_uri, void *user_data);

typedef bool (*URIDecodeFunction)(const std::string &in_uri,
                                  std::string *out_uri, void *user_data);

bool URIDecode(const std::string &in_uri, std::string *out_uri, void *user_data);

struct URICallbacks {
    URIEncodeFunction encode;  // Optional encode method
    URIDecodeFunction decode;  // Required decode method
    void *user_data;  // An argument that is passed to all uri callbacks
};

typedef bool (*FileExistsFunction)(const std::string &abs_filename, void *);
typedef std::string (*ExpandFilePathFunction)(const std::string &, void *);

struct FsCallbacks {
    FileExistsFunction FileExists;
    ExpandFilePathFunction ExpandFilePath;
    void *user_data;  // An argument that is passed to all fs callbacks
};

bool FileExists(const std::string &abs_filename, void *);
std::string ExpandFilePath(const std::string &filepath, void *userdata);

class TinyGLTF {
public:
    bool LoadASCIIFromFile(Model *model, std::string *err, std::string *warn,
                         const std::string &filename);

    bool LoadASCIIFromString(Model *model, std::string *err, std::string *warn,
                           const char *str, const unsigned int length,
                           const std::string &base_dir);

    bool LoadBinaryFromFile(Model *model, std::string *err, std::string *warn,
                          const std::string &filename);

    bool LoadBinaryFromMemory(Model *model, std::string *err, std::string *warn,
                            const unsigned char *bytes, const unsigned int length,
                            const std::string &base_dir = "");

    void SetURICallbacks(URICallbacks callbacks);
private:
    bool LoadFromString(Model *model, std::string *err,
                      const char *str, const unsigned int length, const std::string &base_dir);

    const uint8_t *bin_data_ = nullptr;
    size_t bin_size_ = 0;
    bool is_binary_ = false;
    bool serialize_default_values_ = false;  ///< Serialize default values?
    bool preserve_image_channels_ = false;
    size_t max_external_file_size_{ size_t((std::numeric_limits<int32_t>::max)())};
    std::string warn_;
    std::string err_;

    FsCallbacks fs = {
      &tinygltf::FileExists,
      &tinygltf::ExpandFilePath,
      nullptr  // Fs callback user data
    };

    URICallbacks uri_cb = {
      // Use paths as-is by default. This will use JSON string escaping.
      nullptr,
      // Decode all URIs before using them as paths as the application may have
      // percent encoded them.
      &tinygltf::URIDecode,
      // URI callback user data
      nullptr};
};

}  // namespace tinygltf
#endif  // TINY_GLTF_H_


