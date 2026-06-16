#ifndef JSON_CONFIG_HPP
#define JSON_CONFIG_HPP

/**
 * @file json_config.hpp
 * @brief 极简 JSON 配置解析 -- 声明
 *
 * 实现文件: src/json_config.cpp
 *
 * 手写递归下降解析器，零外部依赖。
 * 支持：对象 / 数组 / 字符串 / 数字 / 布尔 / null / 行注释 / 块注释
 */

#include "types.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
// JSON 值类型
// ============================================================
class JsonValue {
public:
  enum class Type : i32 {
    Null   = 0,
    String = 1,
    Number = 2,
    Bool   = 3,
    Object = 4,
    Array  = 5,
  };

  JsonValue();
  explicit JsonValue(const std::string& s);
  explicit JsonValue(double n);
  explicit JsonValue(bool b);
  explicit JsonValue(Type t);

  // ---- 类型查询 ----
  [[nodiscard]] Type  type()       const noexcept;
  [[nodiscard]] bool  is_null()    const noexcept;
  [[nodiscard]] bool  is_string()  const noexcept;
  [[nodiscard]] bool  is_number()  const noexcept;
  [[nodiscard]] bool  is_bool()    const noexcept;
  [[nodiscard]] bool  is_object()  const noexcept;
  [[nodiscard]] bool  is_array()   const noexcept;

  // ---- 取值（带默认值） ----
  [[nodiscard]] std::string  as_string(const std::string& def = {}) const;
  [[nodiscard]] double       as_double(double def = 0.0) const;
  [[nodiscard]] i32          as_int(i32 def = 0) const;
  [[nodiscard]] u32          as_uint32(u32 def = 0) const;
  [[nodiscard]] bool         as_bool(bool def = false) const;

  // ---- 对象/数组访问 ----
  [[nodiscard]] const JsonValue& get(const std::string& key) const;
  [[nodiscard]] const JsonValue& at(std::size_t idx) const;
  [[nodiscard]] std::size_t      array_size() const;

  // ---- 内部构造 ----
  void set(const std::string& key, JsonValue val);
  void push(JsonValue val);

private:
  Type        type_{Type::Null};
  std::string str_{};
  double      num_{0.0};
  bool        bool_{false};
  std::unordered_map<std::string, JsonValue> obj_{};
  std::vector<JsonValue> arr_{};
};

// ============================================================
// JSON 配置接口
// ============================================================
class JsonConfig {
public:
  JsonConfig() = default;

  /// 从 JSON 文件加载
  [[nodiscard]] bool load(const std::string& filepath);

  /// 从 JSON 字符串解析
  [[nodiscard]] bool parse(const std::string& content);

  /// 获取值，支持嵌套路径（如 "output.render_max_points"）
  [[nodiscard]] const JsonValue& get(const std::string& dotted_path) const;

  /// 获取字符串值
  [[nodiscard]] std::string get_string(
      const std::string& key,
      const std::string& def = {}) const;

  /// 获取整数值
  [[nodiscard]] i32 get_int(const std::string& key, i32 def = 0) const;
  [[nodiscard]] u32 get_uint32(const std::string& key, u32 def = 0) const;

  /// 获取浮点数值
  [[nodiscard]] double get_double(const std::string& key,
                                   double def = 0.0) const;

  /// 获取布尔值
  [[nodiscard]] bool  get_bool(const std::string& key,
                                bool def = false) const;

  /// 检查 key 是否存在
  [[nodiscard]] bool  has(const std::string& dotted_path) const;

private:
  JsonValue root_{};

  /// 递归查询点号路径
  [[nodiscard]] const JsonValue& query(const std::string& path) const;
};

#endif  // JSON_CONFIG_HPP
