#ifndef CXX_EVAL
#define CXX_EVAL

#include "table.hpp"
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <sstream>
#include <cmath>

namespace cxx_eval
{
    constexpr auto size_max(std::numeric_limits<std::size_t>::max());

    namespace detail
    {
        template <typename>
        struct StringLiteral;

        template <>
        struct StringLiteral<char>
        {
            static constexpr auto get(const char *str, const wchar_t *) -> const char *
            {
                return str;
            }
        };

        template <>
        struct StringLiteral<wchar_t>
        {
            static constexpr auto get(const char *, const wchar_t *wstr) -> const wchar_t *
            {
                return wstr;
            }
        };

        template <bool is_shared>
        struct Ptype
        {
            template <typename T>
            using type = typename std::conditional<is_shared, std::shared_ptr<T>, std::weak_ptr<T>>::type;
        };

        template <bool is_shared, typename T>
        struct GetRaw;

        template <typename T>
        struct GetRaw<true, T>
        {
            static auto get(const std::shared_ptr<T> &ptr) -> std::shared_ptr<T>
            {
                return ptr;
            }
        };

        template <typename T>
        struct GetRaw<false, T>
        {
            static auto get(const std::weak_ptr<T> &ptr) -> std::shared_ptr<T>
            {
                if (auto sp = ptr.lock())
                    return sp;
                throw std::runtime_error("Expired weak_ptr in expression");
            }
        };
    }

    template <typename StringType, typename Type>
    struct eval
    {
        using CharType = typename StringType::value_type;

        enum class var_type
        {
            const_var,
            mutable_var
        };

        enum class parse_mode
        {
            Immediate,  // 立即模式：所有值都解析为常量
            Persistent, // 持久模式：所有值都解析为变量
            Normal      // 普通模式：mutable的为变量，const的为常量
        };

        struct RootVar
        {
            Type data;
            virtual auto get_type() const noexcept -> var_type = 0;
            virtual auto get_name() const noexcept -> StringType
            {
                return StringType();
            }
            virtual ~RootVar()
            {
            }
        };
        struct Variable : RootVar
        {
            var_type type;
            StringType name;
            auto get_name() const noexcept -> StringType override
            {
                return name;
            }
            auto get_type() const noexcept -> var_type override
            {
                return type;
            }
        };
        struct ConstVar : RootVar
        {
            auto get_type() const noexcept -> var_type override
            {
                return var_type::const_var;
            }
        };

        using FuncType = std::function<std::shared_ptr<RootVar>(std::shared_ptr<RootVar> *
#ifdef CXX_EVAL_ID
                                                                ,std::size_t
#endif
                                                                )>;

        struct Operation
        {
            StringType name;
            std::size_t arity;
            std::size_t precedence;
            FuncType function;
        };

        struct OperInfix:Operation
        {
            bool left_associative{false};  // true: 左结合, false: 右结合
        };

        struct OperPrefix:Operation
        {
            bool function_mode{true};  // true: 检查左括号, false: 检查右括号
        };

        template <bool is_shared = false, template <typename> class Ptype = detail::Ptype<is_shared>::template type>
        struct Expression
        {
            std::vector<Ptype<Operation>> operations;
            std::vector<Ptype<Variable>> variables;
            std::vector<ConstVar> constants;
            std::string structure;

            auto evaluate() const -> Type
            {
                std::vector<std::shared_ptr<RootVar>> stack;
                std::size_t op_idx = 0, var_idx = 0, const_idx = 0;

                for (char ch : structure)
                {
                    switch (ch)
                    {
                    case 'c':
                    {
                        if (const_idx >= constants.size())
                            throw std::runtime_error("Constant index out of range");
                        stack.emplace_back(std::make_shared<ConstVar>(constants[const_idx++]));
                        break;
                    }
                    case 'v':
                    {
                        if (var_idx >= variables.size())
                            throw std::runtime_error("Variable index out of range");
                        stack.emplace_back(detail::GetRaw<is_shared, Variable>::get(variables[var_idx++]));
                        break;
                    }
                    case 'f':
                    {
                        if (op_idx >= operations.size())
                            throw std::runtime_error("Operation index out of range");
                        auto op_sp(detail::GetRaw<is_shared, Operation>::get(operations[op_idx++]));
                        if (stack.size() < op_sp->arity)
                            throw std::runtime_error("Stack underflow");

                        auto start{stack.size() - op_sp->arity};
                        auto result(op_sp->function(&stack[start]
#ifdef CXX_EVAL_ID
                                                    ,op_idx
#endif
                                                    ));

                        stack.resize(start);

                        if (result)
                            stack.push_back(result);
                        break;
                    }
                    default:
                        throw std::runtime_error("Invalid structure character");
                    }
                }

                if (stack.size() != 1)
                    throw std::runtime_error("Malformed expression: stack size != 1");

                return stack[0]->data;
            }
        };

