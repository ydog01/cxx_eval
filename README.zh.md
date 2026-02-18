** [en Document](README.md) ** 
 # C++ Mathematical Expression Evaluator 
 > 一个功能全面、仅头文件的 C++ 库，用于解析和计算数学表达式 
 [![C++11](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/w/cpp/11) [![Header-Only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)]() 
 ## ✨ 特性 
 • **仅头文件** – 直接包含即可使用，零编译开销<br>• **类型灵活** – 支持任意数值类型（double、float、int 等）<br>• **字符支持** – 完整支持窄字符（`char`）和宽字符（`wchar_t`）字符串<br>• **变量管理** – 注册变量，支持常量/变量语义<br>• **运算符控制** – 自定义运算符，支持优先级和结合性<br>• **函数库** – 丰富的内置数学函数<br>• **表达式解析** – 三种解析模式：立即、持久、普通<br>• **常量折叠** – 自动优化常量子表达式<br>• **自定义函数** – 使用表达式字符串定义新函数<br>• **智能内存** – 符合 RAII，支持 shared_ptr/weak_ptr 
 ## 🔧 环境要求 
 • C++11 或更高版本<br>• 标准模板库（STL）<br>• 无外部依赖 
 ## 📥 安装 
 1. 下载 `eval.hpp` 和 `table.hpp`<br>
 2. 放入项目的 include 目录<br>
 3. 在源代码中包含 `eval.hpp`<br>
 4. 启用 C++11 支持进行编译 
 ```cpp 
 #include "eval.hpp" 
 ``` 
 ## 🚀 快速入门 
 ### 基本算术运算 
 ```cpp 
 #include "eval.hpp" 
 #include <iostream> 
 int main() 
 { 
     using Evaluator = cxx_eval::basic_eval<char, double>::Evaluator; 
     using Simple = cxx_eval::basic_eval<char, double>::simple; 
     Evaluator eval; 
     Simple::setup_allmath(eval); 
     Simple::register_vars(eval, "x", 10.0); 
     auto expr = eval.parse<false>("2 * x + 3.14"); 
     double result = expr.evaluate(); 
     std::cout << "结果: " << result << std::endl; // 23.14 
     return 0; 
 } 
 ``` 
 ### 使用变量 
 ```cpp 
 Simple::register_vars(eval, "a", 5.0, "b", 3.0, "c", 2.0); 
 auto expr = eval.parse<false>("a * b + c"); 
 std::cout << expr.evaluate() << std::endl; // 17.0 
 auto var_node = eval.get_variables()->find_seq("a"); 
 var_node->get_data()->data = 10.0; 
 std::cout << expr.evaluate() << std::endl; // 32.0 
 ``` 
 ## 📚 文档说明 
 以下是完整的文档、API 参考和高级示例。 
 ## 🧠 核心概念 
 ### 表达式结构 
 库内部使用后缀表达式（RPN）表示：<br>• `c` – 常量值<br>• `v` – 变量引用<br>• `f` – 函数/运算符应用 
 ### 解析模式 
 • **立即模式** – 所有值都转为常量（优化用）<br>• **持久模式** – 所有值保持为变量（使用 weak_ptr）<br>• **普通模式** – 遵循 const/mutable 声明 
 ### 运算符优先级 
 优先级数值越高绑定越紧密。默认优先级：<br>• 赋值（`=`）– 0<br>• 加减法（`+`, `-`）– 1<br>• 乘除法（`*`, `/`, `%`）– 2<br>• 乘方（`^`）– 3<br>• 一元运算符 – 2<br>• 函数 – `size_max`（最高） 
 ## 📖 API 参考 
 ### Evaluator 类 
 #### 构造与设置 
 ```cpp 
 cxx_eval::basic_eval<char, double>::Evaluator eval; 
 eval.set_skip([](char c) { return c == ' '; }); 
 eval.enable_whitespace_skip(true); 
 eval.set_left_delimiter('('); 
 eval.set_right_delimiter(')'); 
 eval.set_cut_delimiter(','); 
 evaluator.enable_brackets(true);
 evaluator.enable_cut(true);
 ``` 
 #### 变量管理 
 ```cpp 
 Simple::register_vars(eval, "var1", 1.0, "var2", 2.0); 
 Simple::register_consts(eval, "PI", 3.14159); 
 auto vars = eval.get_variables(); 
 auto node = vars->find_seq("var1"); 
 if (node && node->has_data()) 
 { 
     double value = node->get_data()->data; 
 } 
 ``` 
 #### 运算符注册 
 ```cpp 
 Simple::register_infix(eval, "**", [](std::shared_ptr<RootVar>* args) 
 { 
     ConstVar result; 
     result.data = std::pow(args[0]->data, args[1]->data); 
     return std::make_shared<ConstVar>(result); 
 }, 3); 
 Simple::register_prefix(eval, "
", [](std::shared_ptr<RootVar>* args) 
 { 
     ConstVar result; 
     result.data = -args[0]->data; 
     return std::make_shared<ConstVar>(result); 
 }, 2); 
 Simple::register_function(eval, "max", [](std::shared_ptr<RootVar>* args) 
 { 
     ConstVar result; 
     result.data = std::max(args[0]->data, args[1]->data); 
     return std::make_shared<ConstVar>(result); 
 }, 2); 
 ``` 
 #### 从表达式定义函数 
 ```cpp 
 Simple::register_function<false>(eval, "f", std::vector<std::string>{"x", "y"}, "x^2 + y^2"); 
 auto expr = eval.parse<false>("f(3,4)"); 
 std::cout << expr.evaluate() << std::endl; // 25.0 
 ``` 
 ### 表达式模板 
 ```cpp 
 auto expr = eval.parse<false>("2 + 2"); 
 double result = expr.evaluate(); 
 auto persistent = eval.parse<true>("x + y"); 
 ``` 
 ## 🎯 高级示例 
 ### 自定义数学常量 
 ```cpp 
 Simple::register_consts(eval, "e", 2.718281828459045); 
 Simple::register_consts(eval, "phi", 1.618033988749895); 
 ``` 
 ### 复杂函数定义 
 ```cpp 
 Simple::register_function<false>(eval, "quadratic", std::vector<std::string>{"a", "b", "c"}, "(-b + sqrt(b^2 - 4*a*c)) / (2*a)"); 
 auto expr = eval.parse<false>("quadratic(1, -5, 6)"); 
 std::cout << expr.evaluate() << std::endl; // 3.0 
 ``` 
 ### 赋值运算符 
 ```cpp 
 Simple::setup_assignment(eval); 
 auto expr = eval.parse<false>("x = 42"); 
 expr.evaluate(); 
 auto check = eval.parse<false>("x"); 
 std::cout << check.evaluate() << std::endl; // 42 
 ``` 
 ### 宽字符支持 
 ```cpp 
 using WEvaluator = cxx_eval::basic_eval<wchar_t, double>::Evaluator; 
 using WSimple = cxx_eval::basic_eval<wchar_t, double>::simple; 
 WEvaluator weval; 
 WSimple::setup_allmath(weval); 
 WSimple::register_vars(weval, L"变量", 3.14); 
 auto wexpr = weval.parse<false>(L"变量 * 2"); 
 std::wcout << wexpr.evaluate() << std::endl; 
 ``` 
 ### 自定义跳过行为 
 ```cpp 
 eval.set_skip([](char c) 
 { 
     return !std::isalnum(c) && c != '+' && c != '-' && c != '*' && c != '/' && c != '^' && c != '(' && c != ')'; 
 }); 
 eval.enable_whitespace_skip(true); 
 ``` 
 ## 🔨 自定义扩展 
 ### 实现自定义数值类型 
 ```cpp 
 struct Complex 
 { 
     double real, imag; 
     Complex operator+(const Complex& other) const 
     { 
         return {real + other.real, imag + other.imag}; 
     } 
 }; 
 using ComplexEval = cxx_eval::basic_eval<char, Complex>::Evaluator; 
 using ComplexSimple = cxx_eval::basic_eval<char, Complex>::simple; 
 ``` 
 ### 自定义常量解析器 
 ```cpp 
 eval.set_constant_parser([](const std::string& str, std::size_t& pos, std::string& structure, std::vector<cxx_eval::basic_eval<char, double>::ConstVar>& constants) -> bool 
 { 
     if (str.substr(pos, 2) == "0x") 
     { 
         // 解析十六进制逻辑 
         return true; 
     } 
     return false; 
 }); 
 ``` 
 ## ⚠️ 错误处理 
 库会抛出 `std::runtime_error` 异常，情况包括：<br>• 语法错误<br>• 未定义的变量<br>• 括号不匹配<br>• 栈上溢/下溢<br>• 过期 weak_ptr 访问 
 ```cpp 
 try 
 { 
     auto result = expr.evaluate(); 
 } 
 catch (const std::runtime_error& e) 
 { 
     std::cerr << "计算错误: " << e.what() << std::endl; 
 } 
 ``` 
 ## 🤝 贡献 
 欢迎贡献代码！提交 issue 或 pull request 报告 bug 或请求功能。