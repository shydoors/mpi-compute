#ifndef PATH_UTIL_HPP
#define PATH_UTIL_HPP

/**
 * @file path_util.hpp
 * @brief 路径工具 — header-only
 *
 * 所有函数均为纯函数，不依赖全局状态。
 *
 * 包含：
 *   path_is_absolute    判断绝对路径
 *   path_join           拼接路径
 *   path_resolve        解析为绝对路径
 *   detect_project_dir  自动检测项目根
 *   ensure_dir          确保目录存在
 *   timestamp_utc8      获取 UTC+8 时间戳
 */

#include "types.hpp"

#include <chrono>
#include <climits>
#include <ctime>
#include <string>
#include <unistd.h>

// ============================================================
// 路径判定
// ============================================================

/// 判断 path 是否为绝对路径
inline bool path_is_absolute(const char* path) noexcept {
  if (path == nullptr) { return false; }
  if (path[0] == '\0') { return false; }
  if (path[0] == '/')  { return true; }
  return false;
}

inline bool path_is_absolute(const std::string& path) noexcept {
  return path_is_absolute(path.c_str());
}

// ============================================================
// 路径拼接
// ============================================================

/// 拼接 base + rel，若 rel 为绝对路径则直接返回 rel
inline std::string path_join(const std::string& base,
                              const std::string& rel) {
  if (rel.empty())  { return base; }
  if (path_is_absolute(rel)) { return rel; }
  if (base.empty()) { return rel; }

  std::string result = base;
  if (result.back() != '/') { result += '/'; }
  result += rel;
  return result;
}

/// 将 path 解析为绝对路径：若已是绝对则直接返回，否则拼接 base + path
inline std::string path_resolve(const std::string& path,
                                 const std::string& base) {
  if (path.empty())          { return path; }
  if (path_is_absolute(path)) { return path; }
  return path_join(base, path);
}

// ============================================================
// 项目根目录检测
// ============================================================

/// 从 CWD 向上查找含 data/ 和 src/ 的目录，作为项目根
inline std::string detect_project_dir() {
  char buf[PATH_MAX] = {};
  if (::getcwd(buf, sizeof(buf)) == nullptr) { return {}; }

  std::string dir(buf);
  while (true) {
    const std::string data_path = dir + "/data";
    const std::string src_path  = dir + "/src";

    if ((access(data_path.c_str(), F_OK) == 0) &&
        (access(src_path.c_str(),  F_OK) == 0)) {
      return dir;
    }

    if (dir == "/" || dir.empty()) { return {}; }

    const auto pos = dir.rfind('/');
    if (pos == std::string::npos) { return {}; }
    dir = (pos == 0) ? "/" : dir.substr(0, pos);
  }
}

// ============================================================
// 目录创建
// ============================================================

/// 确保目录存在（不存在则创建）
inline void ensure_dir(const char* dir) {
  if (dir == nullptr)  { return; }
  if (dir[0] == '\0') { return; }

#ifdef _WIN32
  std::system(("if not exist \"" + std::string(dir) + "\" mkdir \"" + std::string(dir) + "\"").c_str());
#else
  std::system(("mkdir -p \"" + std::string(dir) + "\"").c_str());
#endif
}

inline void ensure_dir(const std::string& dir) {
  ensure_dir(dir.c_str());
}

// ============================================================
// 时间戳
// ============================================================

/// 获取 UTC+8 时间戳字符串 "YYYYMMDD_HHMMSS"
inline std::string timestamp_utc8() {
  const auto now = std::chrono::system_clock::now();
  auto tt = std::chrono::system_clock::to_time_t(now);
  tt += 8 * 3600;  // UTC -> UTC+8

  struct tm t = {};
  gmtime_r(&tt, &t);

  char buf[32] = {};
  std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &t);
  return std::string(buf);
}

#endif  // PATH_UTIL_HPP
