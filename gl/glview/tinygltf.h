//
// Header-only tiny glTF 2.0 loader and serializer.
//
//
// The MIT License (MIT)
//
// Copyright (c) 2015 - Present Syoyo Fujita, Aurélien Chatelain and many
// contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Version: - v2.8.10
// See https://github.com/syoyo/tinygltf/releases for release history.
//
// Tiny glTF loader is using following third party libraries:
//
//  - jsonhpp: C++ JSON library.
//  - base64: base64 decode/encode library.
//  - stb_image: Image loading library.
//
#ifndef TINY_GLTF_H_
#define TINY_GLTF_H_

#include <array>
#include <cassert>
#include <cmath>  // std::fabs
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>

// Auto-detect C++14 standard version
#if !defined(TINYGLTF_USE_CPP14) && defined(__cplusplus) && \
    (__cplusplus >= 201402L)
#define TINYGLTF_USE_CPP14
#endif

#ifdef __GNUC__
#if (__GNUC__ < 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ <= 8))
#define TINYGLTF_NOEXCEPT
#else
#define TINYGLTF_NOEXCEPT noexcept
#endif
#else
#define TINYGLTF_NOEXCEPT noexcept
#endif

#define DEFAULT_METHODS(x)             \
  ~x() = default;                      \
  x(const x &) = default;              \
  x(x &&) TINYGLTF_NOEXCEPT = default; \
  x &operator=(const x &) = default;   \
  x &operator=(x &&) TINYGLTF_NOEXCEPT = default;

namespace tinygltf {

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
#define TINYGLTF_COMPONENT_TYPE_DOUBLE \
  (5130)  // OpenGL double type. Note that some of glTF 2.0 validator does not
          // support double type even the schema seems allow any value of
          // integer:
          // https://github.com/KhronosGroup/glTF/blob/b9884a2fd45130b4d673dd6c8a706ee21ee5c5f7/specification/2.0/schema/accessor.schema.json#L22

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
#define TINYGLTF_PARAMETER_TYPE_BYTE (5120)
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

// End parameter types

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

static inline int32_t GetComponentSizeInBytes(uint32_t componentType) {
  if (componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
    return 1;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    return 1;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
    return 2;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    return 2;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_INT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
    return 8;
  } else {
    // Unknown component type
    return -1;
  }
}

static inline int32_t GetNumComponentsInType(uint32_t ty) {
  if (ty == TINYGLTF_TYPE_SCALAR) {
    return 1;
  } else if (ty == TINYGLTF_TYPE_VEC2) {
    return 2;
  } else if (ty == TINYGLTF_TYPE_VEC3) {
    return 3;
  } else if (ty == TINYGLTF_TYPE_VEC4) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT2) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT3) {
    return 9;
  } else if (ty == TINYGLTF_TYPE_MAT4) {
    return 16;
  } else {
    // Unknown component type
    return -1;
  }
}

// TODO(syoyo): Move these functions to TinyGLTF class
bool IsDataURI(const std::string &in);
bool DecodeDataURI(std::vector<unsigned char> *out, std::string &mime_type,
                   const std::string &in, size_t reqBytes, bool checkSize);

#ifdef __clang__
#pragma clang diagnostic push
// Suppress warning for : static Value null_value
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wpadded"
#endif

// Simple class to represent JSON object
class Value {
 public:
  typedef std::vector<Value> Array;
  typedef std::map<std::string, Value> Object;

  Value() = default;

  explicit Value(bool b) : type_(BOOL_TYPE) { boolean_value_ = b; }
  explicit Value(int i) : type_(INT_TYPE) {
    int_value_ = i;
    real_value_ = i;
  }
  explicit Value(double n) : type_(REAL_TYPE) { real_value_ = n; }
  explicit Value(const std::string &s) : type_(STRING_TYPE) {
    string_value_ = s;
  }
  explicit Value(std::string &&s)
      : type_(STRING_TYPE), string_value_(std::move(s)) {}
  explicit Value(const char *s) : type_(STRING_TYPE) { string_value_ = s; }
  explicit Value(const unsigned char *p, size_t n) : type_(BINARY_TYPE) {
    binary_value_.resize(n);
    memcpy(binary_value_.data(), p, n);
  }
  explicit Value(std::vector<unsigned char> &&v) noexcept
      : type_(BINARY_TYPE),
        binary_value_(std::move(v)) {}
  explicit Value(const Array &a) : type_(ARRAY_TYPE) { array_value_ = a; }
  explicit Value(Array &&a) noexcept : type_(ARRAY_TYPE),
                                       array_value_(std::move(a)) {}

  explicit Value(const Object &o) : type_(OBJECT_TYPE) { object_value_ = o; }
  explicit Value(Object &&o) noexcept : type_(OBJECT_TYPE),
                                        object_value_(std::move(o)) {}

  DEFAULT_METHODS(Value)

  char Type() const { return static_cast<char>(type_); }

  bool IsBool() const { return (type_ == BOOL_TYPE); }

  bool IsInt() const { return (type_ == INT_TYPE); }

  bool IsNumber() const { return (type_ == REAL_TYPE) || (type_ == INT_TYPE); }

  bool IsReal() const { return (type_ == REAL_TYPE); }

  bool IsString() const { return (type_ == STRING_TYPE); }

  bool IsBinary() const { return (type_ == BINARY_TYPE); }

  bool IsArray() const { return (type_ == ARRAY_TYPE); }

  bool IsObject() const { return (type_ == OBJECT_TYPE); }

  // Use this function if you want to have number value as double.
  double GetNumberAsDouble() const {
    if (type_ == INT_TYPE) {
      return double(int_value_);
    } else {
      return real_value_;
    }
  }

  // Use this function if you want to have number value as int.
  // TODO(syoyo): Support int value larger than 32 bits
  int GetNumberAsInt() const {
    if (type_ == REAL_TYPE) {
      return int(real_value_);
    } else {
      return int_value_;
    }
  }

  // Accessor
  template <typename T>
  const T &Get() const;
  template <typename T>
  T &Get();

  // Lookup value from an array
  const Value &Get(int idx) const {
    static Value null_value;
    assert(IsArray());
    assert(idx >= 0);
    return (static_cast<size_t>(idx) < array_value_.size())
               ? array_value_[static_cast<size_t>(idx)]
               : null_value;
  }

  // Lookup value from a key-value pair
  const Value &Get(const std::string &key) const {
    static Value null_value;
    assert(IsObject());
    Object::const_iterator it = object_value_.find(key);
    return (it != object_value_.end()) ? it->second : null_value;
  }

  size_t ArrayLen() const {
    if (!IsArray()) return 0;
    return array_value_.size();
  }

  // Valid only for object type.
  bool Has(const std::string &key) const {
    if (!IsObject()) return false;
    Object::const_iterator it = object_value_.find(key);
    return (it != object_value_.end()) ? true : false;
  }

  // List keys
  std::vector<std::string> Keys() const {
    std::vector<std::string> keys;
    if (!IsObject()) return keys;  // empty

    for (Object::const_iterator it = object_value_.begin();
         it != object_value_.end(); ++it) {
      keys.push_back(it->first);
    }

    return keys;
  }

  size_t Size() const { return (IsArray() ? ArrayLen() : Keys().size()); }

  bool operator==(const tinygltf::Value &other) const;

 protected:
  int type_ = NULL_TYPE;

  int int_value_ = 0;
  double real_value_ = 0.0;
  std::string string_value_;
  std::vector<unsigned char> binary_value_;
  Array array_value_;
  Object object_value_;
  bool boolean_value_ = false;
};

#define TINYGLTF_VALUE_GET(ctype, var)            \
  template <>                                     \
  inline const ctype &Value::Get<ctype>() const { \
    return var;                                   \
  }                                               \
  template <>                                     \
  inline ctype &Value::Get<ctype>() {             \
    return var;                                   \
  }
TINYGLTF_VALUE_GET(bool, boolean_value_)
TINYGLTF_VALUE_GET(double, real_value_)
TINYGLTF_VALUE_GET(int, int_value_)
TINYGLTF_VALUE_GET(std::string, string_value_)
TINYGLTF_VALUE_GET(std::vector<unsigned char>, binary_value_)
TINYGLTF_VALUE_GET(Value::Array, array_value_)
TINYGLTF_VALUE_GET(Value::Object, object_value_)
#undef TINYGLTF_VALUE_GET

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wpadded"
#endif

/// Aggregate object for representing a color
using ColorValue = std::array<double, 4>;

// === legacy interface ====
// TODO(syoyo): Deprecate `Parameter` class.
struct Parameter {
  bool bool_value = false;
  bool has_number_value = false;
  std::string string_value;
  std::vector<double> number_array;
  std::map<std::string, double> json_double_value;
  double number_value = 0.0;

  /// Material factor, like the roughness or metalness of a material
  /// Returned value is only valid if the parameter represent a texture from a
  /// material
  double Factor() const { return number_value; }

  /// Return the color of a material
  /// Returned value is only valid if the parameter represent a texture from a
  /// material
  ColorValue ColorFactor() const {
    return {
        {// this aggregate initialize the std::array object, and uses C++11 RVO.
         number_array[0], number_array[1], number_array[2],
         (number_array.size() > 3 ? number_array[3] : 1.0)}};
  }

  Parameter() = default;
  DEFAULT_METHODS(Parameter)
  bool operator==(const Parameter &) const;
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

typedef std::map<std::string, Parameter> ParameterMap;
typedef std::map<std::string, Value> ExtensionMap;

struct Skin {
  std::string name;
  int inverseBindMatrices{-1};  // required here but not in the spec
  int skeleton{-1};             // The index of the node used as a skeleton root
  std::vector<int> joints;      // Indices of skeleton nodes

  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Skin() = default;
  DEFAULT_METHODS(Skin)
  bool operator==(const Skin &) const;
};

struct Sampler {
  std::string name;
  // glTF 2.0 spec does not define default value for `minFilter` and
  // `magFilter`. Set -1 in TinyGLTF(issue #186)
  int minFilter =
      -1;  // optional. -1 = no filter defined. ["NEAREST", "LINEAR",
           // "NEAREST_MIPMAP_NEAREST", "LINEAR_MIPMAP_NEAREST",
           // "NEAREST_MIPMAP_LINEAR", "LINEAR_MIPMAP_LINEAR"]
  int magFilter =
      -1;  // optional. -1 = no filter defined. ["NEAREST", "LINEAR"]
  int wrapS =
      TINYGLTF_TEXTURE_WRAP_REPEAT;  // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT",
                                     // "REPEAT"], default "REPEAT"
  int wrapT =
      TINYGLTF_TEXTURE_WRAP_REPEAT;  // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT",
                                     // "REPEAT"], default "REPEAT"
  // int wrapR = TINYGLTF_TEXTURE_WRAP_REPEAT;  // TinyGLTF extension. currently
  // not used.

  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Sampler() = default;
  DEFAULT_METHODS(Sampler)
  bool operator==(const Sampler &) const;
};

struct Image {
  std::string name;
  int width{-1};
  int height{-1};
  int component{-1};
  int bits{-1};        // bit depth per channel. 8(byte), 16 or 32.
  int pixel_type{-1};  // pixel type(TINYGLTF_COMPONENT_TYPE_***). usually
                       // UBYTE(bits = 8) or USHORT(bits = 16)
  std::vector<unsigned char> image;
  int bufferView{-1};    // (required if no uri)
  std::string mimeType;  // (required if no uri) ["image/jpeg", "image/png",
                         // "image/bmp", "image/gif"]
  std::string uri;       // (required if no mimeType) uri is not decoded(e.g.
                         // whitespace may be represented as %20)
  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  bool as_is{false};

  Image() = default;
  DEFAULT_METHODS(Image)

  bool operator==(const Image &) const;
};

struct Texture {
  std::string name;

  int sampler{-1};
  int source{-1};
  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Texture() = default;
  DEFAULT_METHODS(Texture)

  bool operator==(const Texture &) const;
};

struct TextureInfo {
  int index{-1};    // required.
  int texCoord{0};  // The set index of texture's TEXCOORD attribute used for
                    // texture coordinate mapping.

  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  TextureInfo() = default;
  DEFAULT_METHODS(TextureInfo)
  bool operator==(const TextureInfo &) const;
};

struct NormalTextureInfo {
  int index{-1};    // required
  int texCoord{0};  // The set index of texture's TEXCOORD attribute used for
                    // texture coordinate mapping.
  double scale{
      1.0};  // scaledNormal = normalize((<sampled normal texture value>
             // * 2.0 - 1.0) * vec3(<normal scale>, <normal scale>, 1.0))

  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  NormalTextureInfo() = default;
  DEFAULT_METHODS(NormalTextureInfo)
  bool operator==(const NormalTextureInfo &) const;
};

struct OcclusionTextureInfo {
  int index{-1};    // required
  int texCoord{0};  // The set index of texture's TEXCOORD attribute used for
                    // texture coordinate mapping.
  double strength{1.0};  // occludedColor = lerp(color, color * <sampled
                         // occlusion texture value>, <occlusion strength>)

  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  OcclusionTextureInfo() = default;
  DEFAULT_METHODS(OcclusionTextureInfo)
  bool operator==(const OcclusionTextureInfo &) const;
};

// pbrMetallicRoughness class defined in glTF 2.0 spec.
struct PbrMetallicRoughness {
  std::vector<double> baseColorFactor;  // len = 4. default [1,1,1,1]
  TextureInfo baseColorTexture;
  double metallicFactor{1.0};   // default 1
  double roughnessFactor{1.0};  // default 1
  TextureInfo metallicRoughnessTexture;

  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  PbrMetallicRoughness()
      : baseColorFactor(std::vector<double>{1.0, 1.0, 1.0, 1.0}) {}
  DEFAULT_METHODS(PbrMetallicRoughness)
  bool operator==(const PbrMetallicRoughness &) const;
};

// Each extension should be stored in a ParameterMap.
// members not in the values could be included in the ParameterMap
// to keep a single material model
struct Material {
  std::string name;

