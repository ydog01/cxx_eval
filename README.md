** [🇨🇳 中文文档](README.zh.md) **

# C++ Mathematical Expression Evaluator

> A powerful, header-only C++ library for parsing and evaluating mathematical expressions

[![C++11](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/w/cpp/11)
[![Header-Only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)]()

## ✨ Features

- **Header-only** – Zero compilation overhead, easy integration
- **Type Flexible** – Works with any numeric type (double, float, int, etc.)
- **Character Support** – Full support for narrow (`char`) and wide (`wchar_t`) strings
- **Variable Management** – Add, get, set, find variables with ease
- **Operator Control** – Define custom operators with precedence and associativity
- **Function Library** – 30+ built-in mathematical functions
- **Custom Functions** – Create functions with variable number of arguments
- **Smart Memory** – RAII-compliant with shared_ptr support

## 🔧 Requirements

- C++11 or later
- Standard Template Library (STL)
- No external dependencies

## 📥 Installation

1. Download `eval.hpp`, `eval_core.hpp` and `options.hpp`
2. Place them in your project's include directory
3. Include `eval.hpp` in your source files
4. Compile with C++11 support enabled

```cpp
#include "eval.hpp"
```

## 🚀 Quick Start

### Basic Arithmetic

```cpp
#include "eval.hpp"
#include <iostream>

using namespace ydog01::eval;

int main()
{
    Evaluator<char, double> eval(Options::All);
    double result = eval("2 * 3 + 1");
    std::cout << "Result: " << result << std::endl;
    return 0;
}
```

### With Variables

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

### Custom Function

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

### Custom Operator

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
std::cout << result << std::endl;
```

## 📖 API Reference

### Configuration

| Method | Description |
|--------|-------------|
| `enable_whitespace_skip(bool)` | Skip whitespace characters |
| `enable_constant_parser(bool)` | Parse numeric constants |
| `enable_function_call(bool)` | Enable parentheses and comma |

### Variable Operations

| Method | Description |
|--------|-------------|
| `add_variable(name, value)` | Add a variable |
| `add_variable(name, ptr)` | Add a variable from pointer |
| `get_variable(name)` | Get variable reference |
| `set_variable(name, value)` | Modify variable |
| `find_variable(name)` | Find variable, returns pointer |

### Operator Registration

| Method | Description |
|--------|-------------|
| `add_prefix(name, func, prec, assoc)` | Register prefix operator |
| `add_infix(name, func, prec, assoc)` | Register infix operator |
| `add_suffix(name, func, prec, assoc)` | Register suffix operator |
| `add_function(name, func, assoc)` | Register function |

### Removal

| Method | Description |
|--------|-------------|
| `remove_variable(name)` | Remove variable |
| `remove_prefix(name)` | Remove prefix operator |
| `remove_infix(name)` | Remove infix operator |
| `remove_suffix(name)` | Remove suffix operator |

### Evaluation

| Method | Description |
|--------|-------------|
| `parse(expr)` | Parse expression, return Expression object |
| `evaluate(expr)` | Parse and evaluate |
| `operator()(expr)` | Same as evaluate |

### Built-in Functions

| Category | Functions |
|----------|-----------|
| Basic Ops | `+ - * / % ^` |
| Constants | `pi e` |
| Trig | `sin cos tan asin acos atan atan2` |
| Hyperbolic | `sinh cosh tanh asinh acosh atanh` |
| Exp/Log | `exp exp2 ln log log10 log2 log1p` |
| Power/Root | `sqrt cbrt hypot` |
| Rounding | `ceil floor round trunc abs` |
| Special | `erf erfc tgamma lgamma` |

### Options

| Option | Value | Description |
|--------|-------|-------------|
| `None` | 0 | No features |
| `WhitespaceSkip` | 1 | Skip whitespace |
| `ConstantParser` | 2 | Parse numbers |
| `Parentheses` | 4 | Support parentheses |
| `Comma` | 8 | Support comma |
| `BuiltinOps` | 16 | Built-in operators |
| `BuiltinConstants` | 32 | Built-in constants |
| `BuiltinFuncs` | 64 | Built-in functions |
| `All` | 127 | Enable all features |

## 🎯 Advanced Examples

### Nested Function Calls

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

### Complex Expressions

```cpp
double result = eval("(2+3)*4 - 5/2 + sin(pi/2)");
double result2 = eval("sqrt( (2^3 + 4^2) / 2 )");
```

## ⚠️ Error Handling

```cpp
try {
    double result = eval("invalid expression");
} catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## 🤝 Contributing

Contributions welcome! Submit pull requests or open issues for bugs and feature requests.
