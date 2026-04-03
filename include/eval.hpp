/*
    C++11 header only
    github:https://github.com/ydog01/cxx_eval
    2026/4/3 -- version 1.0
*/

#ifndef EVAL_HPP
#define EVAL_HPP

#include "eval_core.hpp"
#include "options.hpp"
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace ydog01
{
    namespace eval
    {

        struct OperatorType : core::ExtraData
        {
            enum Kind
            {
                PREFIX,
                INFIX,
                SUFFIX
            } kind;
            OperatorType(Kind k) : kind(k)
            {
            }
        };

        template <typename KeyType, typename DataType>
        class Evaluator
        {
            template <typename KeyT, typename DataT>
            using MapType = std::map<KeyT, DataT>;
            using Context = core::ParserContext<MapType, KeyType, DataType>;

            Context ctx_;

            static auto to_string(const char *str) -> std::basic_string<KeyType>;

        public:
            Evaluator(Options opt = Options::All);

            auto enable_whitespace_skip(bool on = true) -> void;
            auto enable_constant_parser(bool on = true) -> void;
            auto enable_function_call(bool on = true) -> void;

            auto add_variable(const std::basic_string<KeyType> &name, const DataType &val) -> void;
            auto add_variable(const std::basic_string<KeyType> &name, DataType *ptr) -> void;
            auto get_variable(const std::basic_string<KeyType> &name) -> DataType &;
            auto set_variable(const std::basic_string<KeyType> &name, const DataType &val) -> void;
            auto find_variable(const std::basic_string<KeyType> &name) -> DataType *;

            auto add_prefix(const std::basic_string<KeyType> &name,
                            std::function<DataType(core::ParamViewer<DataType>)> func, int prec,
                            core::Associativity assoc = core::Associativity::Right) -> void;

            auto add_infix(const std::basic_string<KeyType> &name,
                           std::function<DataType(core::ParamViewer<DataType>)> func, int prec,
                           core::Associativity assoc = core::Associativity::Left) -> void;

            auto add_suffix(const std::basic_string<KeyType> &name,
                            std::function<DataType(core::ParamViewer<DataType>)> func, int prec,
                            core::Associativity assoc = core::Associativity::Left) -> void;

            auto add_function(const std::basic_string<KeyType> &name,
                              std::function<DataType(core::ParamViewer<DataType>)> func,
                              core::Associativity assoc = core::Associativity::Right) -> void;

            auto remove_variable(const std::basic_string<KeyType> &name) -> bool;
            auto remove_prefix(const std::basic_string<KeyType> &name) -> bool;
            auto remove_infix(const std::basic_string<KeyType> &name) -> bool;
            auto remove_suffix(const std::basic_string<KeyType> &name) -> bool;

            template <template <typename> class PtrType = std::shared_ptr>
            auto parse(const std::basic_string<KeyType> &expr) -> core::Expression<DataType, PtrType>;

            auto evaluate(const std::basic_string<KeyType> &expr) -> DataType;
            auto operator()(const std::basic_string<KeyType> &expr) -> DataType;

            auto add_builtin_operators() -> void;
            auto add_builtin_constants() -> void;
            auto add_builtin_functions() -> void;
        };

        template <typename KeyType, typename DataType>
        Evaluator<KeyType, DataType>::Evaluator(Options opt)
        {
            using Opt = Options;
            if ((opt & Opt::WhitespaceSkip) != Opt::None)
                enable_whitespace_skip();
            if ((opt & Opt::ConstantParser) != Opt::None)
                enable_constant_parser();
            if ((opt & Opt::Parentheses) != Opt::None || (opt & Opt::Comma) != Opt::None)
                enable_function_call();
            if ((opt & Opt::BuiltinOps) != Opt::None)
                add_builtin_operators();
            if ((opt & Opt::BuiltinConstants) != Opt::None)
                add_builtin_constants();
            if ((opt & Opt::BuiltinFuncs) != Opt::None)
                add_builtin_functions();
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::enable_whitespace_skip(bool on) -> void
        {
            if (on)
                ctx_.skip = [](core::ParserInfo<KeyType, DataType> &info) -> bool
                {
                    bool after_left_paren = info.pos && info.keys[info.pos - 1] == static_cast<KeyType>('(');
                    while (info.pos < info.keys.size())
                    {
                        KeyType c = info.keys[info.pos];
                        if (c == static_cast<KeyType>(' ') || c == static_cast<KeyType>('\t') ||
                            c == static_cast<KeyType>('\n') || c == static_cast<KeyType>('\r'))
                            ++info.pos;
                        else
                            break;
                    }
                    if (info.pos == info.keys.size())
                        return true;
                    if (after_left_paren && info.keys[info.pos] == static_cast<KeyType>(')'))
                    {
                        if (!info.stack.empty())
                            info.stack.back().second = 0;
                    }
                    return false;
                };
            else
                ctx_.skip = nullptr;
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::enable_constant_parser(bool on) -> void
        {
            if (on)
                ctx_.constant_parser = [](core::ParserInfo<KeyType, DataType> &info) -> std::unique_ptr<DataType>
                {
                    auto start = info.pos;
                    bool dot = false, digit = false;
                    while (info.pos < info.keys.size())
                    {
                        KeyType c = info.keys[info.pos];
                        if (c >= static_cast<KeyType>('0') && c <= static_cast<KeyType>('9'))
                        {
                            digit = true;
                            ++info.pos;
                        }
                        else if (c == static_cast<KeyType>('.') && !dot)
                        {
                            dot = true;
                            ++info.pos;
                        }
                        else
                            break;
                    }
                    if (!digit)
                    {
                        info.pos = start;
                        return nullptr;
                    }
                    auto val = core::make_unique<DataType>();
                    std::basic_stringstream<KeyType> ss(info.keys.substr(start, info.pos - start));
                    ss >> *val;
                    return val;
                };
            else
                ctx_.constant_parser = nullptr;
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::enable_function_call(bool on) -> void
        {
            using namespace core;
            if (on)
            {
                auto left = std::make_shared<OperatorEx<KeyType, DataType>>();
                left->precedence = std::numeric_limits<int32_t>::max();
                left->assoc = Associativity::Right;
                left->default_param_size = 1;
                left->extra_mid = [](ParserInfo<KeyType, DataType> &,
                                     std::shared_ptr<OperatorEx<KeyType, DataType>>) -> BreakType
                { return BreakType::BREAK; };
                ctx_.resource.insert(static_cast<KeyType>('('))->template set_data<Context::prefix_pos>(left);

                auto right = std::make_shared<OperatorEx<KeyType, DataType>>();
                right->extra_front = [left](ParserInfo<KeyType, DataType> &info) -> bool
                {
                    while (!info.stack.empty())
                    {
                        auto &top = info.stack.back();
                        if (top.first == left)
                            break;
                        info.expression.index.emplace_back(TokenType::Operator);
                        info.expression.operators.emplace_back(top);
                        info.stack.pop_back();
                    }
                    if (info.stack.empty())
                        throw std::runtime_error("Missing left parentheses");
                    auto size = info.stack.back().second;
                    info.stack.pop_back();
                    if (!info.stack.empty() && info.stack.back().first->extra_data)
                    {
                        auto &top = info.stack.back();
                        auto data = dynamic_cast<OperatorType *>(top.first->extra_data.get());
                        if (data && data->kind == OperatorType::PREFIX)
                            top.second = size;
                    }
                    return true;
                };
                ctx_.resource.insert(static_cast<KeyType>(')'))->template set_data<Context::suffix_pos>(right);

                auto comma = std::make_shared<OperatorEx<KeyType, DataType>>();
                comma->precedence = std::numeric_limits<int32_t>::min();
                comma->assoc = Associativity::Left;
                comma->default_param_size = 0;
                comma->extra_front = [left](ParserInfo<KeyType, DataType> &info) -> bool
                {
                    while (!info.stack.empty())
                    {
                        auto &top = info.stack.back();
                        if (top.first == left)
                            break;
                        info.expression.index.emplace_back(TokenType::Operator);
                        info.expression.operators.emplace_back(top);
                        info.stack.pop_back();
                    }
                    if (info.stack.empty())
                        throw std::runtime_error("Missing left parentheses");
                    ++info.stack.back().second;
                    info.value_class = true;
                    return true;
                };
                ctx_.resource.insert(static_cast<KeyType>(','))->template set_data<Context::infix_pos>(comma);
            }
            else
            {
                ctx_.resource.template remove<Context::prefix_pos>(static_cast<KeyType>('('));
                ctx_.resource.template remove<Context::suffix_pos>(static_cast<KeyType>(')'));
                ctx_.resource.template remove<Context::infix_pos>(static_cast<KeyType>(','));
            }
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_variable(const std::basic_string<KeyType> &name, const DataType &val)
            -> void
        {
            ctx_.resource.insert(name)->template set_data<Context::variable_pos>(std::make_shared<DataType>(val));
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_variable(const std::basic_string<KeyType> &name, DataType *ptr) -> void
        {
            ctx_.resource.insert(name)->template set_data<Context::variable_pos>(
                std::shared_ptr<DataType>(ptr, [](DataType *) {}));
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::get_variable(const std::basic_string<KeyType> &name) -> DataType &
        {
            auto node = ctx_.resource.search(name);
            if (!node || !node->template has_data<Context::variable_pos>())
                throw std::runtime_error("Variable not found");
            return *node->template get_data<Context::variable_pos>();
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::set_variable(const std::basic_string<KeyType> &name, const DataType &val)
            -> void
        {
            auto node = ctx_.resource.search(name);
            if (!node || !node->template has_data<Context::variable_pos>())
                throw std::runtime_error("Variable not found");
            *node->template get_data<Context::variable_pos>() = val;
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::find_variable(const std::basic_string<KeyType> &name) -> DataType *
        {
            auto node = ctx_.resource.search(name);
            if (!node || !node->template has_data<Context::variable_pos>())
                return nullptr;
            return node->template get_data<Context::variable_pos>().get();
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_prefix(const std::basic_string<KeyType> &name,
                                                      std::function<DataType(core::ParamViewer<DataType>)> func,
                                                      int prec, core::Associativity assoc) -> void
        {
            auto op = std::make_shared<core::OperatorEx<KeyType, DataType>>();
            op->function = func;
            op->precedence = prec;
            op->assoc = assoc;
            op->default_param_size = 1;
            op->extra_data = core::make_unique<OperatorType>(OperatorType::PREFIX);
            ctx_.resource.insert(name)->template set_data<Context::prefix_pos>(op);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_infix(const std::basic_string<KeyType> &name,
                                                     std::function<DataType(core::ParamViewer<DataType>)> func,
                                                     int prec, core::Associativity assoc) -> void
        {
            auto op = std::make_shared<core::OperatorEx<KeyType, DataType>>();
            op->function = func;
            op->precedence = prec;
            op->assoc = assoc;
            op->default_param_size = 2;
            op->extra_back = [](core::ParserInfo<KeyType, DataType> &info) { info.value_class = true; };
            op->extra_data = core::make_unique<OperatorType>(OperatorType::INFIX);
            ctx_.resource.insert(name)->template set_data<Context::infix_pos>(op);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_suffix(const std::basic_string<KeyType> &name,
                                                      std::function<DataType(core::ParamViewer<DataType>)> func,
                                                      int prec, core::Associativity assoc) -> void
        {
            auto op = std::make_shared<core::OperatorEx<KeyType, DataType>>();
            op->function = func;
            op->precedence = prec;
            op->assoc = assoc;
            op->default_param_size = 1;
            op->extra_data = core::make_unique<OperatorType>(OperatorType::SUFFIX);
            ctx_.resource.insert(name)->template set_data<Context::suffix_pos>(op);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_function(const std::basic_string<KeyType> &name,
                                                        std::function<DataType(core::ParamViewer<DataType>)> func,
                                                        core::Associativity assoc)
            -> void
        {
            auto op = std::make_shared<core::OperatorEx<KeyType, DataType>>();
            op->function = func;
            op->assoc = assoc;
            op->precedence = std::numeric_limits<int32_t>::max();
            op->default_param_size = 1;
            op->extra_data = core::make_unique<OperatorType>(OperatorType::PREFIX);
            ctx_.resource.insert(name)->template set_data<Context::prefix_pos>(op);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::remove_variable(const std::basic_string<KeyType> &name) -> bool
        {
            return ctx_.resource.template remove<Context::variable_pos>(name);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::remove_prefix(const std::basic_string<KeyType> &name) -> bool
        {
            return ctx_.resource.template remove<Context::prefix_pos>(name);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::remove_infix(const std::basic_string<KeyType> &name) -> bool
        {
            return ctx_.resource.template remove<Context::infix_pos>(name);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::remove_suffix(const std::basic_string<KeyType> &name) -> bool
        {
            return ctx_.resource.template remove<Context::suffix_pos>(name);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_builtin_operators() -> void
        {
            add_infix(to_string("+"), [](core::ParamViewer<DataType> a) { return a[0] + a[1]; }, 10);
            add_infix(to_string("-"), [](core::ParamViewer<DataType> a) { return a[0] - a[1]; }, 10);
            add_infix(to_string("*"), [](core::ParamViewer<DataType> a) { return a[0] * a[1]; }, 20);
            add_infix(to_string("/"), [](core::ParamViewer<DataType> a) { return a[0] / a[1]; }, 20);
            add_infix(to_string("%"), [](core::ParamViewer<DataType> a) { return std::fmod(a[0], a[1]); }, 20);
            add_infix(
                to_string("^"), [](core::ParamViewer<DataType> a) { return std::pow(a[0], a[1]); }, 30,
                core::Associativity::Right);
            add_prefix(to_string("+"), [](core::ParamViewer<DataType> a) { return +a[0]; }, 40);
            add_prefix(to_string("-"), [](core::ParamViewer<DataType> a) { return -a[0]; }, 40);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_builtin_constants() -> void
        {
            static DataType pi_val = std::acos(DataType(-1));
            static DataType e_val = std::exp(DataType(1));
            add_variable(to_string("pi"), &pi_val);
            add_variable(to_string("e"), &e_val);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::add_builtin_functions() -> void
        {
            add_function(to_string("sin"), [](core::ParamViewer<DataType> a) { return std::sin(a[0]); });
            add_function(to_string("cos"), [](core::ParamViewer<DataType> a) { return std::cos(a[0]); });
            add_function(to_string("tan"), [](core::ParamViewer<DataType> a) { return std::tan(a[0]); });
            add_function(to_string("asin"), [](core::ParamViewer<DataType> a) { return std::asin(a[0]); });
            add_function(to_string("acos"), [](core::ParamViewer<DataType> a) { return std::acos(a[0]); });
            add_function(to_string("atan"), [](core::ParamViewer<DataType> a) { return std::atan(a[0]); });
            add_function(to_string("atan2"), [](core::ParamViewer<DataType> a) { return std::atan2(a[0], a[1]); });
            add_function(to_string("sinh"), [](core::ParamViewer<DataType> a) { return std::sinh(a[0]); });
            add_function(to_string("cosh"), [](core::ParamViewer<DataType> a) { return std::cosh(a[0]); });
            add_function(to_string("tanh"), [](core::ParamViewer<DataType> a) { return std::tanh(a[0]); });
            add_function(to_string("asinh"), [](core::ParamViewer<DataType> a) { return std::asinh(a[0]); });
            add_function(to_string("acosh"), [](core::ParamViewer<DataType> a) { return std::acosh(a[0]); });
            add_function(to_string("atanh"), [](core::ParamViewer<DataType> a) { return std::atanh(a[0]); });
            add_function(to_string("exp"), [](core::ParamViewer<DataType> a) { return std::exp(a[0]); });
            add_function(to_string("exp2"), [](core::ParamViewer<DataType> a) { return std::exp2(a[0]); });
            add_function(to_string("ln"), [](core::ParamViewer<DataType> a) { return std::log(a[0]); });
            add_function(to_string("log"),
                         [](core::ParamViewer<DataType> a) { return std::log(a[1]) / std::log(a[0]); });
            add_function(to_string("log10"), [](core::ParamViewer<DataType> a) { return std::log10(a[0]); });
            add_function(to_string("log2"), [](core::ParamViewer<DataType> a) { return std::log2(a[0]); });
            add_function(to_string("log1p"), [](core::ParamViewer<DataType> a) { return std::log1p(a[0]); });
            add_function(to_string("sqrt"), [](core::ParamViewer<DataType> a) { return std::sqrt(a[0]); });
            add_function(to_string("cbrt"), [](core::ParamViewer<DataType> a) { return std::cbrt(a[0]); });
            add_function(to_string("hypot"), [](core::ParamViewer<DataType> a) { return std::hypot(a[0], a[1]); });
            add_function(to_string("ceil"), [](core::ParamViewer<DataType> a) { return std::ceil(a[0]); });
            add_function(to_string("floor"), [](core::ParamViewer<DataType> a) { return std::floor(a[0]); });
            add_function(to_string("round"), [](core::ParamViewer<DataType> a) { return std::round(a[0]); });
            add_function(to_string("trunc"), [](core::ParamViewer<DataType> a) { return std::trunc(a[0]); });
            add_function(to_string("abs"), [](core::ParamViewer<DataType> a) { return std::abs(a[0]); });
            add_function(to_string("erf"), [](core::ParamViewer<DataType> a) { return std::erf(a[0]); });
            add_function(to_string("erfc"), [](core::ParamViewer<DataType> a) { return std::erfc(a[0]); });
            add_function(to_string("tgamma"), [](core::ParamViewer<DataType> a) { return std::tgamma(a[0]); });
            add_function(to_string("lgamma"), [](core::ParamViewer<DataType> a) { return std::lgamma(a[0]); });
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::to_string(const char *str) -> std::basic_string<KeyType>
        {
            std::basic_string<KeyType> result;
            while (*str)
                result += static_cast<KeyType>(*str++);
            return result;
        }

        template <typename KeyType, typename DataType>
        template <template <typename> class PtrType>
        auto Evaluator<KeyType, DataType>::parse(const std::basic_string<KeyType> &expr)
            -> core::Expression<DataType, PtrType>
        {
            return ctx_.template parse<std::shared_ptr>(expr);
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::evaluate(const std::basic_string<KeyType> &expr) -> DataType
        {
            return ctx_.template parse<std::shared_ptr>(expr).value();
        }

        template <typename KeyType, typename DataType>
        auto Evaluator<KeyType, DataType>::operator()(const std::basic_string<KeyType> &expr) -> DataType
        {
            return evaluate(expr);
        }
    }
}

#endif