  std::vector<double> emissiveFactor;  // length 3. default [0, 0, 0]
  std::string alphaMode;               // default "OPAQUE"
  double alphaCutoff{0.5};             // default 0.5
  bool doubleSided{false};             // default false;

  PbrMetallicRoughness pbrMetallicRoughness;

  NormalTextureInfo normalTexture;
  OcclusionTextureInfo occlusionTexture;
  TextureInfo emissiveTexture;

  // For backward compatibility
  // TODO(syoyo): Remove `values` and `additionalValues` in the next release.
  ParameterMap values;
  ParameterMap additionalValues;

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Material() : alphaMode("OPAQUE") {}
  DEFAULT_METHODS(Material)

  bool operator==(const Material &) const;
};

struct BufferView {
  std::string name;
  int buffer{-1};        // Required
  size_t byteOffset{0};  // minimum 0, default 0
  size_t byteLength{0};  // required, minimum 1. 0 = invalid
  size_t byteStride{0};  // minimum 4, maximum 252 (multiple of 4), default 0 =
                         // understood to be tightly packed
  int target{0};  // ["ARRAY_BUFFER", "ELEMENT_ARRAY_BUFFER"] for vertex indices
                  // or attribs. Could be 0 for other data
  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  bool dracoDecoded{false};  // Flag indicating this has been draco decoded

  BufferView() = default;
  DEFAULT_METHODS(BufferView)
  bool operator==(const BufferView &) const;
};

struct Accessor {
  int bufferView{-1};  // optional in spec but required here since sparse
                       // accessor are not supported
  std::string name;
  size_t byteOffset{0};
  bool normalized{false};  // optional.
  int componentType{-1};   // (required) One of TINYGLTF_COMPONENT_TYPE_***
  size_t count{0};         // required
  int type{-1};            // (required) One of TINYGLTF_TYPE_***   ..
  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  std::vector<double>
      minValues;  // optional. integer value is promoted to double
  std::vector<double>
      maxValues;  // optional. integer value is promoted to double

  struct Sparse {
    int count;
    bool isSparse;
    struct {
      int byteOffset;
      int bufferView;
      int componentType;  // a TINYGLTF_COMPONENT_TYPE_ value
      Value extras;
      ExtensionMap extensions;
      std::string extras_json_string;
      std::string extensions_json_string;
    } indices;
    struct {
      int bufferView;
      int byteOffset;
      Value extras;
      ExtensionMap extensions;
      std::string extras_json_string;
      std::string extensions_json_string;
    } values;
    Value extras;
    ExtensionMap extensions;
    std::string extras_json_string;
    std::string extensions_json_string;
  };

  Sparse sparse;

  ///
  /// Utility function to compute byteStride for a given bufferView object.
  /// Returns -1 upon invalid glTF value or parameter configuration.
  ///
  int ByteStride(const BufferView &bufferViewObject) const {
    if (bufferViewObject.byteStride == 0) {
      // Assume data is tightly packed.
      int componentSizeInBytes =
          GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
      if (componentSizeInBytes <= 0) {
        return -1;
      }

      int numComponents = GetNumComponentsInType(static_cast<uint32_t>(type));
      if (numComponents <= 0) {
        return -1;
      }

      return componentSizeInBytes * numComponents;
    } else {
      // Check if byteStride is a multiple of the size of the accessor's
      // component type.
      int componentSizeInBytes =
          GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
      if (componentSizeInBytes <= 0) {
        return -1;
      }

      if ((bufferViewObject.byteStride % uint32_t(componentSizeInBytes)) != 0) {
        return -1;
      }
      return static_cast<int>(bufferViewObject.byteStride);
    }

    // unreachable return 0;
  }

  Accessor()

  {
    sparse.isSparse = false;
  }
  DEFAULT_METHODS(Accessor)
  bool operator==(const tinygltf::Accessor &) const;
};

struct PerspectiveCamera {
  double aspectRatio{0.0};  // min > 0
  double yfov{0.0};         // required. min > 0
  double zfar{0.0};         // min > 0
  double znear{0.0};        // required. min > 0

  PerspectiveCamera() = default;
  DEFAULT_METHODS(PerspectiveCamera)
  bool operator==(const PerspectiveCamera &) const;

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

struct OrthographicCamera {
  double xmag{0.0};   // required. must not be zero.
  double ymag{0.0};   // required. must not be zero.
  double zfar{0.0};   // required. `zfar` must be greater than `znear`.
  double znear{0.0};  // required

  OrthographicCamera() = default;
  DEFAULT_METHODS(OrthographicCamera)
  bool operator==(const OrthographicCamera &) const;

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

struct Camera {
  std::string type;  // required. "perspective" or "orthographic"
  std::string name;

  PerspectiveCamera perspective;
  OrthographicCamera orthographic;

  Camera() = default;
  DEFAULT_METHODS(Camera)
  bool operator==(const Camera &) const;

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

struct Primitive {
  std::map<std::string, int> attributes;  // (required) A dictionary object of
                                          // integer, where each integer
                                          // is the index of the accessor
                                          // containing an attribute.
  int material{-1};  // The index of the material to apply to this primitive
                     // when rendering.
  int indices{-1};   // The index of the accessor that contains the indices.
  int mode{-1};      // one of TINYGLTF_MODE_***
  std::vector<std::map<std::string, int> > targets;  // array of morph targets,
  // where each target is a dict with attributes in ["POSITION, "NORMAL",
  // "TANGENT"] pointing
  // to their corresponding accessors
  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Primitive() = default;
  DEFAULT_METHODS(Primitive)
  bool operator==(const Primitive &) const;
};

struct Mesh {
  std::string name;
  std::vector<Primitive> primitives;
  std::vector<double> weights;  // weights to be applied to the Morph Targets
  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Mesh() = default;
  DEFAULT_METHODS(Mesh)
  bool operator==(const Mesh &) const;
};

class Node {
 public:
  Node() = default;

  DEFAULT_METHODS(Node)

  bool operator==(const Node &) const;

  int camera{-1};  // the index of the camera referenced by this node

  std::string name;
  int skin{-1};
  int mesh{-1};
  int light{-1};    // light source index (KHR_lights_punctual)
  int emitter{-1};  // audio emitter index (KHR_audio)
  std::vector<int> children;
  std::vector<double> rotation;     // length must be 0 or 4
  std::vector<double> scale;        // length must be 0 or 3
  std::vector<double> translation;  // length must be 0 or 3
  std::vector<double> matrix;       // length must be 0 or 16
  std::vector<double> weights;  // The weights of the instantiated Morph Target

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

struct Buffer {
  std::string name;
  std::vector<unsigned char> data;
  std::string
      uri;  // considered as required here but not in the spec (need to clarify)
            // uri is not decoded(e.g. whitespace may be represented as %20)
  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Buffer() = default;
  DEFAULT_METHODS(Buffer)
  bool operator==(const Buffer &) const;
};

struct Asset {
  std::string version = "2.0";  // required
  std::string generator;
  std::string minVersion;
  std::string copyright;
  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Asset() = default;
  DEFAULT_METHODS(Asset)
  bool operator==(const Asset &) const;
};

struct Scene {
  std::string name;
  std::vector<int> nodes;
  std::vector<int> audioEmitters;  // KHR_audio global emitters

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;

  Scene() = default;
  DEFAULT_METHODS(Scene)
  bool operator==(const Scene &) const;
};

struct SpotLight {
  double innerConeAngle{0.0};
  double outerConeAngle{0.7853981634};

  SpotLight() = default;
  DEFAULT_METHODS(SpotLight)
  bool operator==(const SpotLight &) const;

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

struct Light {
  std::string name;
  std::vector<double> color;
  double intensity{1.0};
  std::string type;
  double range{0.0};  // 0.0 = infinite
  SpotLight spot;

  Light() = default;
  DEFAULT_METHODS(Light)

  bool operator==(const Light &) const;

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

struct PositionalEmitter {
  double coneInnerAngle{6.283185307179586};
  double coneOuterAngle{6.283185307179586};
  double coneOuterGain{0.0};
  double maxDistance{100.0};
  double refDistance{1.0};
  double rolloffFactor{1.0};

  PositionalEmitter() = default;
  DEFAULT_METHODS(PositionalEmitter)
  bool operator==(const PositionalEmitter &) const;

  ExtensionMap extensions;
  Value extras;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

class Model {
 public:
  Model() = default;
  DEFAULT_METHODS(Model)

  bool operator==(const Model &) const;

  std::vector<Accessor> accessors;
  //std::vector<Animation> animations;
  std::vector<Buffer> buffers;
  std::vector<BufferView> bufferViews;
  std::vector<Material> materials;
  std::vector<Mesh> meshes;
  std::vector<Node> nodes;
  //std::vector<Texture> textures;
  std::vector<Image> images;
  //std::vector<Skin> skins;
  std::vector<Sampler> samplers;
  std::vector<Camera> cameras;
  std::vector<Scene> scenes;
  std::vector<Light> lights;

  int defaultScene{-1};
  std::vector<std::string> extensionsUsed;
  std::vector<std::string> extensionsRequired;

  Asset asset;

  Value extras;
  ExtensionMap extensions;

  // Filled when SetStoreOriginalJSONForExtrasAndExtensions is enabled.
  std::string extras_json_string;
  std::string extensions_json_string;
};

enum SectionCheck {
  NO_REQUIRE = 0x00,
  REQUIRE_VERSION = 0x01,
  REQUIRE_SCENE = 0x02,
  REQUIRE_SCENES = 0x04,
  REQUIRE_NODES = 0x08,
  REQUIRE_ACCESSORS = 0x10,
  REQUIRE_BUFFERS = 0x20,
  REQUIRE_BUFFER_VIEWS = 0x40,
  REQUIRE_ALL = 0x7f
};

///
/// URIEncodeFunction type. Signature for custom URI encoding of external
/// resources such as .bin and image files. Used by tinygltf to re-encode the
/// final location of saved files. object_type may be used to encode buffer and
/// image URIs differently, for example. See
/// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#uris
///
typedef bool (*URIEncodeFunction)(const std::string &in_uri,
                                  const std::string &object_type,
                                  std::string *out_uri, void *user_data);

///
/// URIDecodeFunction type. Signature for custom URI decoding of external
/// resources such as .bin and image files. Used by tinygltf when computing
/// filenames to write resources.
///
typedef bool (*URIDecodeFunction)(const std::string &in_uri,
                                  std::string *out_uri, void *user_data);

// Declaration of default uri decode function
bool URIDecode(const std::string &in_uri, std::string *out_uri,
               void *user_data);

///
/// A structure containing URI callbacks and a pointer to their user data.
///
struct URICallbacks {
  URIEncodeFunction encode;  // Optional encode method
  URIDecodeFunction decode;  // Required decode method

  void *user_data;  // An argument that is passed to all uri callbacks
};

typedef bool (*FileExistsFunction)(const std::string &abs_filename, void *);

///
/// ExpandFilePathFunction type. Signature for custom filesystem callbacks.
///
typedef std::string (*ExpandFilePathFunction)(const std::string &, void *);

///
/// ReadWholeFileFunction type. Signature for custom filesystem callbacks.
///
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

#ifndef TINYGLTF_NO_FS
// Declaration of default filesystem callbacks

bool FileExists(const std::string &abs_filename, void *);

///
/// Expand file path(e.g. `~` to home directory on posix, `%APPDATA%` to
/// `C:\\Users\\tinygltf\\AppData`)
///
/// @param[in] filepath File path string. Assume UTF-8
/// @param[in] userdata User data. Set to `nullptr` if you don't need it.
///
std::string ExpandFilePath(const std::string &filepath, void *userdata);

bool ReadWholeFile(std::vector<unsigned char> *out, std::string *err,
                   const std::string &filepath, void *);

bool WriteWholeFile(std::string *err, const std::string &filepath,
                    const std::vector<unsigned char> &contents, void *);

bool GetFileSizeInBytes(size_t *filesize_out, std::string *err,
                        const std::string &filepath, void *);
#endif

///
/// glTF Parser/Serializer context.
///
class TinyGLTF {
 public:
  TinyGLTF() = default;
  ~TinyGLTF() = default;

  ///
  /// Loads glTF ASCII asset from a file.
  /// Set warning message to `warn` for example it fails to load asserts.
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadASCIIFromFile(Model *model, std::string *err, std::string *warn,
                         const std::string &filename,
                         unsigned int check_sections = REQUIRE_VERSION);

  ///
  /// Loads glTF ASCII asset from string(memory).
  /// `length` = strlen(str);
  /// `base_dir` is a search path of glTF asset(e.g. images). Path Must be an
  /// expanded path (e.g. no tilde(`~`), no environment variables). Set warning
  /// message to `warn` for example it fails to load asserts. Returns false and
  /// set error string to `err` if there's an error.
  ///
  bool LoadASCIIFromString(Model *model, std::string *err, std::string *warn,
                           const char *str, const unsigned int length,
                           const std::string &base_dir,
                           unsigned int check_sections = REQUIRE_VERSION);

  ///
  /// Loads glTF binary asset from a file.
  /// Set warning message to `warn` for example it fails to load asserts.
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadBinaryFromFile(Model *model, std::string *err, std::string *warn,
                          const std::string &filename,
                          unsigned int check_sections = REQUIRE_VERSION);

  ///
  /// Loads glTF binary asset from memory.
  /// `length` = strlen(str);
  /// `base_dir` is a search path of glTF asset(e.g. images). Path Must be an
  /// expanded path (e.g. no tilde(`~`), no environment variables).
  /// Set warning message to `warn` for example it fails to load asserts.
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadBinaryFromMemory(Model *model, std::string *err, std::string *warn,
                            const unsigned char *bytes,
                            const unsigned int length,
                            const std::string &base_dir = "",
                            unsigned int check_sections = REQUIRE_VERSION);

