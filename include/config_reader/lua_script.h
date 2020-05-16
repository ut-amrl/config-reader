// Copyright 2019 - 2020 Kyle Vedder (kvedder@seas.upenn.edu),
// 2018 Ishan Khatri (ikhatri@umass.edu)
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
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ========================================================================
#ifndef CONFIGREADER_LUA_SCRIPT_H_
#define CONFIGREADER_LUA_SCRIPT_H_

#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include "lua5.1/lauxlib.h"
#include "lua5.1/lua.h"
#include "lua5.1/lualib.h"
}

static constexpr bool kDisableTopLevelMissingError = true;

inline void StackDump(lua_State* L) {
  int top = lua_gettop(L);
  for (int i = 1; i <= top; i++) { /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {
      case LUA_TSTRING: /* strings */
        printf("`%s'", lua_tostring(L, i));
        break;

      case LUA_TBOOLEAN: /* booleans */
        printf(lua_toboolean(L, i) ? "true" : "false");
        break;

      case LUA_TNUMBER: /* numbers */
        printf("%g", lua_tonumber(L, i));
        break;

      default: /* other values */
        printf("%s", lua_typename(L, t));
        break;
    }
    printf(" <-> "); /* put a separator */
  }
  printf("\n"); /* end the listing */
}

namespace config_reader {
class LuaScript {
  lua_State* lua_state_;

  void ResetStack() { lua_pop(lua_state_, lua_gettop(lua_state_)); }

  void Error(const std::string& variable_name,
             const std::string& reason) const {
    std::cerr << "Error: can't get [" << variable_name << "]. " << reason
              << std::endl;
  }

  bool LoadStackLocation(const std::string& variable_name) {
    int level = 0;
    std::string var = "";
    for (size_t i = 0; i < variable_name.size(); i++) {
      if (variable_name.at(i) == '.') {
        if (level == 0) {
          lua_getglobal(lua_state_, var.c_str());
        } else {
          lua_getfield(lua_state_, -1, var.c_str());
        }

        if (lua_isnil(lua_state_, -1)) {
          if (!kDisableTopLevelMissingError) {
            Error(variable_name, var + " is not defined");
          }
          return false;
        } else {
          var = "";
          level++;
        }
      } else {
        var += variable_name.at(i);
      }
    }
    if (level == 0) {
      lua_getglobal(lua_state_, var.c_str());
    } else {
      lua_getfield(lua_state_, -1, var.c_str());
    }
    if (lua_isnil(lua_state_, -1)) {
      if (!kDisableTopLevelMissingError || level > 0) {
        Error(variable_name, var + " is not defined");
      }
      return false;
    }

    return true;
  }

  template <typename T>
  T Get(const std::string& variable_name);

  template <typename T>
  T GetDefault();

  void CleanupLuaState() {
    if (lua_state_) {
      lua_close(lua_state_);
      lua_state_ = nullptr;
    }
  }

 public:
  LuaScript() : lua_state_(nullptr) {}

  explicit LuaScript(const std::vector<std::string>& files)
      : lua_state_(luaL_newstate()) {
    luaL_openlibs(lua_state_);
    for (const std::string& filename : files) {
      if (luaL_loadfile(lua_state_, filename.c_str()) ||
          lua_pcall(lua_state_, 0, 0, 0)) {
        std::cout << "Error: failed to load (" << filename << ")" << std::endl;
        std::cout << "Error Message: " << lua_tostring(lua_state_, -1)
                  << std::endl;
        CleanupLuaState();
        break;
      }
    }
  }

  ~LuaScript() { CleanupLuaState(); }