        typedef table<CharType, Variable> VariableTree;
        typedef table<CharType, Operation> OperationTree;
        typedef table<CharType, OperInfix> OperInfixTree;
        typedef table<CharType, OperPrefix> OperPrefixTree;
        typedef std::function<bool(const StringType &, std::size_t &, std::string &, std::vector<ConstVar> &)> ParserType;
        typedef std::function<bool(const CharType &)> SkipFuncType;

        class Evaluator
        {
            ParserType constant_parser_;

            std::shared_ptr<VariableTree> variables_;
            std::shared_ptr<OperPrefixTree> prefix_ops_; // 函数也是prefix修饰的,但是函数一定要F(x)
            std::shared_ptr<OperInfixTree> infix_ops_;   // 默认右结合运算
            std::shared_ptr<OperationTree> suffix_ops_;

            SkipFuncType isvoid; // 空白检测器,跳过无用/空白字符
            CharType Lseq;                                // 左结合算符:  (
            CharType Rseq;                                // 右结合算符:  )
            CharType Cuts;                                // 分隔算符:    ,
            bool iropen;                                  // 是否启用空白检测器
            bool lropen;                                  // 是否启用结合运算
            bool ctopen;                                  // 是否启用分隔运算

        public:
            Evaluator()
                : variables_(std::make_shared<VariableTree>()), prefix_ops_(std::make_shared<OperPrefixTree>()),
                  infix_ops_(std::make_shared<OperInfixTree>()), suffix_ops_(std::make_shared<OperationTree>()),
                  Lseq(CharType()), Rseq(CharType()), Cuts(CharType()), iropen(false), lropen(false), ctopen(false)
            {
            }

            auto set_constant_parser(ParserType parser) noexcept -> void
            {
                constant_parser_ = parser;
            }

            auto set_constant_parser() noexcept -> void
            {
                constant_parser_ = nullptr;
            }

            auto set_skip(SkipFuncType parser) noexcept -> void
            {
                isvoid = parser;
            }

            auto set_skip() noexcept -> void
            {
                isvoid = nullptr;
            }

            auto set_variables(std::shared_ptr<VariableTree> vars) noexcept -> void
            {
                variables_ = vars;
            }

            auto set_variables() noexcept -> void
            {
                variables_.reset();
            }

            auto new_variables() -> void
            {
                variables_ = std::make_shared<VariableTree>();
            }

            auto set_prefix_ops(std::shared_ptr<OperPrefixTree> ops) noexcept -> void
            {
                prefix_ops_ = ops;
            }

            auto set_prefix_ops() noexcept -> void
            {
                prefix_ops_.reset();
            }

            auto new_prefix_ops() -> void
            {
                prefix_ops_ = std::make_shared<OperationTree>();
            }

            auto set_infix_ops(std::shared_ptr<OperInfixTree> ops) noexcept -> void
            {
                infix_ops_ = ops;
            }

            auto set_infix_ops() noexcept -> void
            {
                infix_ops_.reset();
            }

            auto new_infix_ops() -> void
            {
                infix_ops_ = std::make_shared<OperationTree>();
            }

            auto set_suffix_ops(std::shared_ptr<OperationTree> ops) noexcept -> void
            {
                suffix_ops_ = ops;
            }

            auto set_suffix_ops() noexcept -> void
            {
                suffix_ops_.reset();
            }

            auto new_suffix_ops() -> void
            {
                suffix_ops_ = std::make_shared<OperationTree>();
            }

            auto enable_brackets(bool enable) noexcept -> void
            {
                lropen = enable;
            }

            auto enable_cut(bool enable) noexcept -> void
            {
                ctopen = enable;
            }

            auto enable_whitespace_skip(bool enable) noexcept -> void
            {
                iropen = enable;
            }

            auto set_left_delimiter(CharType left) noexcept -> void
            {
                Lseq = left;
            }