  ///
  /// Write glTF to stream, buffers and images will be embedded
  ///
  bool WriteGltfSceneToStream(const Model *model, std::ostream &stream,
                              bool prettyPrint, bool writeBinary);

  ///
  /// Sets the parsing strictness.
  ///
  void SetParseStrictness(ParseStrictness strictness);

  ///
  /// Unset(remove) callback of loading image data
  ///

  ///
  /// Set callbacks to use for URI encoding and decoding and their user data
  ///
  void SetURICallbacks(URICallbacks callbacks);

  void SetSerializeDefaultValues(const bool enabled) {
    serialize_default_values_ = enabled;
  }

  bool GetSerializeDefaultValues() const { return serialize_default_values_; }

  ///
  /// Store original JSON string for `extras` and `extensions`.
  /// This feature will be useful when the user want to reconstruct custom data
  /// structure from JSON string.
  ///
  void SetStoreOriginalJSONForExtrasAndExtensions(const bool enabled) {
    store_original_json_for_extras_and_extensions_ = enabled;
  }

  bool GetStoreOriginalJSONForExtrasAndExtensions() const {
    return store_original_json_for_extras_and_extensions_;
  }

  void SetPreserveImageChannels(bool onoff) {
    preserve_image_channels_ = onoff;
  }

  ///
  /// Set maximum allowed external file size in bytes.
  /// Default: 2GB
  /// Only effective for built-in ReadWholeFileFunction FS function.
  ///
  void SetMaxExternalFileSize(size_t max_bytes) {
    max_external_file_size_ = max_bytes;
  }

  size_t GetMaxExternalFileSize() const { return max_external_file_size_; }

  bool GetPreserveImageChannels() const { return preserve_image_channels_; }

 private:
  ///
  /// Loads glTF asset from string(memory).
  /// `length` = strlen(str);
  /// Set warning message to `warn` for example it fails to load asserts
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadFromString(Model *model, std::string *err, std::string *warn,
                      const char *str, const unsigned int length,
                      const std::string &base_dir, unsigned int check_sections);

  const unsigned char *bin_data_ = nullptr;
  size_t bin_size_ = 0;
  bool is_binary_ = false;

  ParseStrictness strictness_ = ParseStrictness::STRICT;

  bool serialize_default_values_ = false;  ///< Serialize default values?

  bool store_original_json_for_extras_and_extensions_ = false;

  bool preserve_image_channels_ = false;  /// Default false(expand channels to
                                          /// RGBA) for backward compatibility.

  size_t max_external_file_size_{
      size_t((std::numeric_limits<int32_t>::max)())};  // Default 2GB

  // Warning & error messages
  std::string warn_;
  std::string err_;

  FsCallbacks fs = {
#ifndef TINYGLTF_NO_FS
      &tinygltf::FileExists,
      &tinygltf::ExpandFilePath,
      &tinygltf::ReadWholeFile,
      &tinygltf::WriteWholeFile,
      &tinygltf::GetFileSizeInBytes,

      nullptr  // Fs callback user data
#else
      nullptr, nullptr, nullptr, nullptr, nullptr,

      nullptr  // Fs callback user data
#endif
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

#ifdef __clang__
#pragma clang diagnostic pop  // -Wpadded
#endif

}  // namespace tinygltf

#endif  // TINY_GLTF_H_

#if defined(TINYGLTF_IMPLEMENTATION) || defined(__INTELLISENSE__)
#include <algorithm>
// #include <cassert>
#ifndef TINYGLTF_NO_FS
#include <sys/stat.h>  // for is_directory check

#include <cstdio>
#include <fstream>
#endif
#include <sstream>

// Disable GCC warnings
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif  // __GNUC__

#ifndef TINYGLTF_NO_INCLUDE_JSON
#ifndef TINYGLTF_USE_RAPIDJSON
#ifdef __GNUC__
#pragma GCC visibility push(hidden)
#endif
#include <nlohmann/json.hpp>
#ifdef __GNUC__
#pragma GCC visibility pop
#endif
#else
#endif
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _WIN32

// issue 143.
// Define NOMINMAX to avoid min/max defines,
// but undef it after included Windows.h
#ifndef NOMINMAX
#define TINYGLTF_INTERNAL_NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define TINYGLTF_INTERNAL_WIN32_LEAN_AND_MEAN
#endif
#ifndef __MINGW32__
#include <Windows.h>  // include API for expanding a file path
#else
#include <windows.h>
#endif

#ifdef TINYGLTF_INTERNAL_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#if defined(TINYGLTF_INTERNAL_NOMINMAX)
#undef NOMINMAX
#endif

#if defined(__GLIBCXX__)  // mingw

#include <fcntl.h>  // _O_RDONLY

#include <ext/stdio_filebuf.h>  // fstream (all sorts of IO stuff) + stdio_filebuf (=streambuf)

#endif

#elif !defined(__ANDROID__) && !defined(__OpenBSD__)
// #include <wordexp.h>
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TINYGLTF_LITTLE_ENDIAN 1
#endif

namespace tinygltf {
namespace detail {
#ifdef TINYGLTF_USE_RAPIDJSON

#ifdef TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR
// This uses the RapidJSON CRTAllocator.  It is thread safe and multiple
// documents may be active at once.
using json =
    rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::CrtAllocator>;
using json_iterator = json::MemberIterator;
using json_const_iterator = json::ConstMemberIterator;
using json_const_array_iterator = json const *;
using JsonDocument =
    rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator>;
rapidjson::CrtAllocator s_CrtAllocator;  // stateless and thread safe
rapidjson::CrtAllocator &GetAllocator() { return s_CrtAllocator; }
#else
// This uses the default RapidJSON MemoryPoolAllocator.  It is very fast, but
// not thread safe. Only a single JsonDocument may be active at any one time,
// meaning only a single gltf load/save can be active any one time.
using json = rapidjson::Value;
using json_iterator = json::MemberIterator;
using json_const_iterator = json::ConstMemberIterator;
using json_const_array_iterator = json const *;
rapidjson::Document *s_pActiveDocument = nullptr;
rapidjson::Document::AllocatorType &GetAllocator() {
  assert(s_pActiveDocument);  // Root json node must be JsonDocument type
  return s_pActiveDocument->GetAllocator();
}

struct JsonDocument : public rapidjson::Document {
  JsonDocument() {
    assert(s_pActiveDocument ==
           nullptr);  // When using default allocator, only one document can be
                      // active at a time, if you need multiple active at once,
                      // define TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR
    s_pActiveDocument = this;
  }
  JsonDocument(const JsonDocument &) = delete;
  JsonDocument(JsonDocument &&rhs) noexcept
      : rapidjson::Document(std::move(rhs)) {
    s_pActiveDocument = this;
    rhs.isNil = true;
  }
  ~JsonDocument() {
    if (!isNil) {
      s_pActiveDocument = nullptr;
    }
  }

 private:
  bool isNil = false;
};

#endif  // TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR

#else
using nlohmann::json;
using json_iterator = json::iterator;
using json_const_iterator = json::const_iterator;
using json_const_array_iterator = json_const_iterator;
using JsonDocument = json;
#endif

void JsonParse(JsonDocument &doc, const char *str, size_t length, bool throwExc = false)
{
  doc = detail::json::parse(str, str + length, nullptr, throwExc);
}
}  // namespace detail
}  // namespace tinygltf

namespace tinygltf {

struct LoadImageDataOption {
  bool preserve_channels{false};
};

// Equals function for Value, for recursivity
static bool Equals(const tinygltf::Value &one, const tinygltf::Value &other) {
  if (one.Type() != other.Type()) return false;

  switch (one.Type()) {
    case NULL_TYPE:
      return true;
    case BOOL_TYPE:
      return one.Get<bool>() == other.Get<bool>();
    case REAL_TYPE:
      return TINYGLTF_DOUBLE_EQUAL(one.Get<double>(), other.Get<double>());
    case INT_TYPE:
      return one.Get<int>() == other.Get<int>();
    case OBJECT_TYPE: {
      auto oneObj = one.Get<tinygltf::Value::Object>();
      auto otherObj = other.Get<tinygltf::Value::Object>();
      if (oneObj.size() != otherObj.size()) return false;
      for (auto &it : oneObj) {
        auto otherIt = otherObj.find(it.first);
        if (otherIt == otherObj.end()) return false;

        if (!Equals(it.second, otherIt->second)) return false;
      }
      return true;
    }
    case ARRAY_TYPE: {
      if (one.Size() != other.Size()) return false;
      for (int i = 0; i < int(one.Size()); ++i)
        if (!Equals(one.Get(i), other.Get(i))) return false;
      return true;
    }
    case STRING_TYPE:
      return one.Get<std::string>() == other.Get<std::string>();
    case BINARY_TYPE:
      return one.Get<std::vector<unsigned char> >() ==
             other.Get<std::vector<unsigned char> >();
    default: {
      // unhandled type
      return false;
    }
  }
}

// Equals function for std::vector<double> using TINYGLTF_DOUBLE_EPSILON
static bool Equals(const std::vector<double> &one,
                   const std::vector<double> &other) {
  if (one.size() != other.size()) return false;
  for (int i = 0; i < int(one.size()); ++i) {
    if (!TINYGLTF_DOUBLE_EQUAL(one[size_t(i)], other[size_t(i)])) return false;
  }
  return true;
}

bool Accessor::operator==(const Accessor &other) const {
  return this->bufferView == other.bufferView &&
         this->byteOffset == other.byteOffset &&
         this->componentType == other.componentType &&
         this->count == other.count && this->extensions == other.extensions &&
         this->extras == other.extras &&
         Equals(this->maxValues, other.maxValues) &&
         Equals(this->minValues, other.minValues) && this->name == other.name &&
         this->normalized == other.normalized && this->type == other.type;
}
bool Asset::operator==(const Asset &other) const {
  return this->copyright == other.copyright &&
         this->extensions == other.extensions && this->extras == other.extras &&
         this->generator == other.generator &&
         this->minVersion == other.minVersion && this->version == other.version;
}
bool Buffer::operator==(const Buffer &other) const {
  return this->data == other.data && this->extensions == other.extensions &&
         this->extras == other.extras && this->name == other.name &&
         this->uri == other.uri;
}
bool BufferView::operator==(const BufferView &other) const {
  return this->buffer == other.buffer && this->byteLength == other.byteLength &&
         this->byteOffset == other.byteOffset &&
         this->byteStride == other.byteStride && this->name == other.name &&
         this->target == other.target && this->extensions == other.extensions &&
         this->extras == other.extras &&
         this->dracoDecoded == other.dracoDecoded;
}
bool Mesh::operator==(const Mesh &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         this->name == other.name && Equals(this->weights, other.weights) &&
         this->primitives == other.primitives;
}
bool Node::operator==(const Node &other) const {
  return this->camera == other.camera && this->children == other.children &&
         this->extensions == other.extensions && this->extras == other.extras &&
         Equals(this->matrix, other.matrix) && this->mesh == other.mesh &&
         (this->light == other.light) && (this->emitter == other.emitter) &&
         this->name == other.name && Equals(this->rotation, other.rotation) &&
         Equals(this->scale, other.scale) && this->skin == other.skin &&
         Equals(this->translation, other.translation) &&
         Equals(this->weights, other.weights);
}
bool SpotLight::operator==(const SpotLight &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         TINYGLTF_DOUBLE_EQUAL(this->innerConeAngle, other.innerConeAngle) &&
         TINYGLTF_DOUBLE_EQUAL(this->outerConeAngle, other.outerConeAngle);
}
bool PositionalEmitter::operator==(const PositionalEmitter &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         TINYGLTF_DOUBLE_EQUAL(this->coneInnerAngle, other.coneInnerAngle) &&
         TINYGLTF_DOUBLE_EQUAL(this->coneOuterAngle, other.coneOuterAngle) &&
         TINYGLTF_DOUBLE_EQUAL(this->coneOuterGain, other.coneOuterGain) &&
         TINYGLTF_DOUBLE_EQUAL(this->maxDistance, other.maxDistance) &&
         TINYGLTF_DOUBLE_EQUAL(this->refDistance, other.refDistance) &&
         TINYGLTF_DOUBLE_EQUAL(this->rolloffFactor, other.rolloffFactor);
}
bool OrthographicCamera::operator==(const OrthographicCamera &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         TINYGLTF_DOUBLE_EQUAL(this->xmag, other.xmag) &&
         TINYGLTF_DOUBLE_EQUAL(this->ymag, other.ymag) &&
         TINYGLTF_DOUBLE_EQUAL(this->zfar, other.zfar) &&
         TINYGLTF_DOUBLE_EQUAL(this->znear, other.znear);
}
bool Parameter::operator==(const Parameter &other) const {
  if (this->bool_value != other.bool_value ||
      this->has_number_value != other.has_number_value)
    return false;

  if (!TINYGLTF_DOUBLE_EQUAL(this->number_value, other.number_value))
    return false;

  if (this->json_double_value.size() != other.json_double_value.size())
    return false;
  for (auto &it : this->json_double_value) {
    auto otherIt = other.json_double_value.find(it.first);
    if (otherIt == other.json_double_value.end()) return false;

    if (!TINYGLTF_DOUBLE_EQUAL(it.second, otherIt->second)) return false;
  }

  if (!Equals(this->number_array, other.number_array)) return false;

  if (this->string_value != other.string_value) return false;

  return true;
}
bool PerspectiveCamera::operator==(const PerspectiveCamera &other) const {
  return TINYGLTF_DOUBLE_EQUAL(this->aspectRatio, other.aspectRatio) &&
         this->extensions == other.extensions && this->extras == other.extras &&
         TINYGLTF_DOUBLE_EQUAL(this->yfov, other.yfov) &&
         TINYGLTF_DOUBLE_EQUAL(this->zfar, other.zfar) &&
         TINYGLTF_DOUBLE_EQUAL(this->znear, other.znear);
}
bool Primitive::operator==(const Primitive &other) const {
  return this->attributes == other.attributes && this->extras == other.extras &&
         this->indices == other.indices && this->material == other.material &&
         this->mode == other.mode && this->targets == other.targets;
}
bool Value::operator==(const Value &other) const {
  return Equals(*this, other);
}

static void swap4(unsigned int *val) {
#ifdef TINYGLTF_LITTLE_ENDIAN
  (void)val;
#else
  unsigned int tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
#endif
}

static std::string JoinPath(const std::string &path0,
                            const std::string &path1) {
  if (path0.empty()) {
    return path1;
  } else {
    // check '/'
    char lastChar = *path0.rbegin();
    if (lastChar != '/') {
      return path0 + std::string("/") + path1;
    } else {
      return path0 + path1;
    }
  }
}

static std::string FindFile(const std::vector<std::string> &paths,
                            const std::string &filepath, FsCallbacks *fs) {
  if (fs == nullptr || fs->ExpandFilePath == nullptr ||
      fs->FileExists == nullptr) {
    // Error, fs callback[s] missing
    return std::string();
  }

  // https://github.com/syoyo/tinygltf/issues/416
  // Use strlen() since std::string's size/length reports the number of elements
  // in the buffer, not the length of string(null-terminated) strip
  // null-character in the middle of string.
  size_t slength = strlen(filepath.c_str());
  if (slength == 0) {
    return std::string();
  }

  std::string cleaned_filepath = std::string(filepath.c_str());

  for (size_t i = 0; i < paths.size(); i++) {
    std::string absPath =
        fs->ExpandFilePath(JoinPath(paths[i], cleaned_filepath), fs->user_data);
    if (fs->FileExists(absPath, fs->user_data)) {
      return absPath;
    }
  }

  return std::string();
}

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

static std::string GetBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

static std::string GetBaseFilename(const std::string &filepath) {
  auto idx = filepath.find_last_of("/\\");
  if (idx != std::string::npos) return filepath.substr(idx + 1);
  return filepath;
}

std::string base64_encode(unsigned char const *, unsigned int len);
std::string base64_decode(std::string const &s);

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const *bytes_to_encode,
                          unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  const char *base64_chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];

