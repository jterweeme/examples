#include "tools.h"
#include "json_tools.h"
#include <algorithm>
#include <sys/stat.h>  // for is_directory check
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cassert>

namespace tinygltf {

static inline int32_t GetNumComponentsInType(uint32_t ty)
{
    if (ty == TINYGLTF_TYPE_SCALAR)
        return 1;
    if (ty == TINYGLTF_TYPE_VEC2)
        return 2;
    if (ty == TINYGLTF_TYPE_VEC3)
        return 3;
    if (ty == TINYGLTF_TYPE_VEC4)
        return 4;
    if (ty == TINYGLTF_TYPE_MAT2)
        return 4;
    if (ty == TINYGLTF_TYPE_MAT3)
        return 9;
    if (ty == TINYGLTF_TYPE_MAT4)
        return 16;
  
    // Unknown component type
    return -1;
}

static inline int32_t GetComponentSizeInBytes(uint32_t componentType)
{
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

int Accessor::ByteStride(const BufferView &bufferViewObject) const
{
    if (bufferViewObject.byteStride == 0)
    {
        // Assume data is tightly packed.
        int componentSizeInBytes = GetComponentSizeInBytes(static_cast<uint32_t>(componentType));

        if (componentSizeInBytes <= 0)
            return -1;

        int numComponents = GetNumComponentsInType(static_cast<uint32_t>(type));

        if (numComponents <= 0)
            return -1;

        return componentSizeInBytes * numComponents;
    }
    else
    {
        int componentSizeInBytes = GetComponentSizeInBytes(static_cast<uint32_t>(componentType));

        if (componentSizeInBytes <= 0)
            return -1;

        if ((bufferViewObject.byteStride % uint32_t(componentSizeInBytes)) != 0)
            return -1;
        
        return static_cast<int>(bufferViewObject.byteStride);
    }
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
    ParseDoubleArrayProperty(accessor->minValues, o, "min", false);
    accessor->maxValues.clear();
    ParseDoubleArrayProperty(accessor->maxValues, o, "max", false);
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

static void ParseNode(Node &node, JSONObject *o)
{
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
    ParseIntegerArrayProperty(scene.nodes, o, "nodes", false);
}

bool TinyGLTF::LoadFromString(Model *model, std::string *err, std::string *warn,
                              const char *json_str, unsigned int json_str_length,
                              const std::string &base_dir, unsigned int)
{
    if (json_str_length < 4) {
        if (err) {
            (*err) = "JSON string too short.\n";
        }
        return false;
    }

    std::stringstream ss(json_str);
    Tokenizer tokenizer(&ss);
    JSONRoot *jsonRoot = new JSONRoot();
    ::parse(jsonRoot, tokenizer);
    JSONObject *rootObj = dynamic_cast<JSONObject *>(jsonRoot->root());

    model->buffers.clear();
    model->bufferViews.clear();
    model->accessors.clear();
    model->meshes.clear();
    model->nodes.clear();
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

        auto bufferView = model->accessors[size_t(primitive.indices)].bufferView;
        if (bufferView < 0 || size_t(bufferView) >= model->bufferViews.size()) {
          if (err) {
            (*err) += "accessor[" + std::to_string(primitive.indices) +
                      "] invalid bufferView";
          }
          return false;
        }

        model->bufferViews[size_t(bufferView)].target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
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
            const char *str, unsigned int length, const std::string &base_dir)
{
    is_binary_ = false;
    bin_data_ = nullptr;
    bin_size_ = 0;
    return LoadFromString(model, err, warn, str, length, base_dir, 0);
}

bool TinyGLTF::LoadASCIIFromFile(Model *model, std::string *err,
                                 std::string *warn, const std::string &filename)
{
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
      static_cast<unsigned int>(data.size()), basedir);

  return ret;
}

bool TinyGLTF::LoadBinaryFromMemory(Model *model, std::string *err,
                                    std::string *warn, const uint8_t *bytes,
                                    unsigned int size, const std::string &base_dir)
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

    bin_data_ = bytes + header_and_json_size + 8;
    bin_size_ = size_t(chunk1_length);
  }

  std::string jsonString(reinterpret_cast<const char *>(&bytes[20]), chunk0_length);
  is_binary_ = true;

  bool ret = LoadFromString(model, err, warn, reinterpret_cast<const char *>(&bytes[20]),
                            chunk0_length, base_dir, 0);
  if (!ret) {
    return ret;
  }

  return true;
}

bool TinyGLTF::LoadBinaryFromFile(Model *model, std::string *err, std::string *warn,
                                  const std::string &filename)
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
                                  static_cast<unsigned int>(data.size()), basedir);

  return ret;
}

}  // namespace tinygltf