            auto set_right_delimiter(CharType right) noexcept -> void
            {
                Rseq = right;
            }

            auto set_cut_delimiter(CharType cut) noexcept -> void
            {
                Cuts = cut;
            }

            auto get_variables() noexcept -> std::shared_ptr<VariableTree> &
            {
                return variables_;
            }

            auto get_prefixs() noexcept -> std::shared_ptr<OperPrefixTree> &
            {
                return prefix_ops_;
            }

            auto get_infixs() noexcept -> std::shared_ptr<OperInfixTree> &
            {
                return infix_ops_;
            }

            auto get_suffixs() noexcept -> std::shared_ptr<OperationTree> &
            {
                return suffix_ops_;
            }

        private:
            auto skip_whitespace(const StringType &str, std::size_t &pos) const -> void
            {
                if (iropen && isvoid)
                {
                    while (pos < str.size() && isvoid(str[pos]))
                        ++pos;
                }
            }

            auto parse_left_bracket(const StringType &str, std::size_t &pos,
                                    std::vector<std::shared_ptr<Operation>> &op_stack) -> bool
            {
                if (lropen && str[pos] == Lseq)
                {
                    op_stack.push_back(std::shared_ptr<Operation>());
                    ++pos;
                    return true;
                }
                return false;
            }

            template <bool is_shared>
            auto parse_right_bracket(const StringType &str, std::size_t &pos,
                                     std::vector<std::shared_ptr<Operation>> &op_stack, Expression<is_shared>&expr) -> bool
            {
                if (lropen && str[pos] == Rseq)
                {
                    while (!op_stack.empty() && op_stack.back())
                    {
                        expr.structure.push_back('f');
                        expr.operations.push_back(op_stack.back());
                        
                        op_stack.pop_back();
                    }
                    if (op_stack.empty())
                        throw std::runtime_error("Mismatched parentheses");
                    op_stack.pop_back();
                    ++pos;
                    return true;
                }
                return false;
            }

            
            template <bool is_shared>
            auto parse_delimiter(const StringType &str, std::size_t &pos,
                                 std::vector<std::shared_ptr<Operation>> &op_stack, bool &expecting_operand,
                                 Expression<is_shared>&expr) -> bool
            {
                if (ctopen && str[pos] == Cuts)
                {
                    while (!op_stack.empty() && op_stack.back())
                    {
                        expr.structure.push_back('f');
                        expr.operations.push_back(op_stack.back());
                        op_stack.pop_back();
                    }
                    ++pos;
                    return expecting_operand = true;
                }
                return false;
            }

            template <bool is_shared>
            auto parse_constant(const StringType &str, std::size_t &pos, Expression<is_shared> &expr,
                                bool &expecting_operand) -> bool
            {
                if (constant_parser_ && constant_parser_(str, pos, expr.structure, expr.constants))
                {
                    expecting_operand = false;
                    return true;
                }
                return false;
            }

            template <typename TreeType>
            auto find_longest_match(const StringType &str, std::size_t &pos,
                                                           std::shared_ptr<TreeType> &tree) -> typename TreeType::iterator
            {
                if (!tree)
                    return nullptr;

                auto start_pos(pos);
                auto current_node(tree->self());
                decltype(current_node) target_node(nullptr);

                while (pos < str.size())
                {
                    auto next_node(current_node->find_child(str[pos]));
                    if (!next_node)
                        break;

                    current_node = next_node;
                    ++pos;

                    if (current_node->has_data())
                        target_node = current_node;
                }

                if (!target_node)
                    pos = start_pos;

                return target_node;
            }

            auto parse_prefix(const StringType &str, std::size_t &pos,
                              std::vector<std::shared_ptr<Operation>> &op_stack) -> bool
            {
                auto old(pos);
                auto target_node(find_longest_match(str, pos, prefix_ops_));
                if (!target_node)
                    return false;

                op_stack.push_back(target_node->get_data());
                if(target_node->get_data()->function_mode&&!parse_left_bracket(str,pos,op_stack))
                {
                    op_stack.pop_back();
                    pos = old;
                    return false;
                }

                return true;
            }

            template <bool is_shared>
            auto parse_suffix(const StringType &str, std::size_t &pos, Expression<is_shared> &expr) -> bool
            {
                auto target_node(find_longest_match(str, pos, suffix_ops_));
                if (!target_node)
                    return false;

                expr.operations.push_back(target_node->get_data());
                expr.structure.push_back('f');

                return true;
            }