    while ((i++ < 3)) ret += '=';
  }

  return ret;
}

std::string base64_decode(std::string const &encoded_string) {
  int in_len = static_cast<int>(encoded_string.size());
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  const std::string base64_chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  while (in_len-- && (encoded_string[in_] != '=') &&
         is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] =
            static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] =
          static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

namespace dlib {

inline unsigned char from_hex(unsigned char ch) {
  if (ch <= '9' && ch >= '0')
    ch -= '0';
  else if (ch <= 'f' && ch >= 'a')
    ch -= 'a' - 10;
  else if (ch <= 'F' && ch >= 'A')
    ch -= 'A' - 10;
  else
    ch = 0;
  return ch;
}

static const std::string urldecode(const std::string &str) {
  using namespace std;
  string result;
  string::size_type i;
  for (i = 0; i < str.size(); ++i) {
    if (str[i] == '+') {
      result += ' ';
    } else if (str[i] == '%' && str.size() > i + 2) {
      const unsigned char ch1 =
          from_hex(static_cast<unsigned char>(str[i + 1]));
      const unsigned char ch2 =
          from_hex(static_cast<unsigned char>(str[i + 2]));
      const unsigned char ch = static_cast<unsigned char>((ch1 << 4) | ch2);
      result += static_cast<char>(ch);
      i += 2;
    } else {
      result += str[i];
    }
  }
  return result;
}

}  // namespace dlib
// --- dlib end --------------------------------------------------------------

bool URIDecode(const std::string &in_uri, std::string *out_uri,
               void *user_data) {
  (void)user_data;
  *out_uri = dlib::urldecode(in_uri);
  return true;
}

static bool LoadExternalFile(std::vector<unsigned char> *out, std::string *err,
                             std::string *warn, const std::string &filename,
                             const std::string &basedir, bool required,
                             size_t reqBytes, bool checkSize,
                             size_t maxFileSize, FsCallbacks *fs) {
  if (fs == nullptr || fs->FileExists == nullptr ||
      fs->ExpandFilePath == nullptr || fs->ReadWholeFile == nullptr) {
    // This is a developer error, assert() ?
    if (err) {
      (*err) += "FS callback[s] not set\n";
    }
    return false;
  }

  std::string *failMsgOut = required ? err : warn;

  out->clear();

  std::vector<std::string> paths;
  paths.push_back(basedir);
  paths.push_back(".");

  std::string filepath = FindFile(paths, filename, fs);
  if (filepath.empty() || filename.empty()) {
    if (failMsgOut) {
      (*failMsgOut) += "File not found : " + filename + "\n";
    }
    return false;
  }

  // Check file size
  if (fs->GetFileSizeInBytes) {
    size_t file_size{0};
    std::string _err;
    bool ok =
        fs->GetFileSizeInBytes(&file_size, &_err, filepath, fs->user_data);
    if (!ok) {
      if (_err.size()) {
        if (failMsgOut) {
          (*failMsgOut) += "Getting file size failed : " + filename +
                           ", err = " + _err + "\n";
        }
      }
      return false;
    }

    if (file_size > maxFileSize) {
      if (failMsgOut) {
        (*failMsgOut) += "File size " + std::to_string(file_size) +
                         " exceeds maximum allowed file size " +
                         std::to_string(maxFileSize) + " : " + filepath + "\n";
      }
      return false;
    }
  }

  std::vector<unsigned char> buf;
  std::string fileReadErr;
  bool fileRead =
      fs->ReadWholeFile(&buf, &fileReadErr, filepath, fs->user_data);
  if (!fileRead) {
    if (failMsgOut) {
      (*failMsgOut) +=
          "File read error : " + filepath + " : " + fileReadErr + "\n";
    }
    return false;
  }

  size_t sz = buf.size();
  if (sz == 0) {
    if (failMsgOut) {
      (*failMsgOut) += "File is empty : " + filepath + "\n";
    }
    return false;
  }

  if (checkSize) {
    if (reqBytes == sz) {
      out->swap(buf);
      return true;
    } else {
      std::stringstream ss;
      ss << "File size mismatch : " << filepath << ", requestedBytes "
         << reqBytes << ", but got " << sz << std::endl;
      if (failMsgOut) {
        (*failMsgOut) += ss.str();
      }
      return false;
    }
  }

  out->swap(buf);
  return true;
}

void TinyGLTF::SetParseStrictness(ParseStrictness strictness) {
  strictness_ = strictness;
}

void TinyGLTF::SetURICallbacks(URICallbacks callbacks) {
  assert(callbacks.decode);
  if (callbacks.decode) {
    uri_cb = callbacks;
  }
}


#ifndef TINYGLTF_NO_FS
// Default implementations of filesystem functions

bool FileExists(const std::string &abs_filename, void *) {
  bool ret;
#ifdef TINYGLTF_ANDROID_LOAD_FROM_ASSETS
  if (asset_manager) {
    AAsset *asset = AAssetManager_open(asset_manager, abs_filename.c_str(),
                                       AASSET_MODE_STREAMING);
    if (!asset) {
      return false;
    }
    AAsset_close(asset);
    ret = true;
  } else {
    return false;
  }
#else
#ifdef _WIN32
#if defined(_MSC_VER) || defined(_LIBCPP_VERSION)

  // First check if a file is a directory.
  DWORD result = GetFileAttributesW(UTF8ToWchar(abs_filename).c_str());
  if (result == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  if (result & FILE_ATTRIBUTE_DIRECTORY) {
    return false;
  }

  FILE *fp = nullptr;
  errno_t err = _wfopen_s(&fp, UTF8ToWchar(abs_filename).c_str(), L"rb");
  if (err != 0) {
    return false;
  }
#elif defined(__GLIBCXX__)
  FILE *fp = fopen(abs_filename.c_str(), "rb");
  if (!fp) {
    return false;
  }
#else
  // TODO: is_directory check
  FILE *fp = nullptr;
  errno_t err = fopen_s(&fp, abs_filename.c_str(), "rb");
  if (err != 0) {
    return false;
  }
#endif

#else
  struct stat sb;
  if (stat(abs_filename.c_str(), &sb)) {
    return false;
  }
  if (S_ISDIR(sb.st_mode)) {
    return false;
  }

  FILE *fp = fopen(abs_filename.c_str(), "rb");
#endif
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }
#endif

  return ret;
}

std::string ExpandFilePath(const std::string &filepath, void *) {
  // https://github.com/syoyo/tinygltf/issues/368
  //
  // No file path expansion in built-in FS function anymore, since glTF URI
  // should not contain tilde('~') and environment variables, and for security
  // reason(`wordexp`).
  //
  // Users need to supply `base_dir`(in `LoadASCIIFromString`,
  // `LoadBinaryFromMemory`) in expanded absolute path.

  return filepath;
}

bool GetFileSizeInBytes(size_t *filesize_out, std::string *err,
                        const std::string &filepath, void *userdata) {
  (void)userdata;

  std::ifstream f(filepath.c_str(), std::ifstream::binary);
  if (!f) {
    if (err) {
      (*err) += "File open error : " + filepath + "\n";
    }
    return false;
  }

  // For directory(and pipe?), peek() will fail(Posix gnustl/libc++ only)
  f.peek();
  if (!f) {
    if (err) {
      (*err) +=
          "File read error. Maybe empty file or invalid file : " + filepath +
          "\n";
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());

  // std::cout << "sz = " << sz << "\n";
  f.seekg(0, f.beg);

  if (int64_t(sz) < 0) {
    if (err) {
      (*err) += "Invalid file size : " + filepath +
                " (does the path point to a directory?)";
    }
    return false;
  } else if (sz == 0) {
    if (err) {
      (*err) += "File is empty : " + filepath + "\n";
    }
    return false;
  } else if (sz >= (std::numeric_limits<std::streamoff>::max)()) {
    if (err) {
      (*err) += "Invalid file size : " + filepath + "\n";
    }
    return false;
  }

  (*filesize_out) = sz;
  return true;
}

bool ReadWholeFile(std::vector<unsigned char> *out, std::string *err,
                   const std::string &filepath, void *) {
#ifdef TINYGLTF_ANDROID_LOAD_FROM_ASSETS
  if (asset_manager) {
    AAsset *asset = AAssetManager_open(asset_manager, filepath.c_str(),
                                       AASSET_MODE_STREAMING);
    if (!asset) {
      if (err) {
        (*err) += "File open error : " + filepath + "\n";
      }
      return false;
    }
    size_t size = AAsset_getLength(asset);
    if (size == 0) {
      if (err) {
        (*err) += "Invalid file size : " + filepath +
                  " (does the path point to a directory?)";
      }
      return false;
    }
    out->resize(size);
    AAsset_read(asset, reinterpret_cast<char *>(&out->at(0)), size);
    AAsset_close(asset);
    return true;
  } else {
    if (err) {
      (*err) += "No asset manager specified : " + filepath + "\n";
    }
    return false;
  }
#else
#ifdef _WIN32
#if defined(__GLIBCXX__)  // mingw
  int file_descriptor =
      _wopen(UTF8ToWchar(filepath).c_str(), _O_RDONLY | _O_BINARY);
  __gnu_cxx::stdio_filebuf<char> wfile_buf(file_descriptor, std::ios_base::in);
  std::istream f(&wfile_buf);
#elif defined(_MSC_VER) || defined(_LIBCPP_VERSION)
  // For libcxx, assume _LIBCPP_HAS_OPEN_WITH_WCHAR is defined to accept
  // `wchar_t *`
  std::ifstream f(UTF8ToWchar(filepath).c_str(), std::ifstream::binary);
#else
  // Unknown compiler/runtime
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
#endif
#else
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
#endif
  if (!f) {
    if (err) {
      (*err) += "File open error : " + filepath + "\n";
    }
    return false;
  }

  // For directory(and pipe?), peek() will fail(Posix gnustl/libc++ only)
  f.peek();
  if (!f) {
    if (err) {
      (*err) +=
          "File read error. Maybe empty file or invalid file : " + filepath +
          "\n";
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());

  // std::cout << "sz = " << sz << "\n";
  f.seekg(0, f.beg);

  if (int64_t(sz) < 0) {
    if (err) {
      (*err) += "Invalid file size : " + filepath +
                " (does the path point to a directory?)";
    }
    return false;
  } else if (sz == 0) {
    if (err) {
      (*err) += "File is empty : " + filepath + "\n";
    }
    return false;
  } else if (sz >= (std::numeric_limits<std::streamoff>::max)()) {
    if (err) {
      (*err) += "Invalid file size : " + filepath + "\n";
    }
    return false;
  }

  out->resize(sz);
  f.read(reinterpret_cast<char *>(&out->at(0)),
         static_cast<std::streamsize>(sz));

  return true;
#endif
}

bool WriteWholeFile(std::string *err, const std::string &filepath,
                    const std::vector<unsigned char> &contents, void *) {
#ifdef _WIN32
#if defined(__GLIBCXX__)  // mingw
  int file_descriptor = _wopen(UTF8ToWchar(filepath).c_str(),
                               _O_CREAT | _O_WRONLY | _O_TRUNC | _O_BINARY);
  __gnu_cxx::stdio_filebuf<char> wfile_buf(
      file_descriptor, std::ios_base::out | std::ios_base::binary);
  std::ostream f(&wfile_buf);
#elif defined(_MSC_VER)
  std::ofstream f(UTF8ToWchar(filepath).c_str(), std::ofstream::binary);
#else  // clang?
  std::ofstream f(filepath.c_str(), std::ofstream::binary);
#endif
#else
  std::ofstream f(filepath.c_str(), std::ofstream::binary);
#endif
  if (!f) {
    if (err) {
      (*err) += "File open error for writing : " + filepath + "\n";
    }
    return false;
  }

  f.write(reinterpret_cast<const char *>(&contents.at(0)),
          static_cast<std::streamsize>(contents.size()));
  if (!f) {
    if (err) {
      (*err) += "File write error: " + filepath + "\n";
    }
    return false;
  }

  return true;
}

#endif  // TINYGLTF_NO_FS

static std::string MimeToExt(const std::string &mimeType) {
  if (mimeType == "image/jpeg") {
    return "jpg";
  } else if (mimeType == "image/png") {
    return "png";
  } else if (mimeType == "image/bmp") {
    return "bmp";
  } else if (mimeType == "image/gif") {
    return "gif";
  }

  return "";
}

bool IsDataURI(const std::string &in) {
  std::string header = "data:application/octet-stream;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/jpeg;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/png;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/bmp;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/gif;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:text/plain;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:application/gltf-buffer;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  return false;
}

bool DecodeDataURI(std::vector<unsigned char> *out, std::string &mime_type,
                   const std::string &in, size_t reqBytes, bool checkSize) {
  std::string header = "data:application/octet-stream;base64,";
  std::string data;
  if (in.find(header) == 0) {
    data = base64_decode(in.substr(header.size()));  // cut mime string.
  }

  if (data.empty()) {
    header = "data:image/jpeg;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/jpeg";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/png;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/png";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/bmp;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/bmp";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/gif;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/gif";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:text/plain;base64,";
    if (in.find(header) == 0) {
      mime_type = "text/plain";
      data = base64_decode(in.substr(header.size()));
    }
  }

  if (data.empty()) {
    header = "data:application/gltf-buffer;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));
    }
  }

  // TODO(syoyo): Allow empty buffer? #229
  if (data.empty()) {
    return false;
  }

  if (checkSize) {
    if (data.size() != reqBytes) {
      return false;
    }
    out->resize(reqBytes);
  } else {
    out->resize(data.size());
  }
  std::copy(data.begin(), data.end(), out->begin());
  return true;
}

namespace detail {
bool GetInt(const detail::json &o, int &val) {
  auto type = o.type();

  if ((type == detail::json::value_t::number_integer) ||
      (type == detail::json::value_t::number_unsigned)) {
    val = static_cast<int>(o.get<int64_t>());
    return true;
  }

  return false;
}

#ifdef TINYGLTF_USE_RAPIDJSON
asdf
bool GetDouble(const detail::json &o, double &val) {
  if (o.IsDouble()) {
    val = o.GetDouble();
    return true;
  }

  return false;
}
#endif

bool GetNumber(const detail::json &o, double &val) {
#ifdef TINYGLTF_USE_RAPIDJSON
asdf
  if (o.IsNumber()) {
    val = o.GetDouble();
    return true;
  }

  return false;
#else
  if (o.is_number()) {
    val = o.get<double>();
    return true;
  }

  return false;
#endif
}

bool GetString(const detail::json &o, std::string &val) {
  if (o.type() == detail::json::value_t::string) {
    val = o.get<std::string>();
    return true;
  }

  return false;
}

bool IsArray(const detail::json &o) {
  return o.is_array();
}

detail::json_const_array_iterator ArrayBegin(const detail::json &o) {
  return o.begin();
}

detail::json_const_array_iterator ArrayEnd(const detail::json &o) {
  return o.end();
}

bool IsObject(const detail::json &o) {
  return o.is_object();
}

detail::json_const_iterator ObjectBegin(const detail::json &o) {
  return o.begin();
}

detail::json_const_iterator ObjectEnd(const detail::json &o) {
  return o.end();
}

// Making this a const char* results in a pointer to a temporary when
// TINYGLTF_USE_RAPIDJSON is off.
std::string GetKey(detail::json_const_iterator &it) {
  return it.key().c_str();
}

bool FindMember(const detail::json &o, const char *member,
                detail::json_const_iterator &it)
{
  it = o.find(member);
  return it != o.end();
}

bool FindMember(detail::json &o, const char *member,
                detail::json_iterator &it) {
  it = o.find(member);
  return it != o.end();
}

void Erase(detail::json &o, detail::json_iterator &it) {
  o.erase(it);
}

bool IsEmpty(const detail::json &o) {
  return o.empty();
}

const detail::json &GetValue(detail::json_const_iterator &it) {
  return it.value();
}

detail::json &GetValue(detail::json_iterator &it) {
  return it.value();
}

std::string JsonToString(const detail::json &o, int spacing = -1) {
  return o.dump(spacing);
}

}  // namespace detail

static bool ParseJsonAsValue(Value *ret, const detail::json &o) {
  Value val{};

  switch (o.type()) {
    case detail::json::value_t::object: {
      Value::Object value_object;
      for (auto it = o.begin(); it != o.end(); it++) {
        Value entry;
        ParseJsonAsValue(&entry, it.value());
        if (entry.Type() != NULL_TYPE)
          value_object.emplace(it.key(), std::move(entry));
      }
      if (value_object.size() > 0) val = Value(std::move(value_object));
    } break;
    case detail::json::value_t::array: {
      Value::Array value_array;
      value_array.reserve(o.size());
      for (auto it = o.begin(); it != o.end(); it++) {
        Value entry;
        ParseJsonAsValue(&entry, it.value());
        if (entry.Type() != NULL_TYPE)
          value_array.emplace_back(std::move(entry));
      }
      if (value_array.size() > 0) val = Value(std::move(value_array));
    } break;
    case detail::json::value_t::string:
      val = Value(o.get<std::string>());
      break;
    case detail::json::value_t::boolean:
      val = Value(o.get<bool>());
      break;
    case detail::json::value_t::number_integer:
    case detail::json::value_t::number_unsigned:
      val = Value(static_cast<int>(o.get<int64_t>()));
      break;
    case detail::json::value_t::number_float:
      val = Value(o.get<double>());
      break;
    case detail::json::value_t::null:
    case detail::json::value_t::discarded:
    case detail::json::value_t::binary:
      // default:
      break;
  }
  const bool isNotNull = val.Type() != NULL_TYPE;

  if (ret) *ret = std::move(val);

  return isNotNull;
}

static bool ParseExtrasProperty(Value *ret, const detail::json &o) {
  detail::json_const_iterator it;
  if (!detail::FindMember(o, "extras", it)) {
    return false;
  }

  return ParseJsonAsValue(ret, detail::GetValue(it));
}

static bool ParseBooleanProperty(bool *ret, std::string *err,
                                 const detail::json &o,
                                 const std::string &property,
                                 const bool required,
                                 const std::string &parent_node = "") {
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  auto &value = detail::GetValue(it);

  bool isBoolean;
  bool boolValue = false;
  isBoolean = value.is_boolean();
  if (isBoolean) {
    boolValue = value.get<bool>();
  }
  if (!isBoolean) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a bool type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = boolValue;
  }

  return true;
}

static bool ParseIntegerProperty(int *ret, std::string *err,
                                 const detail::json &o,
                                 const std::string &property,
                                 const bool required,
                                 const std::string &parent_node = "")
{
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  int intValue;
  bool isInt = detail::GetInt(detail::GetValue(it), intValue);
  if (!isInt) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an integer type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = intValue;
  }

