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


#define TINYGLTF_NOEXCEPT noexcept


#define DEFAULT_METHODS(x)             \
  ~x() = default;                      \
  x(const x &) = default;              \
  x(x &&) TINYGLTF_NOEXCEPT = default; \
  x &operator=(const x &) = default;   \
  x &operator=(x &&) TINYGLTF_NOEXCEPT = default;



#define TINYGLTF_MODE_POINTS (0)
#define TINYGLTF_MODE_LINE (1)
#define TINYGLTF_MODE_LINE_LOOP (2)
#define TINYGLTF_MODE_LINE_STRIP (3)
#define TINYGLTF_MODE_TRIANGLES (4)
#define TINYGLTF_MODE_TRIANGLE_STRIP (5)
#define TINYGLTF_MODE_TRIANGLE_FAN (6)

#define TINYGLTF_COMPONENT_TYPE_BYTE (5120)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE (5121)
#define TINYGLTF_COMPONENT_TYPE_SHORT (5122)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT (5123)
#define TINYGLTF_COMPONENT_TYPE_INT (5124)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT (5125)
#define TINYGLTF_COMPONENT_TYPE_FLOAT (5126)
#define TINYGLTF_COMPONENT_TYPE_DOUBLE (5130)

#define TINYGLTF_TEXTURE_FILTER_NEAREST (9728)
#define TINYGLTF_TEXTURE_FILTER_LINEAR (9729)
#define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST (9984)
#define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST (9985)
#define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR (9986)
#define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR (9987)

#define TINYGLTF_TEXTURE_WRAP_REPEAT (10497)
#define TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE (33071)
#define TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT (33648)

// Redeclarations of the above for technique.parameters.
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE (5121)
#define TINYGLTF_PARAMETER_TYPE_SHORT (5122)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT (5123)
#define TINYGLTF_PARAMETER_TYPE_INT (5124)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT (5125)
#define TINYGLTF_PARAMETER_TYPE_FLOAT (5126)

#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC2 (35664)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 (35665)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4 (35666)

#define TINYGLTF_PARAMETER_TYPE_INT_VEC2 (35667)
#define TINYGLTF_PARAMETER_TYPE_INT_VEC3 (35668)
#define TINYGLTF_PARAMETER_TYPE_INT_VEC4 (35669)

#define TINYGLTF_PARAMETER_TYPE_BOOL (35670)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC2 (35671)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC3 (35672)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC4 (35673)

#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT2 (35674)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT3 (35675)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT4 (35676)

#define TINYGLTF_PARAMETER_TYPE_SAMPLER_2D (35678)

#define TINYGLTF_TYPE_VEC2 (2)
#define TINYGLTF_TYPE_VEC3 (3)
#define TINYGLTF_TYPE_VEC4 (4)
#define TINYGLTF_TYPE_MAT2 (32 + 2)
#define TINYGLTF_TYPE_MAT3 (32 + 3)
#define TINYGLTF_TYPE_MAT4 (32 + 4)
#define TINYGLTF_TYPE_SCALAR (64 + 1)
#define TINYGLTF_TYPE_VECTOR (64 + 4)
#define TINYGLTF_TYPE_MATRIX (64 + 16)

#define TINYGLTF_IMAGE_FORMAT_JPEG (0)
#define TINYGLTF_IMAGE_FORMAT_PNG (1)
#define TINYGLTF_IMAGE_FORMAT_BMP (2)
#define TINYGLTF_IMAGE_FORMAT_GIF (3)

#define TINYGLTF_TEXTURE_FORMAT_ALPHA (6406)
#define TINYGLTF_TEXTURE_FORMAT_RGB (6407)
#define TINYGLTF_TEXTURE_FORMAT_RGBA (6408)
#define TINYGLTF_TEXTURE_FORMAT_LUMINANCE (6409)
#define TINYGLTF_TEXTURE_FORMAT_LUMINANCE_ALPHA (6410)

#define TINYGLTF_TEXTURE_TARGET_TEXTURE2D (3553)
#define TINYGLTF_TEXTURE_TYPE_UNSIGNED_BYTE (5121)

#define TINYGLTF_TARGET_ARRAY_BUFFER (34962)
#define TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER (34963)

#define TINYGLTF_SHADER_TYPE_VERTEX_SHADER (35633)
#define TINYGLTF_SHADER_TYPE_FRAGMENT_SHADER (35632)

#define TINYGLTF_DOUBLE_EPS (1.e-12)
#define TINYGLTF_DOUBLE_EQUAL(a, b) (std::fabs((b) - (a)) < TINYGLTF_DOUBLE_EPS)

