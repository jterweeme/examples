#include "tinygltf.h"
#include "json.h"
#include <algorithm>
#include <sys/stat.h>  // for is_directory check
#include <cstdio>
#include <fstream>
#include <sstream>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TINYGLTF_LITTLE_ENDIAN 1
#endif

namespace tinygltf {

bool Value::Has(const std::string &key) const
{
    if (!IsObject()) return false;
    Object::const_iterator it = object_value_.find(key);
    return (it != object_value_.end()) ? true : false;
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

static std::string JoinPath(const std::string &path0, const std::string &path1)
{
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
                            const std::string &filepath, FsCallbacks *fs)
{
  if (fs == nullptr || fs->ExpandFilePath == nullptr ||
      fs->FileExists == nullptr) {
    // Error, fs callback[s] missing
    return std::string();
  }

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

std::string base64_decode(std::string const &s);

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
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

inline uint8_t from_hex(uint8_t ch) {
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
      const uint8_t ch1 = from_hex(static_cast<unsigned char>(str[i + 1]));
      const uint8_t ch2 = from_hex(static_cast<unsigned char>(str[i + 2]));
      const uint8_t ch = static_cast<uint8_t>((ch1 << 4) | ch2);
      result += static_cast<char>(ch);
      i += 2;
    } else {
      result += str[i];
    }
  }
  return result;
}

}  // namespace dlib

bool URIDecode(const std::string &in_uri, std::string *out_uri, void *user_data)
{
    (void)user_data;
    *out_uri = dlib::urldecode(in_uri);
    return true;
}

static bool LoadExternalFile(std::vector<uint8_t> *out, std::string *err,
                             std::string *warn, const std::string &filename,
                             const std::string &basedir, bool required,
                             size_t reqBytes, bool checkSize,
                             size_t maxFileSize, FsCallbacks *fs)
{
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

  std::vector<uint8_t> buf;
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

bool FileExists(const std::string &abs_filename, void *)
{
  bool ret;
  struct stat sb;
  if (stat(abs_filename.c_str(), &sb)) {
    return false;
  }
  if (S_ISDIR(sb.st_mode)) {
    return false;
  }

  FILE *fp = fopen(abs_filename.c_str(), "rb");
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }
  return ret;
}

std::string ExpandFilePath(const std::string &filepath, void *) {
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

  out->resize(sz);
  f.read(reinterpret_cast<char *>(&out->at(0)),
         static_cast<std::streamsize>(sz));

  return true;
}