  return true;
}

static bool ParseUnsignedProperty(size_t *ret, std::string *err,
                                  const detail::json &o,
                                  const std::string &property,
                                  const bool required,
                                  const std::string &parent_node = "") {
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  auto &value = detail::GetValue(it);

  size_t uValue = 0;
  bool isUValue;
#ifdef TINYGLTF_USE_RAPIDJSON
  isUValue = false;
  if (value.IsUint()) {
    uValue = value.GetUint();
    isUValue = true;
  } else if (value.IsUint64()) {
    uValue = value.GetUint64();
    isUValue = true;
  }
#else
  isUValue = value.is_number_unsigned();
  if (isUValue) {
    uValue = value.get<size_t>();
  }
#endif
  if (!isUValue) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a positive integer.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = uValue;
  }

  return true;
}

static bool ParseNumberProperty(double *ret, std::string *err,
                                const detail::json &o,
                                const std::string &property,
                                const bool required,
                                const std::string &parent_node = "") {
  detail::json_const_iterator it;

  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  double numberValue;
  bool isNumber = detail::GetNumber(detail::GetValue(it), numberValue);

  if (!isNumber) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a number type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = numberValue;
  }

  return true;
}

static bool ParseNumberArrayProperty(std::vector<double> *ret, std::string *err,
                                     const detail::json &o,
                                     const std::string &property, bool required,
                                     const std::string &parent_node = "") {
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!detail::IsArray(detail::GetValue(it))) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an array";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  ret->clear();
  auto end = detail::ArrayEnd(detail::GetValue(it));
  for (auto i = detail::ArrayBegin(detail::GetValue(it)); i != end; ++i) {
    double numberValue;
    const bool isNumber = detail::GetNumber(*i, numberValue);
    if (!isNumber) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' property is not a number.\n";
          if (!parent_node.empty()) {
            (*err) += " in " + parent_node;
          }
          (*err) += ".\n";
        }
      }
      return false;
    }
    ret->push_back(numberValue);
  }

  return true;
}

static bool ParseIntegerArrayProperty(std::vector<int> *ret, std::string *err,
                                      const detail::json &o,
                                      const std::string &property,
                                      bool required,
                                      const std::string &parent_node = "") {
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!detail::IsArray(detail::GetValue(it))) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an array";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  ret->clear();
  auto end = detail::ArrayEnd(detail::GetValue(it));
  for (auto i = detail::ArrayBegin(detail::GetValue(it)); i != end; ++i) {
    int numberValue;
    bool isNumber = detail::GetInt(*i, numberValue);
    if (!isNumber) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' property is not an integer type.\n";
          if (!parent_node.empty()) {
            (*err) += " in " + parent_node;
          }
          (*err) += ".\n";
        }
      }
      return false;
    }
    ret->push_back(numberValue);
  }

  return true;
}

static bool ParseStringProperty(
    std::string *ret, std::string *err, const detail::json &o,
    const std::string &property, bool required,
    const std::string &parent_node = std::string()) {
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (parent_node.empty()) {
          (*err) += ".\n";
        } else {
          (*err) += " in `" + parent_node + "'.\n";
        }
      }
    }
    return false;
  }

  std::string strValue;
  if (!detail::GetString(detail::GetValue(it), strValue)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a string type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = std::move(strValue);
  }

  return true;
}

static bool ParseStringIntegerProperty(std::map<std::string, int> *ret,
                                       std::string *err, const detail::json &o,
                                       const std::string &property,
                                       bool required,
                                       const std::string &parent = "") {
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      if (err) {
        if (!parent.empty()) {
          (*err) +=
              "'" + property + "' property is missing in " + parent + ".\n";
        } else {
          (*err) += "'" + property + "' property is missing.\n";
        }
      }
    }
    return false;
  }

  const detail::json &dict = detail::GetValue(it);

  // Make sure we are dealing with an object / dictionary.
  if (!detail::IsObject(dict)) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an object.\n";
      }
    }
    return false;
  }

  ret->clear();

  detail::json_const_iterator dictIt(detail::ObjectBegin(dict));
  detail::json_const_iterator dictItEnd(detail::ObjectEnd(dict));

  for (; dictIt != dictItEnd; ++dictIt) {
    int intVal;
    if (!detail::GetInt(detail::GetValue(dictIt), intVal)) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' value is not an integer type.\n";
        }
      }
      return false;
    }

    // Insert into the list.
    (*ret)[detail::GetKey(dictIt)] = intVal;
  }
  return true;
}

static bool ParseExtensionsProperty(ExtensionMap *ret, std::string *err,
                                    const detail::json &o) {
  (void)err;

  detail::json_const_iterator it;
  if (!detail::FindMember(o, "extensions", it)) {
    return false;
  }

  auto &obj = detail::GetValue(it);
  if (!detail::IsObject(obj)) {
    return false;
  }
  ExtensionMap extensions;
  detail::json_const_iterator extIt =
      detail::ObjectBegin(obj);  // it.value().begin();
  detail::json_const_iterator extEnd = detail::ObjectEnd(obj);
  for (; extIt != extEnd; ++extIt) {
    auto &itObj = detail::GetValue(extIt);
    if (!detail::IsObject(itObj)) continue;
    std::string key(detail::GetKey(extIt));
    if (!ParseJsonAsValue(&extensions[key], itObj)) {
      if (!key.empty()) {
        // create empty object so that an extension object is still of type
        // object
        extensions[key] = Value{Value::Object{}};
      }
    }
  }
  if (ret) {
    (*ret) = std::move(extensions);
  }
  return true;
}

template <typename GltfType>
static bool ParseExtrasAndExtensions(GltfType *target, std::string *err,
                                     const detail::json &o, bool store_json_strings)
{
  ParseExtensionsProperty(&target->extensions, err, o);
  ParseExtrasProperty(&target->extras, o);

  if (store_json_strings) {
    {
      detail::json_const_iterator it;
      if (detail::FindMember(o, "extensions", it)) {
        target->extensions_json_string =
            detail::JsonToString(detail::GetValue(it));
      }
    }
    {
      detail::json_const_iterator it;
      if (detail::FindMember(o, "extras", it)) {
        target->extras_json_string = detail::JsonToString(detail::GetValue(it));
      }
    }
  }
  return true;
}

static bool ParseAsset(Asset *asset, std::string *err, const detail::json &o,
                       bool store_original_json_for_extras_and_extensions) {
  ParseStringProperty(&asset->version, err, o, "version", true, "Asset");
  ParseStringProperty(&asset->generator, err, o, "generator", false, "Asset");
  ParseStringProperty(&asset->minVersion, err, o, "minVersion", false, "Asset");
  ParseStringProperty(&asset->copyright, err, o, "copyright", false, "Asset");
  return true;
}