            template <bool is_shared>
            auto parse_infix(const StringType &str, std::size_t &pos, Expression<is_shared> &expr,
                             std::vector<std::shared_ptr<Operation>> &op_stack, bool &expecting_operand) -> bool
            {
                auto target_node(find_longest_match(str, pos, infix_ops_));
                if (!target_node)
                    return false;

                auto op(target_node->get_data());

                while (!op_stack.empty() && op_stack.back())
                {
                    auto top_op(op_stack.back());
                    if (top_op->precedence > op->precedence ||
                        (op->left_associative && top_op->precedence == op->precedence))
                    {
                        expr.operations.push_back(top_op);
                        expr.structure.push_back('f');
                        op_stack.pop_back();
                        continue;
                    }
                    break;
                }

                op_stack.push_back(op);
                return expecting_operand = true;
            }

            template <bool is_shared>
            auto parse_variable(const StringType &str, std::size_t &pos, Expression<is_shared> &expr,
                                bool &expecting_operand, parse_mode mode) -> bool
            {
                auto target_node(find_longest_match(str, pos, variables_));
                if (!target_node)
                    return false;

                auto var(target_node->get_data());

                auto is_const_var(mode == parse_mode::Immediate ||
                                  (mode == parse_mode::Normal && var->get_type() == var_type::const_var));

                if (is_const_var)
                {
                    ConstVar const_var;
                    const_var.data = var->data;
                    expr.constants.push_back(const_var);
                    expr.structure.push_back('c');
                }
                else
                {
                    expr.variables.push_back(var);
                    expr.structure.push_back('v');
                }
                expecting_operand = false;
                return true;
            }

            template <bool is_shared>
            auto flush_operator_stack(std::vector<std::shared_ptr<Operation>> &op_stack,
                                      Expression<is_shared> &expr) -> void
            {
                while (!op_stack.empty())
                {
                    if (!op_stack.back())
                        throw std::runtime_error("Mismatched parentheses");
                    expr.structure.push_back('f');
                    expr.operations.push_back(op_stack.back());
                    op_stack.pop_back();
                }
            }

        public:
            template <bool is_shared = false>
            auto parse(const StringType &str, parse_mode mode = parse_mode::Normal) -> Expression<is_shared>
            {
                Expression<is_shared> expr;

                std::vector<std::shared_ptr<Operation>> op_stack;

                std::size_t pos(0);
                bool expecting_operand(true);

                while (pos < str.size())
                {
                    skip_whitespace(str, pos);
                    if (pos >= str.size())
                        break;

                    if (expecting_operand)
                    {
                        if (parse_left_bracket(str, pos, op_stack))
                            continue;

                        if (parse_constant<is_shared>(str, pos, expr, expecting_operand))
                            continue;

                        if (parse_prefix(str, pos, op_stack))
                            continue;

                        if (parse_variable<is_shared>(str, pos, expr, expecting_operand, mode))
                            continue;

                        if (parse_right_bracket<is_shared>(str, pos, op_stack, expr))
                            continue;
                    }
                    else
                    {
                        if (parse_right_bracket<is_shared>(str, pos, op_stack, expr))
                            continue;

                        if (parse_delimiter<is_shared>(str, pos, op_stack, expecting_operand, expr))
                            continue;

                        if (parse_infix<is_shared>(str, pos, expr, op_stack, expecting_operand))
                            continue;

                        if (parse_suffix<is_shared>(str, pos, expr))
                            continue;
                    }

                    throw std::runtime_error("Unexpected character at position " + std::to_string(pos));
                }

                flush_operator_stack<is_shared>(op_stack, expr);
                return expr;
            }
        };

        struct simple
        {
            // 通用常量解析器
            static auto parse_constant(const StringType &str, std::size_t &pos, std::string &structure,
                                       std::vector<ConstVar> &constants) -> bool
            {
                if (pos >= str.size())
                    return false;

                auto start{pos};
                auto has_dot{false};
                auto has_digit{false};

                while (pos < str.size())
                {
                    auto ch{str[pos]};
                    if (ch >= CharType('0') && ch <= CharType('9'))
                    {
                        has_digit = true;
                        ++pos;
                    }
                    else if (ch == CharType('.') && !has_dot)
                    {
                        has_dot = true;
                        ++pos;
                    }
                    else
                    {
                        break;
                    }
                }

                if (!has_digit || start == pos)
                {
                    pos = start;
                    return false;
                }

                auto num_str{str.substr(start, pos - start)};
                Type value;
                std::basic_stringstream<CharType> ss(num_str);
                ss >> value;

                ConstVar const_var;
                const_var.data = value;
                constants.push_back(const_var);
                structure.push_back('c');

                return true;
            }

