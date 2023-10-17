#include "tinygltf.h"
#include "json.h"
#include <algorithm>
#include <sys/stat.h>  // for is_directory check
#include <cstdio>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TINYGLTF_LITTLE_ENDIAN 1
#endif

namespace tinygltf {
namespace detail {

using nlohmann::json;
using json_iterator = json::iterator;
using json_const_iterator = json::const_iterator;
using json_const_array_iterator = json_const_iterator;
using JsonDocument = json;

void JsonParse(JsonDocument &doc, const char *str, size_t length, bool throwExc = false)
{
  doc = detail::json::parse(str, str + length, nullptr, throwExc);
}
}  // namespace detail
}  // namespace tinygltf

namespace tinygltf {

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

bool GetNumber(const detail::json &o, double &val) {
  if (o.is_number()) {
    val = o.get<double>();
    return true;
  }

  return false;
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

static bool
ParseIntegerProperty(int *ret, std::string *err, const detail::json &o,
     const std::string &property, const bool required, const std::string &parent_node = "")
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

  auto &value = detail::GetValue(it);

  size_t uValue = 0;
  bool isUValue;

  isUValue = value.is_number_unsigned();
  if (isUValue) {
    uValue = value.get<size_t>();
  }
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
                  const detail::json &o, const std::string &property,
                  bool required, const std::string &parent_node = "")
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
    const std::string &parent_node = std::string())
{
  detail::json_const_iterator it;
  if (!detail::FindMember(o, property.c_str(), it)) {
    if (required) {
      throw "property is missing";
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

template <typename GltfType> static bool
ParseExtrasAndExtensions(GltfType *target, std::string *err,
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

static void ParseAccessor(Accessor *accessor, JSONObject *o)
{
    JSONNumber *nr;
    JSONString *s;

    nr  = dynamic_cast<JSONNumber *>(o->getProperty("bufferView")->value());
    int bufferView = stoi(nr->value());
    accessor->bufferView = bufferView;

    int byteOffset = 0;
    accessor->byteOffset = byteOffset;

    bool normalized = false;
    accessor->normalized = normalized;

    nr = dynamic_cast<JSONNumber *>(o->getProperty("componentType")->value());
    size_t componentType = stoi(nr->value());
    accessor->componentType = int(componentType);

    nr = dynamic_cast<JSONNumber *>(o->getProperty("count")->value());
    size_t count = stoi(nr->value());
    accessor->count = count;

    s = dynamic_cast<JSONString *>(o->getProperty("type")->value());
    std::string type = s->s();

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
    accessor->maxValues.clear();
}

static void ParseAccessor(Accessor *accessor, const detail::json &o)
{
    std::string *err = new std::string();

    int bufferView = -1;
    ParseIntegerProperty(&bufferView, err, o, "bufferView", false, "Accessor");
    accessor->bufferView = bufferView;

    size_t byteOffset = 0;
    ParseUnsignedProperty(&byteOffset, err, o, "byteOffset", false, "Accessor");
    accessor->byteOffset = byteOffset;

    bool normalized = false;
    ParseBooleanProperty(&normalized, err, o, "normalized", false, "Accessor");
    accessor->normalized = normalized;

    size_t componentType = 0;
    ParseUnsignedProperty(&componentType, err, o, "componentType", true, "Accessor");
    accessor->componentType = int(componentType);

    size_t count = 0;
    ParseUnsignedProperty(&count, err, o, "count", true, "Accessor");
    accessor->count = count;

    std::string type;
    ParseStringProperty(&type, err, o, "type", true, "Accessor");

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

    ParseStringProperty(&accessor->name, err, o, "name", false);
    accessor->minValues.clear();
    //ParseNumberArrayProperty(&accessor->minValues, err, o, "min", false, "Accessor");
    accessor->maxValues.clear();
    //ParseNumberArrayProperty(&accessor->maxValues, err, o, "max", false, "Accessor");
}

static bool ParsePrimitive(Primitive *primitive, Model *model, std::string *err,
                const detail::json &o, bool store_original_json_for_extras_and_extensions)
{
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
  ParseExtrasAndExtensions(node, err, o, store_original_json_for_extras_and_extensions);
  return true;
}

static bool ParseScene(Scene *scene, std::string *err, const detail::json &o,
                       bool store_original_json_for_extras_and_extensions)
{
    ParseStringProperty(&scene->name, err, o, "name", false);
    ParseIntegerArrayProperty(&scene->nodes, err, o, "nodes", false);

    ParseExtrasAndExtensions(scene, err, o,
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
                              const char *json_str, unsigned int json_str_length,
                              const std::string &base_dir, unsigned int check_sections)
{
    if (json_str_length < 4) {
    if (err) {
      (*err) = "JSON string too short.\n";
    }
    return false;
    }

    detail::JsonDocument v;
    std::stringstream ss(json_str);
    std::vector<std::string> tokens;
    ::tokenize(tokens, ss);
#if 0
    for (auto token : tokens)
        std::cerr << token << "\r\n";
#endif
    JSONRoot *jsonRoot = new JSONRoot();
    ::parse(tokens.cbegin(), tokens.cend(), jsonRoot);
    JSONObject *rootObj = dynamic_cast<JSONObject *>(jsonRoot->root());
#if 0
    std::vector<JSONProperty *>::iterator it = rootObj->begin();
    
    while (it != rootObj->end())
    {
        JSONProperty *prop = *it++;
        std::cerr << prop->key() << "\r\n";
    }
#endif

    try {
        detail::JsonParse(v, json_str, json_str_length, true);
    } catch (const std::exception &e) {
        if (err) {
          (*err) = e.what();
        }
        return false;
    }

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
        if (detail::FindMember(v, "asset", it) && detail::IsObject(detail::GetValue(it)))
        {
            auto &itObj = detail::GetValue(it);
            detail::json_const_iterator version_it;
            std::string versionStr;

            if (detail::FindMember(itObj, "version", version_it) &&
                detail::GetString(detail::GetValue(version_it), versionStr))
            {
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
  model->nodes.clear();
  model->extensionsUsed.clear();
  model->extensionsRequired.clear();
  model->extensions.clear();
  model->defaultScene = -1;

  using detail::ForEachInArray;

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
    bool success = ForEachInArray(v, "bufferViews", [&](const detail::json &o)
    {
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
    JSONArray *a = dynamic_cast<JSONArray *>(rootObj->getProperty("accessors")->value());
    std::vector<JSONNode *>::iterator it = a->begin();
    while (it != a->end())
    {
        JSONObject *o = dynamic_cast<JSONObject *>(*it);
        ++it;
        Accessor accessor;
        ParseAccessor(&accessor, o);
        model->accessors.push_back(accessor);
    }
#if 0
    bool success = ForEachInArray(v, "accessors", [&](const detail::json &o)
    {
        Accessor accessor;
        ParseAccessor(&accessor, o);
        model->accessors.push_back(accessor);
        return true;
    });

    if (!success) {
      return false;
    }
#endif
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


