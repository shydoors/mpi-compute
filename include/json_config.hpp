#ifndef JSON_CONFIG_HPP
#define JSON_CONFIG_HPP

/**
 * @file json_config.hpp
 * @brief 极简 JSON 配置解析 — 专为 config.json 设计
 *
 * 设计思路：
 *   - 无需外部依赖，纯 C++17 手写递归下降解析
 *   - 只解析本项目 config.json 用到的结构：对象、字符串、数字、布尔
 *   - 解析结果以 std::unordered_map<std::string, JsonValue> 树形存储
 *   - 提供链式查询接口，支持嵌套 key（如 "output.render_max_points"）
 *
 * 使用示例：
 *   JsonConfig cfg("config.json");
 *   std::string data_path = cfg.get("data_path").as_string();
 *   std::string backend   = cfg.get("backend.value").as_string();
 *   int k                 = cfg.get("k.value").as_int();
 *   bool streaming        = cfg.get("streaming").as_bool();
 *   int render_max        = cfg.get("output.render_max_points").as_int(100000);
 */

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

// ============================================================
// JSON 值类型
// ============================================================
class JsonValue {
public:
  enum class Type : uint8_t {
    Null,
    String,
    Number,
    Bool,
    Object,
    Array,
  };

  JsonValue() : type_(Type::Null), num_(0.0), bool_(false) {}
  explicit JsonValue(const std::string& s) : type_(Type::String), str_(s), num_(0.0), bool_(false) {}
  explicit JsonValue(double n)          : type_(Type::Number), num_(n), bool_(false) {}
  explicit JsonValue(bool b)            : type_(Type::Bool),   num_(0.0), bool_(b) {}
  explicit JsonValue(Type t)            : type_(t), num_(0.0), bool_(false) {}  // Object/Array placeholder

  // ---- 类型查询 ----
  [[nodiscard]] Type type() const noexcept { return type_; }
  [[nodiscard]] bool is_null()   const noexcept { return type_ == Type::Null; }
  [[nodiscard]] bool is_string() const noexcept { return type_ == Type::String; }
  [[nodiscard]] bool is_number() const noexcept { return type_ == Type::Number; }
  [[nodiscard]] bool is_bool()   const noexcept { return type_ == Type::Bool; }
  [[nodiscard]] bool is_object() const noexcept { return type_ == Type::Object; }
  [[nodiscard]] bool is_array()  const noexcept { return type_ == Type::Array; }

  // ---- 取值（带默认值） ----
  [[nodiscard]] std::string as_string(const std::string& def = "") const {
    if (type_ == Type::String) return str_;
    if (type_ == Type::Null)   return def;
    return str_;
  }

  [[nodiscard]] double as_double(double def = 0.0) const {
    if (type_ == Type::Number) return num_;
    if (type_ == Type::Null)   return def;
    return num_;
  }

  [[nodiscard]] int as_int(int def = 0) const {
    if (type_ == Type::Number) return static_cast<int>(num_);
    if (type_ == Type::Null)   return def;
    return static_cast<int>(num_);
  }

  [[nodiscard]] uint32_t as_uint32(uint32_t def = 0) const {
    if (type_ == Type::Number) return static_cast<uint32_t>(num_);
    if (type_ == Type::Null)   return def;
    return static_cast<uint32_t>(num_);
  }

  [[nodiscard]] bool as_bool(bool def = false) const {
    if (type_ == Type::Bool)   return bool_;
    if (type_ == Type::Null)   return def;
    if (type_ == Type::Number) return num_ != 0.0;
    return bool_;
  }

  // ---- 对象/数组访问 ----
  [[nodiscard]] const JsonValue& get(const std::string& key) const {
    static JsonValue null_val;
    if (type_ != Type::Object) return null_val;
    auto it = obj_.find(key);
    if (it == obj_.end()) return null_val;
    return it->second;
  }

  [[nodiscard]] const JsonValue& operator[](const std::string& key) const {
    return get(key);
  }

  [[nodiscard]] const JsonValue& at(size_t idx) const {
    static JsonValue null_val;
    if (type_ != Type::Array || idx >= arr_.size()) return null_val;
    return arr_[idx];
  }