bool WriteWholeFile(std::string *err, const std::string &filepath,
                    const std::vector<unsigned char> &contents, void *) {
  std::ofstream f(filepath.c_str(), std::ofstream::binary);
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

bool IsDataURI(const std::string &in) {
  std::string header = "data:application/octet-stream;base64,";
  if (in.find(header) == 0)
    return true;

  header = "data:image/jpeg;base64,";
  if (in.find(header) == 0)
    return true;

  header = "data:image/png;base64,";
  if (in.find(header) == 0)
    return true;

  header = "data:image/bmp;base64,";
  if (in.find(header) == 0)
    return true;

  header = "data:image/gif;base64,";
  if (in.find(header) == 0)
    return true;

  header = "data:text/plain;base64,";
  if (in.find(header) == 0)
    return true;

  header = "data:application/gltf-buffer;base64,";
  if (in.find(header) == 0)
    return true;

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

static void ParseStringProperty(std::string &s, JSONObject *o, std::string prop, bool req)
{
    JSONProperty *p = o->getProperty(prop);

    if (p == nullptr)
    {
        if (req)
            throw "Cannot find property";
        
        return;
    }

    JSONString *str = dynamic_cast<JSONString *>(p->value());
    s = str->s();
}

template <typename T>
static void ParseIntegerProperty(T &i, JSONObject *o, std::string prop, bool req)
{
    JSONProperty *p = o->getProperty(prop);
    
    if (p == nullptr)
    {
        if (req)
            throw "Cannot find required property";

        return;
    }

    JSONNumber *nr = dynamic_cast<JSONNumber *>(p->value());

    if (nr == nullptr)
    {
        if (req)
            throw "Not the right type";

        return;
    }

    i = T(stoi(nr->value()));
}

static bool
ParseBuffer(Buffer *buffer, std::string *err, JSONObject *o, FsCallbacks *fs,
            const URICallbacks *uri_cb,
            const std::string &basedir, const size_t max_buffer_size, bool is_binary,
            const uint8_t *bin_data, size_t bin_size)
{
    size_t byteLength;
    ParseIntegerProperty<size_t>(byteLength, o, "byteLength", true);

    // In glTF 2.0, uri is not mandatory anymore
    buffer->uri.clear();
    ParseStringProperty(buffer->uri, o, "uri", false);

    // having an empty uri for a non embedded image should not be valid
    if (!is_binary && buffer->uri.empty()) {
        throw "uri is missing from non binary glTF file buffer";
  }

  if (is_binary) {
    // Still binary glTF accepts external dataURI.
    if (!buffer->uri.empty()) {
      // First try embedded data URI.
      if (IsDataURI(buffer->uri)) {
        std::string mime_type;
        if (!DecodeDataURI(&buffer->data, mime_type, buffer->uri, byteLength, true)) {
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
  return true;
}

static void ParseBufferView(BufferView *bufferView, JSONObject *o)
{
    ParseIntegerProperty<int>(bufferView->buffer, o, "buffer", true);
    ParseIntegerProperty<size_t>(bufferView->byteOffset, o, "byteOffset", true);
    ParseIntegerProperty<size_t>(bufferView->byteLength, o, "byteLength", true);
    ParseIntegerProperty<int>(bufferView->target, o, "target", false);
}

static void ParseAccessor(Accessor *accessor, JSONObject *o)
{
    JSONNumber *nr;
    JSONString *s;

    ParseIntegerProperty<int>(accessor->bufferView, o, "bufferView", true);
    int byteOffset = 0;
    accessor->byteOffset = byteOffset;

    bool normalized = false;
    accessor->normalized = normalized;

    ParseIntegerProperty<int>(accessor->componentType, o, "componentType", true);
    ParseIntegerProperty<size_t>(accessor->count, o, "count", true);

    std::string type;
    ParseStringProperty(type, o, "type", true);

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
        throw "error";
    }

    accessor->minValues.clear();
    //TODO: vector uitlezen
    accessor->maxValues.clear();
    //TODO: vector uitlezen
}

static void ParsePrimitive(Primitive &primitive, JSONObject *o)
{
    primitive.material = -1;
    ParseIntegerProperty(primitive.material, o, "material", false);
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    ParseIntegerProperty(primitive.mode, o, "mode", false);
    primitive.indices = -1;
    ParseIntegerProperty(primitive.indices, o, "indices", false);

    JSONObject *attrObj = dynamic_cast<JSONObject *>(o->getProperty("attributes")->value());

    for (JSONProperty *p : *attrObj)
    {
        JSONNumber *nrn = dynamic_cast<JSONNumber *>(p->value());
        int nr = std::stoi(nrn->value());
        primitive.attributes[p->key()] = nr;
    }
}

static void ParseMesh(Mesh &mesh, JSONObject *o)
{
    ParseStringProperty(mesh.name, o, "name", false);
    mesh.primitives.clear();
    JSONProperty *p = o->getProperty("primitives");

    if (p)
    {
        JSONArray *a = dynamic_cast<JSONArray *>(p->value());

        for (JSONNode *node : *a)
        {
            JSONObject *primitiveObj = dynamic_cast<JSONObject *>(node);
            Primitive primitive;
            ParsePrimitive(primitive, primitiveObj);
            mesh.primitives.push_back(primitive);
        }
    }
}

static void ParseDoubleArray(std::vector<double> &v, JSONArray *a)
{
    for (JSONNode *node : *a)
    {
        JSONNumber *nr = dynamic_cast<JSONNumber *>(node);
        v.push_back(std::stod(nr->value()));
    }
}

static void
ParseDoubleArrayProperty(std::vector<double> &v, JSONObject *o, std::string key, bool req)
{
    JSONProperty *p = dynamic_cast<JSONProperty *>(o->getProperty(key));

    if (p == nullptr)
    {
        if (req)
            throw "No such property";

        return;
    }

    JSONArray *a = dynamic_cast<JSONArray *>(p->value());

    if (a == nullptr)
    {
        if (req)
            throw "Property not an array";

        return;
    }

    ParseDoubleArray(v, a);
}

static void ParseIntegerArray(std::vector<int> &v, JSONArray *a)
{
    for (JSONNode *node : *a)
    {
        JSONNumber *nr = dynamic_cast<JSONNumber *>(node);
        v.push_back(stoi(nr->value()));
    }
}

static void
ParseIntegerArrayProperty(std::vector<int> &v, JSONObject *o, std::string key, bool req)
{
    JSONProperty *p = dynamic_cast<JSONProperty *>(o->getProperty(key));

    if (p == nullptr)
    {
        if (req)
            throw "No such property";

        return;
    }

    JSONArray *a = dynamic_cast<JSONArray *>(p->value());
    
    if (a == nullptr)
    {
        if (req)
            throw "Property not an array";

        return;
    }

    ParseIntegerArray(v, a);
}

static void ParseNode(Node &node, JSONObject *o)
{
    JSONArray *a;
    ParseStringProperty(node.name, o, "name", false);
    node.skin = -1;
    ParseIntegerProperty(node.skin, o, "skin", false);

    ParseDoubleArrayProperty(node.matrix, o, "matrix", false);
    ParseDoubleArrayProperty(node.rotation, o, "rotation", false);
    ParseDoubleArrayProperty(node.scale, o, "scale", false);
    ParseDoubleArrayProperty(node.translation, o, "translation", false);
    node.camera = -1;
    ParseIntegerProperty(node.camera, o, "camera", false);
    node.mesh = -1;
    ParseIntegerProperty(node.mesh, o, "mesh", false);
    node.children.clear();
    ParseIntegerArrayProperty(node.children, o, "children", false);
    ParseDoubleArrayProperty(node.weights, o, "weights", false);
}

static void ParseScene(Scene &scene, JSONObject *o)
{
    ParseStringProperty(scene.name, o, "name", false);
    JSONArray *a = dynamic_cast<JSONArray *>(o->getProperty("nodes")->value());
    ParseIntegerArray(scene.nodes, a);
}

void Accessor::serialize(std::ostream &os) const
{
    os << name << " " << bufferView << " " << byteOffset << " " << normalized
       << " " << componentType << " " << count << " " << type << "\r\n";
}

void Node::serialize(std::ostream &os) const
{
    os.put('{');
    os << "\r\n\"name\":\"" << name << "\"\r\n";
    os << "\"skin\":" << skin << "\r\n";
    os << "\"mesh\":" << mesh << "\r\n";

    os << "\"children:\":[\r\n";
    for (int child : children)
        os << child << "\r\n";
    
    os << "]\r\n";
    os << "\"rotation\":[\r\n";
    for (double rot : rotation)
        os << rot << "\r\n";

    os << "]\r\n";

    os << "\"scale\":[";
    for (double sc : scale)
        os << sc << "\r\n";

    os << "]\r\n";
    os.put('}');
}

void Mesh::serialize(std::ostream &os) const
{
    os.put('{');
    os << "\"name\":\"" << name << "\"";
    os.put(',');
    os << "\"primitives\":[";
    
    for (Primitive primitive : primitives)
        primitive.serialize(os);

    os.put(']');
}

void Primitive::serialize(std::ostream &os) const
{
    os.put('{');
    os << "\"attributes\":{\r\n";
        
    for (auto const& attr : attributes)
    {
        os << "\"" << attr.first << "\":" << attr.second << "\r\n";
    }

    os.put('}');
    os.put('}');
}

bool TinyGLTF::LoadFromString(Model *model, std::string *err, std::string *warn,
                              const char *json_str, unsigned int json_str_length,
                              const std::string &base_dir, unsigned int check_sections)
{
    if (json_str_length < 4) {
        if (err) {
            (*err) = "JSON string too short.\n";
        }
        return false;
    }

    std::stringstream ss(json_str);
    std::vector<std::string> tokens;
    ::tokenize(tokens, ss);
    JSONRoot *jsonRoot = new JSONRoot();
    ::parse(tokens.cbegin(), tokens.cend(), jsonRoot);
    JSONObject *rootObj = dynamic_cast<JSONObject *>(jsonRoot->root());

    model->buffers.clear();
    model->bufferViews.clear();
    model->accessors.clear();
    model->meshes.clear();
    model->nodes.clear();
    model->extensionsUsed.clear();
    model->extensionsRequired.clear();
    model->extensions.clear();
    model->defaultScene = -1;

    // 3. Parse Buffer
    {
        JSONArray *a = dynamic_cast<JSONArray *>(rootObj->getProperty("buffers")->value());

        for (JSONNode *node : *a)
        {
            JSONObject *o = dynamic_cast<JSONObject *>(node);
            Buffer buffer;

            ParseBuffer(&buffer, err, o, &fs, &uri_cb, base_dir, max_external_file_size_,
                        is_binary_, bin_data_, bin_size_);
        
            model->buffers.push_back(buffer);
        }
    }
    // 4. Parse BufferView
    {
        JSONArray *a = dynamic_cast<JSONArray *>(rootObj->getProperty("bufferViews")->value());

        for (JSONNode *node : *a)
        {
        JSONObject *o = dynamic_cast<JSONObject *>(node);
        BufferView bufferView;
        ParseBufferView(&bufferView, o);
        model->bufferViews.push_back(bufferView);
        }
    }

    // 5. Parse Accessor
    {
        JSONArray *a = dynamic_cast<JSONArray *>(rootObj->getProperty("accessors")->value());

        for (JSONNode *node : *a)
        {
        JSONObject *o = dynamic_cast<JSONObject *>(node);
        Accessor accessor;
        ParseAccessor(&accessor, o);
        model->accessors.push_back(accessor);
        }
    }

    // 6. Parse Mesh
    {
        JSONArray *a = dynamic_cast<JSONArray *>(rootObj->getProperty("meshes")->value());

        for (JSONNode *node : *a)
        {
        JSONObject *o = dynamic_cast<JSONObject *>(node);
        Mesh mesh;
        ParseMesh(mesh, o);
        model->meshes.push_back(mesh);
        }
    }

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
        JSONArray *a = dynamic_cast<JSONArray *>(rootObj->getProperty("nodes")->value());

        for (JSONNode *node : *a)
        {
            JSONObject *o = dynamic_cast<JSONObject *>(node);
            Node n;
            ParseNode(n, o);
            model->nodes.push_back(n);
        }
    }

    // 8. Parse scenes.
    {
        JSONArray *a = dynamic_cast<JSONArray *>(rootObj->getProperty("scenes")->value());

        for (JSONNode *node : *a)
        {
            JSONObject *o = dynamic_cast<JSONObject *>(node);
            Scene scene;
            ParseScene(scene, o);
            model->scenes.push_back(scene);
        }
    }

    return true;
}

bool TinyGLTF::LoadASCIIFromString(Model *model, std::string *err, std::string *warn,
            const char *str, unsigned int length, const std::string &base_dir,
                                   unsigned int check_sections)
{
    is_binary_ = false;
    bin_data_ = nullptr;
    bin_size_ = 0;
    return LoadFromString(model, err, warn, str, length, base_dir, check_sections);
}

bool TinyGLTF::LoadASCIIFromFile(Model *model, std::string *err,
                                 std::string *warn, const std::string &filename,
                                 unsigned int check_sections) {
  std::stringstream ss;
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
                                    std::string *warn, const uint8_t *bytes,
                                    unsigned int size, const std::string &base_dir,
                                    unsigned int check_sections)
{
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

  if ((header_and_json_size % 4) != 0) {
    if (err) {
      (*err) = "JSON Chunk end does not aligned to a 4-byte boundary.";
    }
  }

  if (header_and_json_size == uint64_t(length)) {
    bin_data_ = nullptr;
    bin_size_ = 0;
  } else {
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
  std::string jsonString(reinterpret_cast<const char *>(&bytes[20]), chunk0_length);

  is_binary_ = true;

  bool ret = LoadFromString(model, err, warn,
                            reinterpret_cast<const char *>(&bytes[20]),
                            chunk0_length, base_dir, check_sections);
  if (!ret) {
    return ret;
  }

  return true;
}

bool TinyGLTF::LoadBinaryFromFile(Model *model, std::string *err, std::string *warn,
                                  const std::string &filename, unsigned int check_sections)
{
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

}  // namespace tinygltf


