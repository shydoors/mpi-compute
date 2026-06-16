/**
 * @file json_config.cpp
 * @brief JSON 配置解析 -- 实现
 *
 * JsonParser 放在匿名命名空间中，不暴露给外部。
 */

#include "json_config.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
/** JsonValue 实现 */
JsonValue::JsonValue() : type_{Type::Null}, num_{0.0}, bool_{false} {}
JsonValue::JsonValue(const std::string &s)
    : type_{Type::String}, str_{s}, num_{0.0}, bool_{false} {}
JsonValue::JsonValue(double n) : type_{Type::Number}, num_{n}, bool_{false} {}
JsonValue::JsonValue(bool b) : type_{Type::Bool}, num_{0.0}, bool_{b} {}
JsonValue::JsonValue(Type t) : type_{t}, num_{0.0}, bool_{false} {}
JsonValue::Type JsonValue::type() const noexcept { return type_; }
bool JsonValue::is_null() const noexcept { return type_ == Type::Null; }
bool JsonValue::is_string() const noexcept { return type_ == Type::String; }
bool JsonValue::is_number() const noexcept { return type_ == Type::Number; }
bool JsonValue::is_bool() const noexcept { return type_ == Type::Bool; }
bool JsonValue::is_object() const noexcept { return type_ == Type::Object; }
bool JsonValue::is_array() const noexcept { return type_ == Type::Array; }

std::string JsonValue::as_string(const std::string &def) const {
  if (type_ == Type::String) {
    return str_;
  }
  if (type_ == Type::Null) {
    return def;
  }
  return str_;
}

double JsonValue::as_double(double def) const {
  if (type_ == Type::Number) {
    return num_;
  }
  if (type_ == Type::Null) {
    return def;
  }
  return num_;
}

i32 JsonValue::as_int(i32 def) const {
  if (type_ == Type::Number) {
    return static_cast<i32>(num_);
  }
  if (type_ == Type::Null) {
    return def;
  }
  return static_cast<i32>(num_);
}

u32 JsonValue::as_uint32(u32 def) const {
  if (type_ == Type::Number) {
    return static_cast<u32>(num_);
  }
  if (type_ == Type::Null) {
    return def;
  }
  return static_cast<u32>(num_);
}

bool JsonValue::as_bool(bool def) const {
  if (type_ == Type::Bool) {
    return bool_;
  }
  if (type_ == Type::Null) {
    return def;
  }
  if (type_ == Type::Number) {
    return num_ != 0.0;
  }
  return bool_;
}

const JsonValue &JsonValue::get(const std::string &key) const {
  static JsonValue null_val{};
  if (type_ != Type::Object) {
    return null_val;
  }
  const auto it = obj_.find(key);
  if (it == obj_.end()) {
    return null_val;
  }
  return it->second;
}

const JsonValue &JsonValue::at(std::size_t idx) const {
  static JsonValue null_val{};
  if (type_ != Type::Array) {
    return null_val;
  }
  if (idx >= arr_.size()) {
    return null_val;
  }
  return arr_[idx];
}

std::size_t JsonValue::array_size() const {
  if (type_ != Type::Array) {
    return 0;
  }
  return arr_.size();
}

void JsonValue::set(const std::string &key, JsonValue val) {
  obj_[key] = std::move(val);
}

void JsonValue::push(JsonValue val) { arr_.push_back(std::move(val)); }

// ============================================================
// 内部解析器（匿名命名空间）
// ============================================================
namespace {

class JsonParser {
public:
  explicit JsonParser(std::string input) : input_{std::move(input)}, pos_{0} {}

  JsonValue parse() {
    skip_whitespace();
    if (pos_ >= input_.size()) {
      return JsonValue{};
    }
    return parse_value();
  }

private:
  const std::string input_;
  std::size_t pos_;

  char peek() const {
    if (pos_ < input_.size()) {
      return input_[pos_];
    }
    return '\0';
  }

  char advance() {
    if (pos_ < input_.size()) {
      return input_[pos_++];
    }
    return '\0';
  }

  void skip_whitespace() {
    while (pos_ < input_.size()) {
      const char c = input_[pos_];
      if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) {
        ++pos_;
      } else {
        break;
      }
    }
  }

  void skip_line_comment() {
    while (pos_ < input_.size()) {
      if (input_[pos_] == '\n') {
        return;
      }
      ++pos_;
    }
  }

