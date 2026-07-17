# C++ 数学表达式求值器

> 一个强大的、仅头文件的 C++ 库，用于解析和求值数学表达式

## ✨ 特性

- **仅头文件** —— 零编译开销，易于集成
- **类型灵活** —— 适用于任何数值类型（double、float、int 等）
- **字符支持** —— 完全支持窄字符（`char`）和宽字符（`wchar_t`）字符串
- **变量管理** —— 轻松添加、获取、设置、查找变量
- **运算符控制** —— 定义具有优先级和结合性的自定义运算符
- **函数库** —— 30+ 个内置数学函数
- **自定义函数** —— 创建具有可变数量参数的函数
- **智能内存** —— 符合 RAII，支持 shared_ptr

## 🔧 要求

- C++11 或更高版本
- 标准模板库（STL）
- 无外部依赖

## 📥 安装

1. 下载 `eval.hpp`、`eval_core.hpp` 和 `options.hpp`
2. 将它们放在项目的 include 目录中
3. 在源文件中包含 `eval.hpp`
4. 启用 C++11 支持进行编译

```cpp
#include "eval.hpp"
```

## 🚀 快速开始

### 基本算术

```cpp
#include "eval.hpp"
#include <iostream>

using namespace ydog01::eval;

int main()
{
    Evaluator<char, double> eval(Options::All);
    double result = eval("2 * 3 + 1");
    std::cout << "结果: " << result << std::endl;
    return 0;
}
```

### 使用变量

```cpp
Evaluator<char, double> eval(Options::All);
eval.add_variable("x", 10.0);
eval.add_variable("y", 20.0);
double result = eval("x + y");
std::cout << result << std::endl;

eval.set_variable("x", 100.0);
result = eval("x + y");
std::cout << result << std::endl;
```

### 自定义函数

```cpp
Evaluator<char, double> eval(Options::All);

eval.add_function("max", [](core::ParamViewer<double> a) {
    double m = a[0];
    for (size_t i = 1; i < a.size(); ++i)
        if (a[i] > m) m = a[i];
    return m;
});

double result = eval("max(10, 20, 30, 5)");
std::cout << result << std::endl;
```

### 自定义运算符

```cpp
Evaluator<char, double> eval;
eval.enable_whitespace_skip();
eval.enable_constant_parser();
eval.enable_function_call();

eval.add_prefix("!", [](core::ParamViewer<double> a) {
    int n = static_cast<int>(a[0]);
    double r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}, 40);

double result = eval("!5");
std::cout << result << std::endl;  // 输出 120
```

## 📖 API 参考

### 配置

| 方法 | 描述 |
|------|------|
| `enable_whitespace_skip(bool)` | 跳过空白字符 |
| `enable_constant_parser(bool)` | 解析数值常量 |
| `enable_function_call(bool)` | 启用括号和逗号 |

### 变量操作

| 方法 | 描述 |
|------|------|
| `add_variable(name, value)` | 添加变量 |
| `add_variable(name, ptr)` | 从指针添加变量 |
| `get_variable(name)` | 获取变量引用 |
| `set_variable(name, value)` | 修改变量 |
| `find_variable(name)` | 查找变量，返回指针 |

### 运算符注册

| 方法 | 描述 |
|------|------|
| `add_prefix(name, func, prec, assoc)` | 注册前缀运算符 |
| `add_infix(name, func, prec, assoc)` | 注册中缀运算符 |
| `add_suffix(name, func, prec, assoc)` | 注册后缀运算符 |
| `add_function(name, func, assoc)` | 注册函数 |

### 移除操作

| 方法 | 描述 |
|------|------|
| `remove_variable(name)` | 移除变量 |
| `remove_prefix(name)` | 移除前缀运算符 |
| `remove_infix(name)` | 移除中缀运算符 |
| `remove_suffix(name)` | 移除后缀运算符 |

### 求值

| 方法 | 描述 |
|------|------|
| `parse(expr)` | 解析表达式，返回 Expression 对象 |
| `evaluate(expr)` | 解析并求值 |
| `operator()(expr)` | 同 evaluate |

### 内置函数

| 类别 | 函数 |
|------|------|
| 基本运算 | `+ - * / % ^` |
| 常量 | `pi e` |
| 三角函数 | `sin cos tan asin acos atan atan2` |
| 双曲函数 | `sinh cosh tanh asinh acosh atanh` |
| 指数/对数 | `exp exp2 ln log log10 log2 log1p` |
| 幂/根 | `sqrt cbrt hypot` |
| 取整 | `ceil floor round trunc abs` |
| 特殊函数 | `erf erfc tgamma lgamma` |

### 选项

| 选项 | 值 | 描述 |
|--------|-------|------|
| `None` | 0 | 无特性 |
| `WhitespaceSkip` | 1 | 跳过空白 |
| `ConstantParser` | 2 | 解析数字 |
| `Parentheses` | 4 | 支持括号 |
| `Comma` | 8 | 支持逗号 |
| `BuiltinOps` | 16 | 内置运算符 |
| `BuiltinConstants` | 32 | 内置常量 |
| `BuiltinFuncs` | 64 | 内置函数 |
| `All` | 127 | 启用所有特性 |

## 🎯 高级示例

### 嵌套函数调用

```cpp
Evaluator<char, double> eval(Options::All);
eval.add_function("max", [](auto a) {
    double m = a[0];
    for (size_t i = 1; i < a.size(); ++i)
        if (a[i] > m) m = a[i];
    return m;
});
double result = eval("max(sin(pi/2), cos(0), tan(pi/4))");
```

### 复杂表达式

```cpp
double result = eval("(2+3)*4 - 5/2 + sin(pi/2)");
double result2 = eval("sqrt( (2^3 + 4^2) / 2 )");
```

## ⚠️ 错误处理

```cpp
try {
    double result = eval("无效表达式");
} catch (const std::runtime_error& e) {
    std::cerr << "错误: " << e.what() << std::endl;
}
```

## 🤝 贡献

欢迎贡献！如有问题或功能请求，请提交 PR 或 issue。

---

**[English](README.md) | 中文**