  [[nodiscard]] size_t array_size() const {
    if (type_ != Type::Array) return 0;
    return arr_.size();
  }

  // ---- 内部构造 ----
  void set(const std::string& key, JsonValue val) {
    obj_[key] = std::move(val);
  }

  void push(JsonValue val) {
    arr_.push_back(std::move(val));
  }

private:
  Type type_;
  std::string str_;
  double num_;
  bool bool_;
  std::unordered_map<std::string, JsonValue> obj_;
  std::vector<JsonValue> arr_;
};

// ============================================================
// JSON 解析器
// ============================================================
class JsonParser {
public:
  explicit JsonParser(std::string input)
    : input_(std::move(input)), pos_(0) {}

  JsonValue parse() {
    skip_whitespace();
    if (pos_ >= input_.size()) return JsonValue();
    return parse_value();
  }

private:
  const std::string input_;
  size_t pos_;

  char peek() const {
    return pos_ < input_.size() ? input_[pos_] : '\0';
  }

  char advance() {
    return pos_ < input_.size() ? input_[pos_++] : '\0';
  }

  void skip_whitespace() {
    while (pos_ < input_.size() &&
           (input_[pos_] == ' ' || input_[pos_] == '\t' ||
            input_[pos_] == '\n' || input_[pos_] == '\r')) {
      ++pos_;
    }
  }

  void skip_line_comment() {
    // '//' 到行末
    while (pos_ < input_.size() && input_[pos_] != '\n') {
      ++pos_;
    }
  }

  void skip_block_comment() {
    // '/*' 到 '*/'
    ++pos_; // skip '*'
    while (pos_ + 1 < input_.size()) {
      if (input_[pos_] == '*' && input_[pos_ + 1] == '/') {
        pos_ += 2;
        return;
      }
      ++pos_;
    }
  }

  void skip_comments_and_whitespace() {
    while (true) {
      skip_whitespace();
      if (peek() == '/') {
        if (pos_ + 1 < input_.size()) {
          if (input_[pos_ + 1] == '/') {
            pos_ += 2;
            skip_line_comment();
            continue;
          } else if (input_[pos_ + 1] == '*') {
            pos_ += 2;
            skip_block_comment();
            continue;
          }
        }
      }
      break;
    }
  }

  JsonValue parse_value() {
    skip_comments_and_whitespace();
    char c = peek();

    switch (c) {
      case '"': return parse_string();
      case '{': return parse_object();
      case '[': return parse_array();
      case 't': case 'f': return parse_bool();
      case 'n': return parse_null();
      default:
        if (c == '-' || (c >= '0' && c <= '9')) {
          return parse_number();
        }
        return JsonValue(); // null
    }
  }

  JsonValue parse_string() {
    advance(); // skip opening '"'
    std::string result;
    while (pos_ < input_.size()) {
      char c = advance();
      if (c == '"') break;
      if (c == '\\') {
        if (pos_ < input_.size()) {
          char next = advance();
          switch (next) {
            case '"':  result += '"'; break;
            case '\\': result += '\\'; break;
            case '/':  result += '/'; break;
            case 'b':  result += '\b'; break;
            case 'f':  result += '\f'; break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            case 'u': {
              // 简易 unicode 处理（跳过4位hex）
              result += "\\u";
              for (int i = 0; i < 4 && pos_ < input_.size(); ++i) {
                result += advance();
              }
              break;
            }
            default: result += next; break;
          }
        }
      } else {
        result += c;
      }
    }
    return JsonValue(result);
  }

  JsonValue parse_number() {
    size_t start = pos_;
    if (peek() == '-') advance();
    while (pos_ < input_.size() && (input_[pos_] >= '0' && input_[pos_] <= '9')) advance();
    if (peek() == '.') {
      advance();
      while (pos_ < input_.size() && (input_[pos_] >= '0' && input_[pos_] <= '9')) advance();
    }
    if (peek() == 'e' || peek() == 'E') {
      advance();
      if (peek() == '+' || peek() == '-') advance();
      while (pos_ < input_.size() && (input_[pos_] >= '0' && input_[pos_] <= '9')) advance();
    }
    double val = std::stod(input_.substr(start, pos_ - start));
    return JsonValue(val);
  }