            // 一键注册常量解析器
            static auto setup_constant_parser(Evaluator &evaluator) -> void
            {
                evaluator.set_constant_parser(parse_constant);
            }

            // 空白跳过通用解析器
            static auto skip_whitespace(const CharType &ch) -> bool
            {
                return ch == CharType(' ') || ch == CharType('\t') || ch == CharType('\n') || ch == CharType('\r');
            }

            // 设置空白跳过
            static auto setup_whitespace(Evaluator &evaluator) -> void
            {
                evaluator.set_skip(skip_whitespace);
                evaluator.enable_whitespace_skip(true);
            }

            // 设置括号
            static auto setup_brackets(Evaluator &evaluator) -> void
            {
                evaluator.enable_brackets(true);
                evaluator.set_left_delimiter(CharType('('));
                evaluator.set_right_delimiter(CharType(')'));
            }

            // 设置分隔符
            static auto setup_cut(Evaluator &evaluator) -> void
            {
                evaluator.enable_cut(true);
                evaluator.set_cut_delimiter(CharType(','));
            }

            // 递归结束函数
            static auto register_vars(Evaluator &) -> void
            {
            }

            // 批量注册变量
            template <typename... Args>
            static auto register_vars(Evaluator &evaluator, const StringType &name, Type value, Args... args) -> void
            {
                auto &table = evaluator.get_variables();
                if (!table)
                    table = std::make_shared<VariableTree>();

                auto node{table->find_seq(name)};
                if (node && node->has_data())
                    node->get_data()->data = value;
                else
                {
                    Variable var;
                    var.data = value;
                    var.type = var_type::mutable_var;
                    var.name = name;
                    table->add_seq(name, var);
                }

                register_vars(evaluator, args...);
            }

            // 递归结束函数
            static auto register_consts(Evaluator &) -> void
            {
            }

            // 批量注册常量
            template <typename... Args>
            static auto register_consts(Evaluator &evaluator, const StringType &name, Type value, Args... args) -> void
            {
                auto &table = evaluator.get_variables();
                if (!table)
                    table = std::make_shared<VariableTree>();

                auto node{table->find_seq(name)};
                if (node && node->has_data())
                    node->get_data()->data = value;
                else
                {
                    Variable var;
                    var.data = value;
                    var.type = var_type::const_var;
                    var.name = name;
                    table->add_seq(name, var);
                }

                register_consts(evaluator, args...);
            }

            // 注册一元运算符（带优先级）
            static auto register_prefix(Evaluator &evaluator, const StringType &name, FuncType func,
                                        std::size_t precedence) -> void
            {
                auto &table = evaluator.get_prefixs();
                if (!table)
                    table = std::make_shared<OperPrefixTree>();

                OperPrefix op;
                op.name = name;
                op.arity = 1;
                op.precedence = precedence;
                op.function = func;
                op.function_mode = false;

                table->add_seq(name, op);
            }

            // 注册中缀运算符（带优先级）
            static auto register_infix(Evaluator &evaluator, const StringType &name, FuncType func,
                                       std::size_t precedence) -> void
            {
                auto &table = evaluator.get_infixs();
                if (!table)
                    table = std::make_shared<OperInfixTree>();

                OperInfix op;
                op.name = name;
                op.arity = 2;
                op.precedence = precedence;
                op.function = func;

                table->add_seq(name, op);
            }