static bool ParseBuffer(Buffer *buffer, std::string *err, const detail::json &o,
                        bool store_original_json_for_extras_and_extensions,
                        FsCallbacks *fs, const URICallbacks *uri_cb,
                        const std::string &basedir,
                        const size_t max_buffer_size, bool is_binary = false,
                        const unsigned char *bin_data = nullptr,
                        size_t bin_size = 0) {
  size_t byteLength;
  if (!ParseUnsignedProperty(&byteLength, err, o, "byteLength", true,
                             "Buffer")) {
    return false;
  }

  // In glTF 2.0, uri is not mandatory anymore
  buffer->uri.clear();
  ParseStringProperty(&buffer->uri, err, o, "uri", false, "Buffer");

  // having an empty uri for a non embedded image should not be valid
  if (!is_binary && buffer->uri.empty()) {
    if (err) {
      (*err) += "'uri' is missing from non binary glTF file buffer.\n";
    }
  }

  detail::json_const_iterator type;
  if (detail::FindMember(o, "type", type)) {
    std::string typeStr;
    if (detail::GetString(detail::GetValue(type), typeStr)) {
      if (typeStr.compare("arraybuffer") == 0) {
        // buffer.type = "arraybuffer";
      }
    }
  }

  if (is_binary) {
    // Still binary glTF accepts external dataURI.
    if (!buffer->uri.empty()) {
      // First try embedded data URI.
      if (IsDataURI(buffer->uri)) {
        std::string mime_type;
        if (!DecodeDataURI(&buffer->data, mime_type, buffer->uri, byteLength,
                           true)) {
          if (err) {
            (*err) +=
                "Failed to decode 'uri' : " + buffer->uri + " in Buffer\n";
          }
          return false;
        }
      } else {
        // External .bin file.
        std::string decoded_uri;
        if (!uri_cb->decode(buffer->uri, &decoded_uri, uri_cb->user_data)) {
          return false;
        }
        if (!LoadExternalFile(&buffer->data, err, /* warn */ nullptr,
                              decoded_uri, basedir, /* required */ true,
                              byteLength, /* checkSize */ true,
                              /* max_file_size */ max_buffer_size, fs)) {
          return false;
        }
      }
    } else {
      // load data from (embedded) binary data

      if ((bin_size == 0) || (bin_data == nullptr)) {
        if (err) {
          (*err) +=
              "Invalid binary data in `Buffer', or GLB with empty BIN chunk.\n";
        }
        return false;
      }

      if (byteLength > bin_size) {
        if (err) {
          std::stringstream ss;
          ss << "Invalid `byteLength'. Must be equal or less than binary size: "
                "`byteLength' = "
             << byteLength << ", binary size = " << bin_size << std::endl;
          (*err) += ss.str();
        }
        return false;
      }

      // Read buffer data
      buffer->data.resize(static_cast<size_t>(byteLength));
      memcpy(&(buffer->data.at(0)), bin_data, static_cast<size_t>(byteLength));
    }

  } else {
    if (IsDataURI(buffer->uri)) {
      std::string mime_type;
      if (!DecodeDataURI(&buffer->data, mime_type, buffer->uri, byteLength,
                         true)) {
        if (err) {
          (*err) += "Failed to decode 'uri' : " + buffer->uri + " in Buffer\n";
        }
        return false;
      }
    } else {
      // Assume external .bin file.
      std::string decoded_uri;
      if (!uri_cb->decode(buffer->uri, &decoded_uri, uri_cb->user_data)) {
        return false;
      }
      if (!LoadExternalFile(&buffer->data, err, /* warn */ nullptr, decoded_uri,
                            basedir, /* required */ true, byteLength,
                            /* checkSize */ true,
                            /* max file size */ max_buffer_size, fs)) {
        return false;
      }
    }
  }

  ParseStringProperty(&buffer->name, err, o, "name", false);

  ParseExtrasAndExtensions(buffer, err, o,
                           store_original_json_for_extras_and_extensions);

  return true;
}

static bool ParseBufferView(
    BufferView *bufferView, std::string *err, const detail::json &o,
    bool store_original_json_for_extras_and_extensions) {
  int buffer = -1;
  if (!ParseIntegerProperty(&buffer, err, o, "buffer", true, "BufferView")) {
    return false;
  }

  size_t byteOffset = 0;
  ParseUnsignedProperty(&byteOffset, err, o, "byteOffset", false);

  size_t byteLength = 1;
  if (!ParseUnsignedProperty(&byteLength, err, o, "byteLength", true,
                             "BufferView")) {
    return false;
  }

  size_t byteStride = 0;
  if (!ParseUnsignedProperty(&byteStride, err, o, "byteStride", false)) {
    // Spec says: When byteStride of referenced bufferView is not defined, it
    // means that accessor elements are tightly packed, i.e., effective stride
    // equals the size of the element.
    // We cannot determine the actual byteStride until Accessor are parsed, thus
    // set 0(= tightly packed) here(as done in OpenGL's VertexAttribPoiner)
    byteStride = 0;
  }

  if ((byteStride > 252) || ((byteStride % 4) != 0)) {
    if (err) {
      std::stringstream ss;
      ss << "Invalid `byteStride' value. `byteStride' must be the multiple of "
            "4 : "
         << byteStride << std::endl;

      (*err) += ss.str();
    }
    return false;
  }

  int target = 0;
  ParseIntegerProperty(&target, err, o, "target", false);
  if ((target == TINYGLTF_TARGET_ARRAY_BUFFER) ||
      (target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)) {
    // OK
  } else {
    target = 0;
  }
  bufferView->target = target;

  ParseStringProperty(&bufferView->name, err, o, "name", false);

  ParseExtrasAndExtensions(bufferView, err, o,
                           store_original_json_for_extras_and_extensions);

  bufferView->buffer = buffer;
  bufferView->byteOffset = byteOffset;
  bufferView->byteLength = byteLength;
  bufferView->byteStride = byteStride;
  return true;
}

static bool ParseSparseAccessor(
    Accessor::Sparse *sparse, std::string *err, const detail::json &o,
    bool store_original_json_for_extras_and_extensions) {
  sparse->isSparse = true;

  int count = 0;
  if (!ParseIntegerProperty(&count, err, o, "count", true, "SparseAccessor")) {
    return false;
  }

  ParseExtrasAndExtensions(sparse, err, o,
                           store_original_json_for_extras_and_extensions);

  detail::json_const_iterator indices_iterator;
  detail::json_const_iterator values_iterator;
  if (!detail::FindMember(o, "indices", indices_iterator)) {
    (*err) = "the sparse object of this accessor doesn't have indices";
    return false;
  }

  if (!detail::FindMember(o, "values", values_iterator)) {
    (*err) = "the sparse object of this accessor doesn't have values";
    return false;
  }

  const detail::json &indices_obj = detail::GetValue(indices_iterator);
  const detail::json &values_obj = detail::GetValue(values_iterator);

  int indices_buffer_view = 0, indices_byte_offset = 0, component_type = 0;
  if (!ParseIntegerProperty(&indices_buffer_view, err, indices_obj,
                            "bufferView", true, "SparseAccessor")) {
    return false;
  }
  ParseIntegerProperty(&indices_byte_offset, err, indices_obj, "byteOffset",
                       false);
  if (!ParseIntegerProperty(&component_type, err, indices_obj, "componentType",
                            true, "SparseAccessor")) {
    return false;
  }

  int values_buffer_view = 0, values_byte_offset = 0;
  if (!ParseIntegerProperty(&values_buffer_view, err, values_obj, "bufferView",
                            true, "SparseAccessor")) {
    return false;
  }
  ParseIntegerProperty(&values_byte_offset, err, values_obj, "byteOffset",
                       false);

  sparse->count = count;
  sparse->indices.bufferView = indices_buffer_view;
  sparse->indices.byteOffset = indices_byte_offset;
  sparse->indices.componentType = component_type;
  ParseExtrasAndExtensions(&sparse->indices, err, indices_obj,
                           store_original_json_for_extras_and_extensions);

  sparse->values.bufferView = values_buffer_view;
  sparse->values.byteOffset = values_byte_offset;
  ParseExtrasAndExtensions(&sparse->values, err, values_obj,
                           store_original_json_for_extras_and_extensions);

  return true;
}

static bool ParseAccessor(Accessor *accessor, std::string *err, const detail::json &o,
                          bool store_original_json_for_extras_and_extensions)
{
  int bufferView = -1;
  ParseIntegerProperty(&bufferView, err, o, "bufferView", false, "Accessor");

  size_t byteOffset = 0;
  ParseUnsignedProperty(&byteOffset, err, o, "byteOffset", false, "Accessor");

  bool normalized = false;
  ParseBooleanProperty(&normalized, err, o, "normalized", false, "Accessor");

  size_t componentType = 0;
  if (!ParseUnsignedProperty(&componentType, err, o, "componentType", true,
                             "Accessor")) {
    return false;
  }

  size_t count = 0;
  if (!ParseUnsignedProperty(&count, err, o, "count", true, "Accessor")) {
    return false;
  }

  std::string type;
  if (!ParseStringProperty(&type, err, o, "type", true, "Accessor")) {
    return false;
  }

  if (type.compare("SCALAR") == 0) {
    accessor->type = TINYGLTF_TYPE_SCALAR;
  } else if (type.compare("VEC2") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC2;
  } else if (type.compare("VEC3") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC3;
  } else if (type.compare("VEC4") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC4;
  } else if (type.compare("MAT2") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT2;
  } else if (type.compare("MAT3") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT3;
  } else if (type.compare("MAT4") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT4;
  } else {
    std::stringstream ss;
    ss << "Unsupported `type` for accessor object. Got \"" << type << "\"\n";
    if (err) {
      (*err) += ss.str();
    }
    return false;
  }

  ParseStringProperty(&accessor->name, err, o, "name", false);

  accessor->minValues.clear();
  accessor->maxValues.clear();
  ParseNumberArrayProperty(&accessor->minValues, err, o, "min", false,
                           "Accessor");

  ParseNumberArrayProperty(&accessor->maxValues, err, o, "max", false,
                           "Accessor");

  accessor->count = count;
  accessor->bufferView = bufferView;
  accessor->byteOffset = byteOffset;
  accessor->normalized = normalized;
  {
    if (componentType >= TINYGLTF_COMPONENT_TYPE_BYTE &&
        componentType <= TINYGLTF_COMPONENT_TYPE_DOUBLE) {
      // OK
      accessor->componentType = int(componentType);
    } else {
      std::stringstream ss;
      ss << "Invalid `componentType` in accessor. Got " << componentType
         << "\n";
      if (err) {
        (*err) += ss.str();
      }
      return false;
    }
  }

  ParseExtrasAndExtensions(accessor, err, o,
                           store_original_json_for_extras_and_extensions);

  // check if accessor has a "sparse" object:
  detail::json_const_iterator iterator;
  if (detail::FindMember(o, "sparse", iterator)) {
    // here this accessor has a "sparse" subobject
    return ParseSparseAccessor(&accessor->sparse, err,
                               detail::GetValue(iterator),
                               store_original_json_for_extras_and_extensions);
  }

  return true;
}

static bool ParsePrimitive(Primitive *primitive, Model *model, std::string *err,
                           const detail::json &o,
                           bool store_original_json_for_extras_and_extensions) {
  int material = -1;
  ParseIntegerProperty(&material, err, o, "material", false);
  primitive->material = material;

  int mode = TINYGLTF_MODE_TRIANGLES;
  ParseIntegerProperty(&mode, err, o, "mode", false);
  primitive->mode = mode;  // Why only triangles were supported ?

  int indices = -1;
  ParseIntegerProperty(&indices, err, o, "indices", false);
  primitive->indices = indices;
  if (!ParseStringIntegerProperty(&primitive->attributes, err, o, "attributes",
                                  true, "Primitive")) {
    return false;
  }

  // Look for morph targets
  detail::json_const_iterator targetsObject;
  if (detail::FindMember(o, "targets", targetsObject) &&
      detail::IsArray(detail::GetValue(targetsObject))) {
    auto targetsObjectEnd = detail::ArrayEnd(detail::GetValue(targetsObject));
    for (detail::json_const_array_iterator i =
             detail::ArrayBegin(detail::GetValue(targetsObject));
         i != targetsObjectEnd; ++i) {
      std::map<std::string, int> targetAttribues;

      const detail::json &dict = *i;
      if (detail::IsObject(dict)) {
        detail::json_const_iterator dictIt(detail::ObjectBegin(dict));
        detail::json_const_iterator dictItEnd(detail::ObjectEnd(dict));

        for (; dictIt != dictItEnd; ++dictIt) {
          int iVal;
          if (detail::GetInt(detail::GetValue(dictIt), iVal))
            targetAttribues[detail::GetKey(dictIt)] = iVal;
        }
        primitive->targets.emplace_back(std::move(targetAttribues));
      }
    }
  }

  ParseExtrasAndExtensions(primitive, err, o,
                           store_original_json_for_extras_and_extensions);


  (void)model;

  return true;
}

static bool ParseMesh(Mesh *mesh, Model *model, std::string *err, const detail::json &o,
                      bool store_original_json_for_extras_and_extensions)
{
  ParseStringProperty(&mesh->name, err, o, "name", false);

  mesh->primitives.clear();
  detail::json_const_iterator primObject;
  if (detail::FindMember(o, "primitives", primObject) &&
      detail::IsArray(detail::GetValue(primObject))) {
    detail::json_const_array_iterator primEnd =
        detail::ArrayEnd(detail::GetValue(primObject));
    for (detail::json_const_array_iterator i =
             detail::ArrayBegin(detail::GetValue(primObject));
         i != primEnd; ++i) {
      Primitive primitive;
      if (ParsePrimitive(&primitive, model, err, *i,
                         store_original_json_for_extras_and_extensions)) {
        // Only add the primitive if the parsing succeeds.
        mesh->primitives.emplace_back(std::move(primitive));
      }
    }
  }

  // Should probably check if has targets and if dimensions fit
  ParseNumberArrayProperty(&mesh->weights, err, o, "weights", false);

  ParseExtrasAndExtensions(mesh, err, o,
                           store_original_json_for_extras_and_extensions);

  return true;
}