namespace tinygltf {

typedef enum {
  NULL_TYPE,
  REAL_TYPE,
  INT_TYPE,
  BOOL_TYPE,
  STRING_TYPE,
  ARRAY_TYPE,
  BINARY_TYPE,
  OBJECT_TYPE
} Type;

typedef enum {
  PERMISSIVE,
  STRICT
} ParseStrictness;

struct BufferView {
    std::string name;
    int buffer{-1};        // Required
    size_t byteOffset{0};  // minimum 0, default 0
    size_t byteLength{0};  // required, minimum 1. 0 = invalid
    size_t byteStride{0};  // minimum 4, maximum 252 (multiple of 4), default 0 =
    int target{0};
    BufferView() = default;
    DEFAULT_METHODS(BufferView)
};

struct Accessor
{
    int bufferView = -1;
    std::string name;
    size_t byteOffset = 0;
    bool normalized = false;
    int componentType = -1;
    size_t count{0};
    int type{-1};
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
    DEFAULT_METHODS(Accessor)
};

struct Primitive {
    std::map<std::string, int> attributes;
    int material{-1};
    int indices{-1};
    int mode{-1};
    std::vector<std::map<std::string, int> > targets;  // array of morph targets,
    Primitive() = default;
    DEFAULT_METHODS(Primitive)
};

struct Mesh {
    std::string name;
    std::vector<Primitive> primitives;
    std::vector<double> weights;  // weights to be applied to the Morph Targets
    Mesh() = default;
    DEFAULT_METHODS(Mesh)
};

class Node {
public:
    Node() = default;
    DEFAULT_METHODS(Node)
    int camera{-1};  // the index of the camera referenced by this node
    std::string name;
    int skin{-1};
    int mesh{-1};
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
  Buffer() = default;
  DEFAULT_METHODS(Buffer)
};

struct Asset {
  std::string version = "2.0";  // required
  std::string generator;
  std::string minVersion;
  std::string copyright;
  Asset() = default;
  DEFAULT_METHODS(Asset)
};

struct Scene {
  std::string name;
  std::vector<int> nodes;
  Scene() = default;
  DEFAULT_METHODS(Scene)
};

class Model {
 public:
  Model() = default;
  DEFAULT_METHODS(Model)
  std::vector<Accessor> accessors;
  std::vector<Buffer> buffers;
  std::vector<BufferView> bufferViews;
  std::vector<Mesh> meshes;
  std::vector<Node> nodes;
  std::vector<Scene> scenes;
  int defaultScene{-1};
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

typedef bool (*ReadWholeFileFunction)(std::vector<unsigned char> *,
                                      std::string *, const std::string &,
                                      void *);

typedef bool (*WriteWholeFileFunction)(std::string *, const std::string &,
                                       const std::vector<unsigned char> &,
                                       void *);

///
/// GetFileSizeFunction type. Signature for custom filesystem callbacks.
///
typedef bool (*GetFileSizeFunction)(size_t *filesize_out, std::string *err,
                                    const std::string &abs_filename,
                                    void *userdata);

///
/// A structure containing all required filesystem callbacks and a pointer to
/// their user data.
///
struct FsCallbacks {
  FileExistsFunction FileExists;
  ExpandFilePathFunction ExpandFilePath;
  ReadWholeFileFunction ReadWholeFile;
  WriteWholeFileFunction WriteWholeFile;
  GetFileSizeFunction GetFileSizeInBytes;  // To avoid GetFileSize Win32 API,
                                           // add `InBytes` suffix.

  void *user_data;  // An argument that is passed to all fs callbacks
};

bool FileExists(const std::string &abs_filename, void *);

std::string ExpandFilePath(const std::string &filepath, void *userdata);

bool ReadWholeFile(std::vector<unsigned char> *out, std::string *err,
                   const std::string &filepath, void *);

bool WriteWholeFile(std::string *err, const std::string &filepath,
                    const std::vector<unsigned char> &contents, void *);

bool GetFileSizeInBytes(size_t *filesize_out, std::string *err,
                        const std::string &filepath, void *);

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

    TinyGLTF() = default;
    ~TinyGLTF() = default;
    void SetParseStrictness(ParseStrictness strictness);
    void SetURICallbacks(URICallbacks callbacks);
    void SetSerializeDefaultValues(const bool enabled) { serialize_default_values_ = enabled; }
    bool GetSerializeDefaultValues() const { return serialize_default_values_; }
    void SetPreserveImageChannels(bool onoff) { preserve_image_channels_ = onoff; }
    void SetMaxExternalFileSize(size_t max_bytes) { max_external_file_size_ = max_bytes; }
    size_t GetMaxExternalFileSize() const { return max_external_file_size_; }
    bool GetPreserveImageChannels() const { return preserve_image_channels_; }
private:
    bool LoadFromString(Model *model, std::string *err, std::string *warn,
                      const char *str, const unsigned int length,
                      const std::string &base_dir, unsigned int check_sections);

    const unsigned char *bin_data_ = nullptr;
    size_t bin_size_ = 0;
    bool is_binary_ = false;
    ParseStrictness strictness_ = ParseStrictness::STRICT;
    bool serialize_default_values_ = false;  ///< Serialize default values?
    bool store_original_json_for_extras_and_extensions_ = false;
    bool preserve_image_channels_ = false;
    size_t max_external_file_size_{ size_t((std::numeric_limits<int32_t>::max)())};
    std::string warn_;
    std::string err_;

  FsCallbacks fs = {
      &tinygltf::FileExists,
      &tinygltf::ExpandFilePath,
      &tinygltf::ReadWholeFile,
      &tinygltf::WriteWholeFile,
      &tinygltf::GetFileSizeInBytes,

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

  void *load_image_user_data_{nullptr};
  bool user_image_loader_{false};

  void *write_image_user_data_{nullptr};
};

}  // namespace tinygltf
#endif  // TINY_GLTF_H_


