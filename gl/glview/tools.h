#ifndef TOOLS_H
#define TOOLS_H

#include "tinygltf.h"

namespace tinygltf {

static void swap4(unsigned int *val) {
    //we doen alleen little endian nu
    (void)val;
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

}

#endif