static bool ParseNode(Node *node, std::string *err, const detail::json &o,
                      bool store_original_json_for_extras_and_extensions) {
  ParseStringProperty(&node->name, err, o, "name", false);

  int skin = -1;
  ParseIntegerProperty(&skin, err, o, "skin", false);
  node->skin = skin;

  // Matrix and T/R/S are exclusive
  if (!ParseNumberArrayProperty(&node->matrix, err, o, "matrix", false)) {
    ParseNumberArrayProperty(&node->rotation, err, o, "rotation", false);
    ParseNumberArrayProperty(&node->scale, err, o, "scale", false);
    ParseNumberArrayProperty(&node->translation, err, o, "translation", false);
  }

  int camera = -1;
  ParseIntegerProperty(&camera, err, o, "camera", false);
  node->camera = camera;

  int mesh = -1;
  ParseIntegerProperty(&mesh, err, o, "mesh", false);
  node->mesh = mesh;

  node->children.clear();
  ParseIntegerArrayProperty(&node->children, err, o, "children", false);

  ParseNumberArrayProperty(&node->weights, err, o, "weights", false);

  ParseExtrasAndExtensions(node, err, o,
                           store_original_json_for_extras_and_extensions);

  // KHR_lights_punctual: parse light source reference
  int light = -1;
  if (node->extensions.count("KHR_lights_punctual") != 0) {
    auto const &light_ext = node->extensions["KHR_lights_punctual"];
    if (light_ext.Has("light")) {
      light = light_ext.Get("light").GetNumberAsInt();
    } else {
      if (err) {
        *err +=
            "Node has extension KHR_lights_punctual, but does not reference "
            "a light source.\n";
      }
      return false;
    }
  }
  node->light = light;

  // KHR_audio: parse audio source reference
  int emitter = -1;
  if (node->extensions.count("KHR_audio") != 0) {
    auto const &audio_ext = node->extensions["KHR_audio"];
    if (audio_ext.Has("emitter")) {
      emitter = audio_ext.Get("emitter").GetNumberAsInt();
    } else {
      if (err) {
        *err +=
            "Node has extension KHR_audio, but does not reference "
            "a audio emitter.\n";
      }
      return false;
    }
  }
  node->emitter = emitter;

  return true;
}

static bool ParseScene(Scene *scene, std::string *err, const detail::json &o,
                       bool store_original_json_for_extras_and_extensions) {
  ParseStringProperty(&scene->name, err, o, "name", false);
  ParseIntegerArrayProperty(&scene->nodes, err, o, "nodes", false);

  ParseExtrasAndExtensions(scene, err, o,
                           store_original_json_for_extras_and_extensions);

  return true;
}

static bool ParsePerspectiveCamera(
    PerspectiveCamera *camera, std::string *err, const detail::json &o,
    bool store_original_json_for_extras_and_extensions) {
  double yfov = 0.0;
  if (!ParseNumberProperty(&yfov, err, o, "yfov", true, "OrthographicCamera")) {
    return false;
  }

  double znear = 0.0;
  if (!ParseNumberProperty(&znear, err, o, "znear", true,
                           "PerspectiveCamera")) {
    return false;
  }

  double aspectRatio = 0.0;  // = invalid
  ParseNumberProperty(&aspectRatio, err, o, "aspectRatio", false,
                      "PerspectiveCamera");

  double zfar = 0.0;  // = invalid
  ParseNumberProperty(&zfar, err, o, "zfar", false, "PerspectiveCamera");

  camera->aspectRatio = aspectRatio;
  camera->zfar = zfar;
  camera->yfov = yfov;
  camera->znear = znear;

  ParseExtrasAndExtensions(camera, err, o,
                           store_original_json_for_extras_and_extensions);

  // TODO(syoyo): Validate parameter values.

  return true;
}

static bool ParseSpotLight(SpotLight *light, std::string *err,
                           const detail::json &o,
                           bool store_original_json_for_extras_and_extensions) {
  ParseNumberProperty(&light->innerConeAngle, err, o, "innerConeAngle", false);
  ParseNumberProperty(&light->outerConeAngle, err, o, "outerConeAngle", false);

  ParseExtrasAndExtensions(light, err, o,
                           store_original_json_for_extras_and_extensions);

  // TODO(syoyo): Validate parameter values.

  return true;
}

static bool ParseOrthographicCamera(
    OrthographicCamera *camera, std::string *err, const detail::json &o,
    bool store_original_json_for_extras_and_extensions) {
  double xmag = 0.0;
  if (!ParseNumberProperty(&xmag, err, o, "xmag", true, "OrthographicCamera")) {
    return false;
  }

  double ymag = 0.0;
  if (!ParseNumberProperty(&ymag, err, o, "ymag", true, "OrthographicCamera")) {
    return false;
  }

  double zfar = 0.0;
  if (!ParseNumberProperty(&zfar, err, o, "zfar", true, "OrthographicCamera")) {
    return false;
  }

  double znear = 0.0;
  if (!ParseNumberProperty(&znear, err, o, "znear", true,
                           "OrthographicCamera")) {
    return false;
  }

  ParseExtrasAndExtensions(camera, err, o,
                           store_original_json_for_extras_and_extensions);

  camera->xmag = xmag;
  camera->ymag = ymag;
  camera->zfar = zfar;
  camera->znear = znear;

  // TODO(syoyo): Validate parameter values.

  return true;
}