            // 注册函数（最高优先级）
            static auto register_function(Evaluator &evaluator, const StringType &name, FuncType func,
                                          std::size_t arity) -> void
            {
                auto &table = evaluator.get_prefixs();
                if (!table)
                    table = std::make_shared<OperPrefixTree>();

                OperPrefix op;
                op.name = name;
                op.arity = arity;
                op.precedence = size_max;
                op.function = func;

                table->add_seq(name, op);
            }
#ifdef CXX_EVAL_ID
    #define ARG_DEFINITION(name) std::shared_ptr<RootVar> *name,std::size_t
#else
    #define ARG_DEFINITION(name) std::shared_ptr<RootVar> *name
#endif
            // 自定义函数
            template <bool is_shared = false>
            static auto register_function(Evaluator &evaluator, const StringType &name,
                                          const std::vector<StringType> &args, const StringType &expression) -> void
            {
                auto &vars{evaluator.get_variables()};

                std::vector<std::pair<StringType, std::shared_ptr<Variable>>> saved;

                auto vargs{std::make_shared<std::vector<std::shared_ptr<Variable>>>()};

                for (const auto &arg : args)
                {
                    auto node{vars->find_seq(arg)};
                    if (node && node->has_data())
                    {
                        saved.emplace_back(arg, node->get_data());
                        vars->remove_seq(arg);
                    }

                    auto param_var{std::make_shared<Variable>()};
                    param_var->type = var_type::mutable_var;
                    vars->add_seq(arg, param_var);
                    vargs->push_back(param_var);
                }

                auto expr{evaluator.template parse<is_shared>(expression)};

                for (const auto &arg : args)
                    vars->remove_seq(arg);

                for (const auto &pair : saved)
                    vars->add_seq(pair.first, pair.second);

                auto func = [expr, vargs](ARG_DEFINITION(params)) mutable -> std::shared_ptr<RootVar>
                {
                    for (std::size_t i(0); i < vargs->size(); ++i)
                        (*vargs)[i]->data = params[i]->data;

                    ConstVar result;
                    result.data = expr.evaluate();
                    return std::make_shared<ConstVar>(result);
                };

                register_function(evaluator, name, func, args.size());
            }

            // 自定义函数（可变参数）
            template <bool is_shared = false, typename... ArgTypes>
            static auto register_function_args(Evaluator &evaluator, const StringType &name, const StringType &expression,
                                          ArgTypes... args) -> void
            {
                std::vector<StringType> arg_list{args...};
                register_function<is_shared>(evaluator, name, arg_list, expression);
            }

            // 一键注册基本算术
            static auto setup_arithmetic(Evaluator &evaluator) -> void
            {
                // 加法
                auto add{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = args[0]->data + args[1]->data;
                             return std::make_shared<ConstVar>(result);
                         }};
                register_infix(evaluator, detail::StringLiteral<CharType>::get("+", L"+"), add, 1);

                // 减法
                auto sub{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = args[0]->data - args[1]->data;
                             return std::make_shared<ConstVar>(result);
                         }};
                register_infix(evaluator, detail::StringLiteral<CharType>::get("-", L"-"), sub, 1);

                // 乘法
                auto mul{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = args[0]->data * args[1]->data;
                             return std::make_shared<ConstVar>(result);
                         }};
                register_infix(evaluator, detail::StringLiteral<CharType>::get("*", L"*"), mul, 2);