  template <typename T>
  std::pair<bool, T> GetVariable(const std::string& variable_name) {
    if (lua_state_ == nullptr) {
      Error(variable_name, "Script is not loaded");
      return {false, GetDefault<T>()};
    }

    if (!LoadStackLocation(variable_name)) {
      ResetStack();
      return {false, GetDefault<T>()};
    }

    const T result = Get<T>(variable_name);
    ResetStack();
    return {true, result};
  }
};

#define SET_DEFAULT_VALUE(Type, Value)              \
  template <>                                 \
  inline Type LuaScript::GetDefault<Type>() { \
    return Value;                             \
  }

#define SET_DEFAULT(Type)              \
  template <>                                 \
  inline Type LuaScript::GetDefault<Type>() { \
    return Type();                            \
  }

#define GET_NUMBER(Type)                                               \
  template <>                                                          \
  inline Type LuaScript::Get<Type>(const std::string& variable_name) { \
    if (!lua_isnumber(lua_state_, -1)) {                               \
      Error(variable_name, "Not a number");                            \
      return GetDefault<Type>();                                       \
    }                                                                  \
    return static_cast<Type>(lua_tonumber(lua_state_, -1));            \
  }

#define GET_NUMBER_LIST(Type)                                              \
  template <>                                                              \
  inline std::vector<Type> LuaScript::Get<std::vector<Type>>(              \
      const std::string& variable_name) {                                  \
    if (!lua_istable(lua_state_, -1)) {                                    \
      Error(variable_name, "Not a std::vector<Type>");                     \
      return GetDefault<std::vector<Type>>();                              \
    }                                                                      \
    const int table_length = static_cast<int>(lua_objlen(lua_state_, -1)); \
    std::vector<Type> data;                                                \
    for (int i = 1; i <= table_length; ++i) {                              \
      lua_pushinteger(lua_state_, i);                                      \
      lua_gettable(lua_state_, -2);                                        \
      if (!lua_isnumber(lua_state_, -1)) {                                 \
        Error(variable_name, "Element not a Type");                        \
        return GetDefault<std::vector<Type>>();                            \
      }                                                                    \
      data.push_back(static_cast<Type>(lua_tonumber(lua_state_, -1)));     \
      lua_pop(lua_state_, 1);                                              \
    }                                                                      \
    return data;                                                           \
  }

SET_DEFAULT_VALUE(float, 0.0f);
GET_NUMBER(float);

SET_DEFAULT_VALUE(int, 0);
GET_NUMBER(int);

SET_DEFAULT_VALUE(unsigned int, 0);
GET_NUMBER(unsigned int);

SET_DEFAULT_VALUE(double, 0.0);
GET_NUMBER(double);

SET_DEFAULT_VALUE(bool, false);

template <>
inline bool LuaScript::Get<bool>(const std::string& variable_name) {
  if (!lua_isboolean(lua_state_, -1)) {
    Error(variable_name, "Not a boolean");
    return GetDefault<bool>();
  }
  return static_cast<bool>(lua_toboolean(lua_state_, -1));
}

SET_DEFAULT(std::string);
template <>
inline std::string LuaScript::Get<std::string>(
    const std::string& variable_name) {
  if (!lua_isstring(lua_state_, -1)) {
    Error(variable_name, "Not a string");
    return GetDefault<std::string>();
  }
  return std::string(lua_tostring(lua_state_, -1));
}

SET_DEFAULT(std::vector<int>);
GET_NUMBER_LIST(int);

SET_DEFAULT(std::vector<unsigned int>);
GET_NUMBER_LIST(unsigned int);

SET_DEFAULT(std::vector<float>);
GET_NUMBER_LIST(float);

SET_DEFAULT(std::vector<double>);
GET_NUMBER_LIST(double);

SET_DEFAULT(std::vector<std::string>);
template <>
inline std::vector<std::string> LuaScript::Get<std::vector<std::string>>(
    const std::string& variable_name) {
  if (!lua_istable(lua_state_, -1)) {
    Error(variable_name, "Not a std::vector<std::string>");
    return GetDefault<std::vector<std::string>>();
  }
  const int table_length = static_cast<int>(lua_objlen(lua_state_, -1));
  std::vector<std::string> data;
  for (int i = 1; i <= table_length; ++i) {
    lua_pushinteger(lua_state_, i);
    lua_gettable(lua_state_, -2);
    if (!lua_isstring(lua_state_, -1)) {
      Error(variable_name, "Element not a string");
      return GetDefault<std::vector<std::string>>();
    }
    data.push_back(std::string(lua_tostring(lua_state_, -1)));
    lua_pop(lua_state_, 1);
  }
  return data;
}

SET_DEFAULT(std::vector<bool>);
template <>
inline std::vector<bool> LuaScript::Get<std::vector<bool>>(
    const std::string& variable_name) {
  if (!lua_istable(lua_state_, -1)) {
    Error(variable_name, "Not a std::vector<bool>");
    return GetDefault<std::vector<bool>>();
  }
  const int table_length = static_cast<int>(lua_objlen(lua_state_, -1));
  std::vector<bool> data;
  for (int i = 1; i <= table_length; ++i) {
    lua_pushinteger(lua_state_, i);
    lua_gettable(lua_state_, -2);
    if (!lua_isboolean(lua_state_, -1)) {
      Error(variable_name, "Element not a bool");
      return GetDefault<std::vector<bool>>();
    }
    data.push_back(lua_toboolean(lua_state_, -1));
    lua_pop(lua_state_, 1);
  }
  return data;
}

}  // namespace config_reader

#endif  // CONFIGREADER_LUA_SCRIPT_H_