static bool ParseCamera(Camera *camera, std::string *err, const detail::json &o,
                        bool store_original_json_for_extras_and_extensions) {
  if (!ParseStringProperty(&camera->type, err, o, "type", true, "Camera")) {
    return false;
  }

  if (camera->type.compare("orthographic") == 0) {
    detail::json_const_iterator orthoIt;
    if (!detail::FindMember(o, "orthographic", orthoIt)) {
      if (err) {
        std::stringstream ss;
        ss << "Orthographic camera description not found." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    const detail::json &v = detail::GetValue(orthoIt);
    if (!detail::IsObject(v)) {
      if (err) {
        std::stringstream ss;
        ss << "\"orthographic\" is not a JSON object." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    if (!ParseOrthographicCamera(
            &camera->orthographic, err, v,
            store_original_json_for_extras_and_extensions)) {
      return false;
    }
  } else if (camera->type.compare("perspective") == 0) {
    detail::json_const_iterator perspIt;
    if (!detail::FindMember(o, "perspective", perspIt)) {
      if (err) {
        std::stringstream ss;
        ss << "Perspective camera description not found." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    const detail::json &v = detail::GetValue(perspIt);
    if (!detail::IsObject(v)) {
      if (err) {
        std::stringstream ss;
        ss << "\"perspective\" is not a JSON object." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    if (!ParsePerspectiveCamera(
            &camera->perspective, err, v,
            store_original_json_for_extras_and_extensions)) {
      return false;
    }
  } else {
    if (err) {
      std::stringstream ss;
      ss << "Invalid camera type: \"" << camera->type
         << "\". Must be \"perspective\" or \"orthographic\"" << std::endl;
      (*err) += ss.str();
    }
    return false;
  }

  ParseStringProperty(&camera->name, err, o, "name", false);

  ParseExtrasAndExtensions(camera, err, o,
                           store_original_json_for_extras_and_extensions);

  return true;
}

static bool ParseLight(Light *light, std::string *err, const detail::json &o,
                       bool store_original_json_for_extras_and_extensions) {
  if (!ParseStringProperty(&light->type, err, o, "type", true)) {
    return false;
  }

  if (light->type == "spot") {
    detail::json_const_iterator spotIt;
    if (!detail::FindMember(o, "spot", spotIt)) {
      if (err) {
        std::stringstream ss;
        ss << "Spot light description not found." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    const detail::json &v = detail::GetValue(spotIt);
    if (!detail::IsObject(v)) {
      if (err) {
        std::stringstream ss;
        ss << "\"spot\" is not a JSON object." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    if (!ParseSpotLight(&light->spot, err, v,
                        store_original_json_for_extras_and_extensions)) {
      return false;
    }
  }

  ParseStringProperty(&light->name, err, o, "name", false);
  ParseNumberArrayProperty(&light->color, err, o, "color", false);
  ParseNumberProperty(&light->range, err, o, "range", false);
  ParseNumberProperty(&light->intensity, err, o, "intensity", false);

  ParseExtrasAndExtensions(light, err, o,
                           store_original_json_for_extras_and_extensions);

  return true;
}

namespace detail {

template <typename Callback>
bool ForEachInArray(const detail::json &_v, const char *member, Callback &&cb) {
  detail::json_const_iterator itm;
  if (detail::FindMember(_v, member, itm) &&
      detail::IsArray(detail::GetValue(itm))) {
    const detail::json &root = detail::GetValue(itm);
    auto it = detail::ArrayBegin(root);
    auto end = detail::ArrayEnd(root);
    for (; it != end; ++it) {
      if (!cb(*it)) return false;
    }
  }
  return true;
};

}  // end of namespace detail

bool TinyGLTF::LoadFromString(Model *model, std::string *err, std::string *warn,
                              const char *json_str,
                              unsigned int json_str_length,
                              const std::string &base_dir,
                              unsigned int check_sections) {
  if (json_str_length < 4) {
    if (err) {
      (*err) = "JSON string too short.\n";
    }
    return false;
  }

  detail::JsonDocument v;

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || \
     defined(_CPPUNWIND)) &&                               \
    !defined(TINYGLTF_NOEXCEPTION)
  try {
    detail::JsonParse(v, json_str, json_str_length, true);

  } catch (const std::exception &e) {
    if (err) {
      (*err) = e.what();
    }
    return false;
  }
#else
  {
    detail::JsonParse(v, json_str, json_str_length);

    if (!detail::IsObject(v)) {
      // Assume parsing was failed.
      if (err) {
        (*err) = "Failed to parse JSON object\n";
      }
      return false;
    }
  }
#endif

  if (!detail::IsObject(v)) {
    // root is not an object.
    if (err) {
      (*err) = "Root element is not a JSON object\n";
    }
    return false;
  }

  {
    bool version_found = false;
    detail::json_const_iterator it;
    if (detail::FindMember(v, "asset", it) &&
        detail::IsObject(detail::GetValue(it))) {
      auto &itObj = detail::GetValue(it);
      detail::json_const_iterator version_it;
      std::string versionStr;
      if (detail::FindMember(itObj, "version", version_it) &&
          detail::GetString(detail::GetValue(version_it), versionStr)) {
        version_found = true;
      }
    }
    if (version_found) {
      // OK
    } else if (check_sections & REQUIRE_VERSION) {
      if (err) {
        (*err) += "\"asset\" object not found in .gltf or not an object type\n";
      }
      return false;
    }
  }

  // scene is not mandatory.
  // FIXME Maybe a better way to handle it than removing the code

  auto IsArrayMemberPresent = [](const detail::json &_v,
                                 const char *name) -> bool {
    detail::json_const_iterator it;
    return detail::FindMember(_v, name, it) &&
           detail::IsArray(detail::GetValue(it));
  };

  {
    if ((check_sections & REQUIRE_SCENES) &&
        !IsArrayMemberPresent(v, "scenes")) {
      if (err) {
        (*err) += "\"scenes\" object not found in .gltf or not an array type\n";
      }
      return false;
    }
  }

  {
    if ((check_sections & REQUIRE_NODES) && !IsArrayMemberPresent(v, "nodes")) {
      if (err) {
        (*err) += "\"nodes\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    if ((check_sections & REQUIRE_ACCESSORS) &&
        !IsArrayMemberPresent(v, "accessors")) {
      if (err) {
        (*err) += "\"accessors\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    if ((check_sections & REQUIRE_BUFFERS) &&
        !IsArrayMemberPresent(v, "buffers")) {
      if (err) {
        (*err) += "\"buffers\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    if ((check_sections & REQUIRE_BUFFER_VIEWS) &&
        !IsArrayMemberPresent(v, "bufferViews")) {
      if (err) {
        (*err) += "\"bufferViews\" object not found in .gltf\n";
      }
      return false;
    }
  }

  model->buffers.clear();
  model->bufferViews.clear();
  model->accessors.clear();
  model->meshes.clear();
  model->cameras.clear();
  model->nodes.clear();
  model->extensionsUsed.clear();
  model->extensionsRequired.clear();
  model->extensions.clear();
  model->defaultScene = -1;

  // 1. Parse Asset
  {
    detail::json_const_iterator it;
    if (detail::FindMember(v, "asset", it) &&
        detail::IsObject(detail::GetValue(it))) {
      const detail::json &root = detail::GetValue(it);

      ParseAsset(&model->asset, err, root,
                 store_original_json_for_extras_and_extensions_);
    }
  }

  using detail::ForEachInArray;

  // 2. Parse extensionUsed
  {
    ForEachInArray(v, "extensionsUsed", [&](const detail::json &o) {
      std::string str;
      detail::GetString(o, str);
      model->extensionsUsed.emplace_back(std::move(str));
      return true;
    });
  }

  {
    ForEachInArray(v, "extensionsRequired", [&](const detail::json &o) {
      std::string str;
      detail::GetString(o, str);
      model->extensionsRequired.emplace_back(std::move(str));
      return true;
    });
  }

  // 3. Parse Buffer
  {
    bool success = ForEachInArray(v, "buffers", [&](const detail::json &o) {
      if (!detail::IsObject(o)) {
        if (err) {
          (*err) += "`buffers' does not contain an JSON object.";
        }
        return false;
      }
      Buffer buffer;
      if (!ParseBuffer(&buffer, err, o,
                       store_original_json_for_extras_and_extensions_, &fs,
                       &uri_cb, base_dir, max_external_file_size_, is_binary_,
                       bin_data_, bin_size_)) {
        return false;
      }

      model->buffers.emplace_back(std::move(buffer));
      return true;
    });

    if (!success) {
      return false;
    }
  }
  // 4. Parse BufferView
  {
    bool success = ForEachInArray(v, "bufferViews", [&](const detail::json &o) {
      if (!detail::IsObject(o)) {
        if (err) {
          (*err) += "`bufferViews' does not contain an JSON object.";
        }
        return false;
      }
      BufferView bufferView;
      if (!ParseBufferView(&bufferView, err, o,
                           store_original_json_for_extras_and_extensions_)) {
        return false;
      }

      model->bufferViews.emplace_back(std::move(bufferView));
      return true;
    });

    if (!success) {
      return false;
    }
  }

  // 5. Parse Accessor
  {
    bool success = ForEachInArray(v, "accessors", [&](const detail::json &o) {
      if (!detail::IsObject(o)) {
        if (err) {
          (*err) += "`accessors' does not contain an JSON object.";
        }
        return false;
      }
      Accessor accessor;
      if (!ParseAccessor(&accessor, err, o,
                         store_original_json_for_extras_and_extensions_)) {
        return false;
      }

      model->accessors.emplace_back(std::move(accessor));
      return true;
    });

    if (!success) {
      return false;
    }
  }

  // 6. Parse Mesh
  {
    bool success = ForEachInArray(v, "meshes", [&](const detail::json &o) {
      if (!detail::IsObject(o)) {
        if (err) {
          (*err) += "`meshes' does not contain an JSON object.";
        }
        return false;
      }
      Mesh mesh;
      if (!ParseMesh(&mesh, model, err, o,
                     store_original_json_for_extras_and_extensions_)) {
        return false;
      }

      model->meshes.emplace_back(std::move(mesh));
      return true;
    });

    if (!success) {
      return false;
    }
  }

  // Assign missing bufferView target types
  // - Look for missing Mesh indices
  // - Look for missing Mesh attributes
  for (auto &mesh : model->meshes) {
    for (auto &primitive : mesh.primitives) {
      if (primitive.indices >
          -1)  // has indices from parsing step, must be Element Array Buffer
      {
        if (size_t(primitive.indices) >= model->accessors.size()) {
          if (err) {
            (*err) += "primitive indices accessor out of bounds";
          }
          return false;
        }

        auto bufferView =
            model->accessors[size_t(primitive.indices)].bufferView;
        if (bufferView < 0 || size_t(bufferView) >= model->bufferViews.size()) {
          if (err) {
            (*err) += "accessor[" + std::to_string(primitive.indices) +
                      "] invalid bufferView";
          }
          return false;
        }

        model->bufferViews[size_t(bufferView)].target =
            TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        // we could optionally check if accessors' bufferView type is Scalar, as
        // it should be
      }

      for (auto &attribute : primitive.attributes) {
        const auto accessorsIndex = size_t(attribute.second);
        if (accessorsIndex < model->accessors.size()) {
          const auto bufferView = model->accessors[accessorsIndex].bufferView;
          // bufferView could be null(-1) for sparse morph target
          if (bufferView >= 0 && bufferView < (int)model->bufferViews.size()) {
            model->bufferViews[size_t(bufferView)].target =
                TINYGLTF_TARGET_ARRAY_BUFFER;
          }
        }
      }

      for (auto &target : primitive.targets) {
        for (auto &attribute : target) {
          const auto accessorsIndex = size_t(attribute.second);
          if (accessorsIndex < model->accessors.size()) {
            const auto bufferView = model->accessors[accessorsIndex].bufferView;
            // bufferView could be null(-1) for sparse morph target
            if (bufferView >= 0 &&
                bufferView < (int)model->bufferViews.size()) {
              model->bufferViews[size_t(bufferView)].target =
                  TINYGLTF_TARGET_ARRAY_BUFFER;
            }
          }
        }
      }
    }
  }

  // 7. Parse Node
  {
    bool success = ForEachInArray(v, "nodes", [&](const detail::json &o) {
      if (!detail::IsObject(o)) {
        if (err) {
          (*err) += "`nodes' does not contain an JSON object.";
        }
        return false;
      }
      Node node;
      if (!ParseNode(&node, err, o,
                     store_original_json_for_extras_and_extensions_)) {
        return false;
      }

      model->nodes.emplace_back(std::move(node));
      return true;
    });

    if (!success) {
      return false;
    }
  }

  // 8. Parse scenes.
  {
    bool success = ForEachInArray(v, "scenes", [&](const detail::json &o) {
      if (!detail::IsObject(o)) {
        if (err) {
          (*err) += "`scenes' does not contain an JSON object.";
        }
        return false;
      }

      Scene scene;
      if (!ParseScene(&scene, err, o,
                      store_original_json_for_extras_and_extensions_)) {
        return false;
      }

      model->scenes.emplace_back(std::move(scene));
      return true;
    });

    if (!success) {
      return false;
    }
  }

  // 9. Parse default scenes.
  {
    detail::json_const_iterator rootIt;
    int iVal;
    if (detail::FindMember(v, "scene", rootIt) &&
        detail::GetInt(detail::GetValue(rootIt), iVal)) {
      model->defaultScene = iVal;
    }
  }

  // 16. Parse Camera
  {
    bool success = ForEachInArray(v, "cameras", [&](const detail::json &o) {
      if (!detail::IsObject(o)) {
        if (err) {
          (*err) += "`cameras' does not contain an JSON object.";
        }
        return false;
      }
      Camera camera;
      if (!ParseCamera(&camera, err, o,
                       store_original_json_for_extras_and_extensions_)) {
        return false;
      }

      model->cameras.emplace_back(std::move(camera));
      return true;
    });

    if (!success) {
      return false;
    }
  }

  return true;
}

bool TinyGLTF::LoadASCIIFromString(Model *model, std::string *err,
                                   std::string *warn, const char *str,
                                   unsigned int length,
                                   const std::string &base_dir,
                                   unsigned int check_sections) {
  is_binary_ = false;
  bin_data_ = nullptr;
  bin_size_ = 0;

  return LoadFromString(model, err, warn, str, length, base_dir,
                        check_sections);
}

bool TinyGLTF::LoadASCIIFromFile(Model *model, std::string *err,
                                 std::string *warn, const std::string &filename,
                                 unsigned int check_sections) {
  std::stringstream ss;

  if (fs.ReadWholeFile == nullptr) {
    // Programmer error, assert() ?
    ss << "Failed to read file: " << filename
       << ": one or more FS callback not set" << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = fs.ReadWholeFile(&data, &fileerr, filename, fs.user_data);
  if (!fileread) {
    ss << "Failed to read file: " << filename << ": " << fileerr << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  size_t sz = data.size();
  if (sz == 0) {
    if (err) {
      (*err) = "Empty file.";
    }
    return false;
  }

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadASCIIFromString(
      model, err, warn, reinterpret_cast<const char *>(&data.at(0)),
      static_cast<unsigned int>(data.size()), basedir, check_sections);

  return ret;
}

bool TinyGLTF::LoadBinaryFromMemory(Model *model, std::string *err,
                                    std::string *warn,
                                    const unsigned char *bytes,
                                    unsigned int size,
                                    const std::string &base_dir,
                                    unsigned int check_sections) {
  if (size < 20) {
    if (err) {
      (*err) = "Too short data size for glTF Binary.";
    }
    return false;
  }

  if (bytes[0] == 'g' && bytes[1] == 'l' && bytes[2] == 'T' &&
      bytes[3] == 'F') {
    // ok
  } else {
    if (err) {
      (*err) = "Invalid magic.";
    }
    return false;
  }

  unsigned int version;        // 4 bytes
  unsigned int length;         // 4 bytes
  unsigned int chunk0_length;  // 4 bytes
  unsigned int chunk0_format;  // 4 bytes;

  memcpy(&version, bytes + 4, 4);
  swap4(&version);
  memcpy(&length, bytes + 8, 4);
  swap4(&length);
  memcpy(&chunk0_length, bytes + 12, 4);  // JSON data length
  swap4(&chunk0_length);
  memcpy(&chunk0_format, bytes + 16, 4);
  swap4(&chunk0_format);

  // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#binary-gltf-layout
  //
  // In case the Bin buffer is not present, the size is exactly 20 + size of
  // JSON contents,
  // so use "greater than" operator.
  //
  // https://github.com/syoyo/tinygltf/issues/372
  // Use 64bit uint to avoid integer overflow.
  uint64_t header_and_json_size = 20ull + uint64_t(chunk0_length);

  if (header_and_json_size > std::numeric_limits<uint32_t>::max()) {
    // Do not allow 4GB or more GLB data.
    (*err) = "Invalid glTF binary. GLB data exceeds 4GB.";
  }

  if ((header_and_json_size > uint64_t(size)) || (chunk0_length < 1) ||
      (length > size) || (header_and_json_size > uint64_t(length)) ||
      (chunk0_format != 0x4E4F534A)) {  // 0x4E4F534A = JSON format.
    if (err) {
      (*err) = "Invalid glTF binary.";
    }
    return false;
  }

  // Padding check
  // The start and the end of each chunk must be aligned to a 4-byte boundary.
  // No padding check for chunk0 start since its 4byte-boundary is ensured.
  if ((header_and_json_size % 4) != 0) {
    if (err) {
      (*err) = "JSON Chunk end does not aligned to a 4-byte boundary.";
    }
  }

  // std::cout << "header_and_json_size = " << header_and_json_size << "\n";
  // std::cout << "length = " << length << "\n";

  // Chunk1(BIN) data
  // The spec says: When the binary buffer is empty or when it is stored by
  // other means, this chunk SHOULD be omitted. So when header + JSON data ==
  // binary size, Chunk1 is omitted.
  if (header_and_json_size == uint64_t(length)) {
    bin_data_ = nullptr;
    bin_size_ = 0;
  } else {
    // Read Chunk1 info(BIN data)
    // At least Chunk1 should have 12 bytes(8 bytes(header) + 4 bytes(bin
    // payload could be 1~3 bytes, but need to be aligned to 4 bytes)
    if ((header_and_json_size + 12ull) > uint64_t(length)) {
      if (err) {
        (*err) =
            "Insufficient storage space for Chunk1(BIN data). At least Chunk1 "
            "Must have 4 or more bytes, but got " +
            std::to_string((header_and_json_size + 8ull) - uint64_t(length)) +
            ".\n";
      }
      return false;
    }

    unsigned int chunk1_length;  // 4 bytes
    unsigned int chunk1_format;  // 4 bytes;
    memcpy(&chunk1_length, bytes + header_and_json_size,
           4);  // JSON data length
    swap4(&chunk1_length);
    memcpy(&chunk1_format, bytes + header_and_json_size + 4, 4);
    swap4(&chunk1_format);

    // std::cout << "chunk1_length = " << chunk1_length << "\n";

    if (chunk1_length < 4) {
      if (err) {
        (*err) = "Insufficient Chunk1(BIN) data size.";
      }
      return false;
    }

    if ((chunk1_length % 4) != 0) {
      if (err) {
        (*err) = "BIN Chunk end does not aligned to a 4-byte boundary.";
      }
      return false;
    }

    if (uint64_t(chunk1_length) + header_and_json_size > uint64_t(length)) {
      if (err) {
        (*err) = "BIN Chunk data length exceeds the GLB size.";
      }
      return false;
    }

    if (chunk1_format != 0x004e4942) {
      if (err) {
        (*err) = "Invalid type for chunk1 data.";
      }
      return false;
    }

    // std::cout << "chunk1_length = " << chunk1_length << "\n";

    bin_data_ = bytes + header_and_json_size +
                8;  // 4 bytes (bin_buffer_length) + 4 bytes(bin_buffer_format)

    bin_size_ = size_t(chunk1_length);
  }

  // Extract JSON string.
  std::string jsonString(reinterpret_cast<const char *>(&bytes[20]),
                         chunk0_length);

  is_binary_ = true;

  bool ret = LoadFromString(model, err, warn,
                            reinterpret_cast<const char *>(&bytes[20]),
                            chunk0_length, base_dir, check_sections);
  if (!ret) {
    return ret;
  }

  return true;
}

bool TinyGLTF::LoadBinaryFromFile(Model *model, std::string *err,
                                  std::string *warn,
                                  const std::string &filename,
                                  unsigned int check_sections) {
  std::stringstream ss;

  if (fs.ReadWholeFile == nullptr) {
    // Programmer error, assert() ?
    ss << "Failed to read file: " << filename
       << ": one or more FS callback not set" << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = fs.ReadWholeFile(&data, &fileerr, filename, fs.user_data);
  if (!fileread) {
    ss << "Failed to read file: " << filename << ": " << fileerr << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadBinaryFromMemory(model, err, warn, &data.at(0),
                                  static_cast<unsigned int>(data.size()),
                                  basedir, check_sections);

  return ret;
}

namespace detail {
detail::json JsonFromString(const char *s) {
  return detail::json(s);
}

void JsonAssign(detail::json &dest, const detail::json &src) {
  dest = src;
}

void JsonAddMember(detail::json &o, const char *key, detail::json &&value) {
  o[key] = std::move(value);
}

void JsonPushBack(detail::json &o, detail::json &&value) {
  o.push_back(std::move(value));
}

void JsonSetObject(detail::json &o) {
  o = o.object({});
}

void JsonReserveArray(detail::json &o, size_t s) {
  (void)(o);
  (void)(s);
}
}  // namespace detail

}  // namespace tinygltf

#endif  // TINYGLTF_IMPLEMENTATION