                // 除法
                auto div{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = args[0]->data / args[1]->data;
                             return std::make_shared<ConstVar>(result);
                         }};
                register_infix(evaluator, detail::StringLiteral<CharType>::get("/", L"/"), div, 2);

                // 幂运算
                auto pow{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = std::pow(args[0]->data, args[1]->data);
                             return std::make_shared<ConstVar>(result);
                         }};
                register_infix(evaluator, detail::StringLiteral<CharType>::get("^", L"^"), pow, 3);

                // 取模
                auto mod{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = std::fmod(args[0]->data, args[1]->data);
                             return std::make_shared<ConstVar>(result);
                         }};
                register_infix(evaluator, detail::StringLiteral<CharType>::get("%", L"%"), mod, 2);

                // 负号
                auto neg{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = -args[0]->data;
                             return std::make_shared<ConstVar>(result);
                         }};
                register_prefix(evaluator, detail::StringLiteral<CharType>::get("-", L"-"), neg, 2);

                // 正号
                auto pos{[](ARG_DEFINITION(args))
                         {
                             ConstVar result;
                             result.data = +args[0]->data;
                             return std::make_shared<ConstVar>(result);
                         }};
                register_prefix(evaluator, detail::StringLiteral<CharType>::get("+", L"+"), pos, 2);
            }

            // 一键注册三角函数
            static auto setup_trig(Evaluator &evaluator) -> void
            {
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("sin", L"sin"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::sin(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("cos", L"cos"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::cos(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("tan", L"tan"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::tan(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("asin", L"asin"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::asin(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("acos", L"acos"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::acos(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("atan", L"atan"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::atan(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("atan2", L"atan2"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::atan2(args[0]->data, args[1]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    2);
            }

            // 一键注册双曲函数
            static auto setup_hyper(Evaluator &evaluator) -> void
            {
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("sinh", L"sinh"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::sinh(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("cosh", L"cosh"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::cosh(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("tanh", L"tanh"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::tanh(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("asinh", L"asinh"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::asinh(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("acosh", L"acosh"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::acosh(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("atanh", L"atanh"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::atanh(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
            }

            // 一键注册指数对数
            static auto setup_exp_log(Evaluator &evaluator) -> void
            {
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("exp", L"exp"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::exp(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("exp2", L"exp2"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::exp2(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);

                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("ln", L"ln"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::log(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);

                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("log", L"log"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::log(args[1]->data) / std::log(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    2);

                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("log10", L"log10"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::log10(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("log2", L"log2"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::log2(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("log1p", L"log1p"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::log1p(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
            }

            // 一键注册幂根
            static auto setup_power_root(Evaluator &evaluator) -> void
            {
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("sqrt", L"sqrt"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::sqrt(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("cbrt", L"cbrt"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::cbrt(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("hypot", L"hypot"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::hypot(args[0]->data, args[1]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    2);
            }

            // 一键注册取整
            static auto setup_round(Evaluator &evaluator) -> void
            {
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("ceil", L"ceil"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::ceil(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("floor", L"floor"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::floor(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("round", L"round"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::round(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("trunc", L"trunc"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::trunc(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
            }

            // 一键注册杂项
            static auto setup_misc(Evaluator &evaluator) -> void
            {
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("abs", L"abs"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        Type x = args[0]->data;
                        result.data = x < 0 ? -x : x;
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("erf", L"erf"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::erf(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("erfc", L"erfc"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::erfc(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("tgamma", L"tgamma"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::tgamma(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
                register_function(
                    evaluator, detail::StringLiteral<CharType>::get("lgamma", L"lgamma"),
                    [](ARG_DEFINITION(args))
                    {
                        ConstVar result;
                        result.data = std::lgamma(args[0]->data);
                        return std::make_shared<ConstVar>(result);
                    },
                    1);
            }

            // 一键注册所有数学函数
            static auto setup_math(Evaluator &evaluator) -> void
            {
                setup_trig(evaluator);
                setup_hyper(evaluator);
                setup_exp_log(evaluator);
                setup_power_root(evaluator);
                setup_round(evaluator);
                setup_misc(evaluator);
            }

            // 一键注册数学常量
            static auto setup_constants(Evaluator &evaluator) -> void
            {
                register_consts(evaluator, detail::StringLiteral<CharType>::get("pi", L"pi"), std::acos(static_cast<Type>(-1)),
                                detail::StringLiteral<CharType>::get("e", L"e"), std::exp(static_cast<Type>(1)),
                                detail::StringLiteral<CharType>::get("inf", L"inf"), std::numeric_limits<Type>::infinity(),
                                detail::StringLiteral<CharType>::get("nan", L"nan"), std::numeric_limits<Type>::quiet_NaN());
            }

            // 一键注册所有运算功能+常量+解析设置
            static auto setup_allmath(Evaluator &evaluator) -> void
            {
                setup_whitespace(evaluator);
                setup_brackets(evaluator);
                setup_cut(evaluator);
                setup_constant_parser(evaluator);
                setup_arithmetic(evaluator);
                setup_math(evaluator);
                setup_constants(evaluator);
            }

            // 一键注册赋值运算符 =
            static auto setup_assignment(Evaluator &evaluator) -> void
            {
                auto assign{[](ARG_DEFINITION(args)) -> std::shared_ptr<RootVar>
                            {
                                if (args[0]->get_type() != var_type::mutable_var)
                                    throw std::runtime_error("Cannot assign to const variable");
                                args[0]->data = args[1]->data;
                                return args[0];
                            }};
                register_infix(evaluator, detail::StringLiteral<CharType>::get("=", L"="), assign, 0);
            }
#undef ARG_DEFINITION
        };
    };

    template <typename CharT, typename T>
    using basic_eval = eval<std::basic_string<CharT>, T>;
}

#endif