  void skip_block_comment() {
    ++pos_; // skip '*'
    while (pos_ + 1 < input_.size()) {
      if ((input_[pos_] == '*') && (input_[pos_ + 1] == '/')) {
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
          }
          if (input_[pos_ + 1] == '*') {
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
    const char c = peek();
    switch (c) {
    case '"':
      return parse_string();
    case '{':
      return parse_object();
    case '[':
      return parse_array();
    case 't':
    case 'f':
      return parse_bool();
    case 'n':
      return parse_null();
    default:
      if ((c == '-') || ((c >= '0') && (c <= '9'))) {
        return parse_number();
      }
      return JsonValue{};
    }
  }

  JsonValue parse_string() {
    advance(); // skip '"'
    std::string result{};
    while (pos_ < input_.size()) {
      char c = advance();
      if (c == '"') {
        break;
      }
      if (c == '\\') {
        if (pos_ < input_.size()) {
          const char next = advance();
          switch (next) {
          case '"':
            result += '"';
            break;
          case '\\':
            result += '\\';
            break;
          case '/':
            result += '/';
            break;
          case 'b':
            result += '\b';
            break;
          case 'f':
            result += '\f';
            break;
          case 'n':
            result += '\n';
            break;
          case 'r':
            result += '\r';
            break;
          case 't':
            result += '\t';
            break;
          case 'u': {
            result += "\\u";
            for (int i = 0; i < 4; ++i) {
              if (pos_ < input_.size()) {
                result += advance();
              }
            }
            break;
          }
          default:
            result += next;
            break;
          }
        }
      } else {
        result += c;
      }
    }
    return JsonValue{result};
  }

  JsonValue parse_number() {
    const std::size_t start = pos_;
    if (peek() == '-') {
      advance();
    }
    while (pos_ < input_.size()) {
      const char c = input_[pos_];
      if ((c >= '0') && (c <= '9')) {
        advance();
      } else {
        break;
      }
    }
    if (peek() == '.') {
      advance();
      while (pos_ < input_.size()) {
        const char c = input_[pos_];
        if ((c >= '0') && (c <= '9')) {
          advance();
        } else {
          break;
        }
      }
    }
    if ((peek() == 'e') || (peek() == 'E')) {
      advance();
      if ((peek() == '+') || (peek() == '-')) {
        advance();
      }
      while (pos_ < input_.size()) {
        const char c = input_[pos_];
        if ((c >= '0') && (c <= '9')) {
          advance();
        } else {
          break;
        }
      }
    }
    const double val = std::stod(input_.substr(start, pos_ - start));
    return JsonValue{val};
  }

  JsonValue parse_bool() {
    if (input_.substr(pos_, 4) == "true") {
      pos_ += 4;
      return JsonValue{true};
    }
    if (input_.substr(pos_, 5) == "false") {
      pos_ += 5;
      return JsonValue{false};
    }
    return JsonValue{};
  }

  JsonValue parse_null() {
    if (input_.substr(pos_, 4) == "null") {
      pos_ += 4;
      return JsonValue{};
    }
    return JsonValue{};
  }

  JsonValue parse_object() {
    JsonValue obj{JsonValue::Type::Object};
    advance(); // skip '{'
    bool first = true;
    while (true) {
      skip_comments_and_whitespace();
      if (peek() == '}') {
        advance();
        break;
      }
      if (!first) {
        if (peek() == ',') {
          advance();
        }
        skip_comments_and_whitespace();
      }
      first = false;
      if (peek() != '"') {
        break;
      }
      const auto key_val = parse_string();
      const std::string key = key_val.as_string();
      skip_comments_and_whitespace();
      if (peek() != ':') {
        break;
      }
      advance();
      skip_comments_and_whitespace();
      obj.set(key, parse_value());
    }
    return obj;
  }

  JsonValue parse_array() {
    JsonValue arr{JsonValue::Type::Array};
    advance(); // skip '['
    bool first = true;
    while (true) {
      skip_comments_and_whitespace();
      if (peek() == ']') {
        advance();
        break;
      }
      if (!first) {
        if (peek() == ',') {
          advance();
        }
        skip_comments_and_whitespace();
      }
      first = false;
      arr.push(parse_value());
    }
    return arr;
  }
};

} // anonymous namespace

// ============================================================
// JsonConfig 实现
// ============================================================

bool JsonConfig::load(const std::string &filepath) {
  std::ifstream ifs{filepath};
  if (!ifs) {
    std::fprintf(stderr, "Error: 无法打开配置文件: %s\n", filepath.c_str());
    return false;
  }
  std::stringstream ss{};
  ss << ifs.rdbuf();
  return parse(ss.str());
}

bool JsonConfig::parse(const std::string &content) {
  JsonParser parser{content};
  root_ = parser.parse();
  return root_.is_object();
}

const JsonValue &JsonConfig::get(const std::string &dotted_path) const {
  return query(dotted_path);
}

std::string JsonConfig::get_string(const std::string &key,
                                   const std::string &def) const {
  return get(key).as_string(def);
}

i32 JsonConfig::get_int(const std::string &key, i32 def) const {
  return get(key).as_int(def);
}

u32 JsonConfig::get_uint32(const std::string &key, u32 def) const {
  return get(key).as_uint32(def);
}

double JsonConfig::get_double(const std::string &key, double def) const {
  return get(key).as_double(def);
}

bool JsonConfig::get_bool(const std::string &key, bool def) const {
  return get(key).as_bool(def);
}

bool JsonConfig::has(const std::string &dotted_path) const {
  return !query(dotted_path).is_null();
}

const JsonValue &JsonConfig::query(const std::string &path) const {
  static JsonValue null_val{};
  if (!root_.is_object()) {
    return null_val;
  }

  const JsonValue *current = &root_;
  std::size_t start = 0;

  while (start < path.size()) {
    const auto dot = path.find('.', start);
    const std::string key = (dot == std::string::npos)
                                ? path.substr(start)
                                : path.substr(start, dot - start);

    if (!current->is_object()) {
      return null_val;
    }
    current = &current->get(key);
    if (current->is_null()) {
      return null_val;
    }

    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }

  return *current;
}