  JsonValue parse_bool() {
    if (input_.substr(pos_, 4) == "true") {
      pos_ += 4;
      return JsonValue(true);
    }
    if (input_.substr(pos_, 5) == "false") {
      pos_ += 5;
      return JsonValue(false);
    }
    return JsonValue();
  }

  JsonValue parse_null() {
    if (input_.substr(pos_, 4) == "null") {
      pos_ += 4;
      return JsonValue();
    }
    return JsonValue();
  }

  JsonValue parse_object() {
    JsonValue obj(JsonValue::Type::Object);
    advance(); // skip '{'
    bool first = true;
    while (true) {
      skip_comments_and_whitespace();
      if (peek() == '}') { advance(); break; }
      if (!first) {
        if (peek() == ',') { advance(); }
        skip_comments_and_whitespace();
      }
      first = false;

      // key
      if (peek() != '"') break;
      auto key_val = parse_string();
      std::string key = key_val.as_string();

      skip_comments_and_whitespace();
      if (peek() != ':') break;
      advance(); // skip ':'
      skip_comments_and_whitespace();

      // value
      auto val = parse_value();
      obj.set(key, std::move(val));
    }
    return obj;
  }

  JsonValue parse_array() {
    JsonValue arr(JsonValue::Type::Array);
    advance(); // skip '['
    bool first = true;
    while (true) {
      skip_comments_and_whitespace();
      if (peek() == ']') { advance(); break; }
      if (!first) {
        if (peek() == ',') { advance(); }
        skip_comments_and_whitespace();
      }
      first = false;
      auto val = parse_value();
      arr.push(std::move(val));
    }
    return arr;
  }
};

// ============================================================
// 高层配置接口
// ============================================================
class JsonConfig {
public:
  JsonConfig() = default;

  /// 从 JSON 文件加载
  bool load(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs) {
      std::cerr << "Error: 无法打开配置文件: " << filepath << std::endl;
      return false;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return parse(ss.str());
  }

  /// 从 JSON 字符串解析
  bool parse(const std::string& content) {
    JsonParser parser(content);
    root_ = parser.parse();
    return root_.is_object();
  }

  // ---- 链式查询（支持点号路径：如 "backend.value"） ----

  /// 获取值，支持嵌套路径（如 "output.render_max_points"）
  [[nodiscard]] const JsonValue& get(const std::string& dotted_path) const {
    return query(dotted_path);
  }

  /// 获取字符串值
  [[nodiscard]] std::string get_string(const std::string& key,
                                        const std::string& def = "") const {
    return get(key).as_string(def);
  }

  /// 获取整数值
  [[nodiscard]] int get_int(const std::string& key, int def = 0) const {
    return get(key).as_int(def);
  }

  [[nodiscard]] uint32_t get_uint32(const std::string& key, uint32_t def = 0) const {
    return get(key).as_uint32(def);
  }

  /// 获取浮点数值
  [[nodiscard]] double get_double(const std::string& key, double def = 0.0) const {
    return get(key).as_double(def);
  }

  /// 获取布尔值
  [[nodiscard]] bool get_bool(const std::string& key, bool def = false) const {
    return get(key).as_bool(def);
  }

  /// 检查 key 是否存在
  [[nodiscard]] bool has(const std::string& dotted_path) const {
    return !query(dotted_path).is_null();
  }

private:
  JsonValue root_;

  /// 递归查询点号路径
  [[nodiscard]] const JsonValue& query(const std::string& path) const {
    static JsonValue null_val;
    if (!root_.is_object()) return null_val;

    const JsonValue* current = &root_;
    size_t start = 0;
    while (start < path.size()) {
      size_t dot = path.find('.', start);
      std::string key = (dot == std::string::npos)
                          ? path.substr(start)
                          : path.substr(start, dot - start);

      if (!current->is_object()) return null_val;
      current = &current->get(key);
      if (current->is_null()) return null_val;

      if (dot == std::string::npos) break;
      start = dot + 1;
    }
    return *current;
  }
};

#endif // JSON_CONFIG_HPP
