#ifndef EVAL_INIT_HPP
#define EVAL_INIT_HPP
#include "eval.hpp"
#include <cmath>

namespace eval_init
{
    template <typename T>
    T convert(const std::string &);
    
    template <>
    inline float convert<float>(const std::string &str)
    {
        return std::stof(str);
    }

    template <>
    inline double convert<double>(const std::string &str)
    {
        return std::stod(str);
    }

    template <>
    inline long double convert<long double>(const std::string &str)
    {
        return std::stold(str);
    }

    template <>
    inline short convert<short>(const std::string &str)
    {
        return static_cast<short>(std::stoi(str));
    }

    template <>
    inline int convert<int>(const std::string &str)
    {
        return std::stoi(str);
    }

    template <>
    inline long convert<long>(const std::string &str)
    {
        return std::stol(str);
    }

    template <>
    inline long long convert<long long>(const std::string &str)
    {
        return std::stoll(str);
    }

    template <>
    inline unsigned short convert<unsigned short>(const std::string &str)
    {
        return static_cast<unsigned short>(std::stoul(str));
    }

    template <>
    inline unsigned int convert<unsigned int>(const std::string &str)
    {
        return static_cast<unsigned int>(std::stoul(str));
    }

    template <>
    inline unsigned long convert<unsigned long>(const std::string &str)
    {
        return std::stoul(str);
    }

    template <>
    inline unsigned long long convert<unsigned long long>(const std::string &str)
    {
        return std::stoull(str);
    }

    template <typename T>
    eval::evaluator<char, T> create_real_eval()
    {
        using namespace eval;
        evaluator<char, T> calc(
            [](const std::string &str, size_t &pos, epre<T> &expr) -> bool
            {
                if (str[pos] < '0' || str[pos] > '9')
                    return false;
                size_t start = pos;
                while (pos < str.size() && str[pos] >= '0' && str[pos] <= '9')
                    pos++;
                if (str[pos] == '.' && str[pos + 1] >= '0' && str[pos + 1] <= '9')
                    do
                        pos++;
                    while (pos < str.size() && str[pos] >= '0' && str[pos] <= '9');
                if (str[pos] == 'e' || str[pos] == 'E')
                {
                    pos++;
                    if (str[pos] == '+' || str[pos] == '-')
                        pos++;
                    while (pos < str.size() && str[pos] >= '0' && str[pos] <= '9')
                        pos++;
                }
                expr.consts.push_back(convert<T>(str.substr(start, pos - start)));
                expr.index += 'c';
                return true;
            });

        // 注册基本运算符
        func<T> add_op{2, 1, [](const T *args)
                       { return args[0] + args[1]; }};
        func<T> sub_op{2, 1, [](const T *args)
                       { return args[0] - args[1]; }};
        func<T> mul_op{2, 2, [](const T *args)
                       { return args[0] * args[1]; }};
        func<T> div_op{2, 2, [](const T *args)
                       { return args[0] / args[1]; }};
        func<T> pow_op{2, 3, [](const T *args)
                       { return std::pow(args[0], args[1]); }};
        func<T> mod_op{2, 2, [](const T *args)
                       { return std::fmod(args[0], args[1]); }};
        func<T> neg_op{1, 2, [](const T *args)
                       { return -args[0]; }};
        func<T> aff_op{1, 2, [](const T *args)
                       { return args[0]; }};

        calc.infix_ops->insert("+", add_op);
        calc.infix_ops->insert("-", sub_op);
        calc.infix_ops->insert("*", mul_op);
        calc.infix_ops->insert("/", div_op);
        calc.infix_ops->insert("^", pow_op);
        calc.infix_ops->insert("%", mod_op);
        calc.prefix_ops->insert("-", neg_op);
        calc.prefix_ops->insert("+", aff_op);

        // 注册数学函数
        func<T> sin_op{1, size_max, [](const T *args)
                       { return std::sin(args[0]); }};
        func<T> cos_op{1, size_max, [](const T *args)
                       { return std::cos(args[0]); }};
        func<T> tan_op{1, size_max, [](const T *args)
                       { return std::tan(args[0]); }};
        func<T> asin_op{1, size_max, [](const T *args)
                        { return std::asin(args[0]); }};
        func<T> acos_op{1, size_max, [](const T *args)
                        { return std::acos(args[0]); }};
        func<T> atan_op{1, size_max, [](const T *args)
                        { return std::atan(args[0]); }};
        func<T> atan2_op{2, size_max, [](const T *args)
                         { return std::atan2(args[0], args[1]); }};
        func<T> sinh_op{1, size_max, [](const T *args)
                        { return std::sinh(args[0]); }};
        func<T> cosh_op{1, size_max, [](const T *args)
                        { return std::cosh(args[0]); }};
        func<T> tanh_op{1, size_max, [](const T *args)
                        { return std::tanh(args[0]); }};
        func<T> asinh_op{1, size_max, [](const T *args)
                         { return std::asinh(args[0]); }};
        func<T> acosh_op{1, size_max, [](const T *args)
                         { return std::acosh(args[0]); }};
        func<T> atanh_op{1, size_max, [](const T *args)
                         { return std::atanh(args[0]); }};
        func<T> log_op{2, size_max, [](const T *args)
                       { return std::log(args[1]) / std::log(args[0]); }};
        func<T> lg_op{1, size_max, [](const T *args)
                      { return std::log10(args[0]); }};
        func<T> ln_op{1, size_max, [](const T *args)
                      { return std::log(args[0]); }};
        func<T> log2_op{1, size_max, [](const T *args)
                        { return std::log2(args[0]); }};
        func<T> sqrt_op{1, size_max, [](const T *args)
                        { return std::sqrt(args[0]); }};
        func<T> cbrt_op{1, size_max, [](const T *args)
                        { return std::cbrt(args[0]); }};
        func<T> abs_op{1, size_max, [](const T *args)
                       { return std::abs(args[0]); }};
        func<T> exp_op{1, size_max, [](const T *args)
                       { return std::exp(args[0]); }};
        func<T> exp2_op{1, size_max, [](const T *args)
                        { return std::exp2(args[0]); }};
        func<T> ceil_op{1, size_max, [](const T *args)
                        { return std::ceil(args[0]); }};
        func<T> floor_op{1, size_max, [](const T *args)
                         { return std::floor(args[0]); }};
        func<T> round_op{1, size_max, [](const T *args)
                         { return std::round(args[0]); }};
        func<T> trunc_op{1, size_max, [](const T *args)
                         { return std::trunc(args[0]); }};
        func<T> erf_op{1, size_max, [](const T *args)
                       { return std::erf(args[0]); }};
        func<T> erfc_op{1, size_max, [](const T *args)
                        { return std::erfc(args[0]); }};
        func<T> tgamma_op{1, size_max, [](const T *args)
                          { return std::tgamma(args[0]); }};
        func<T> lgamma_op{1, size_max, [](const T *args)
                          { return std::lgamma(args[0]); }};
        func<T> hypot_op{2, size_max, [](const T *args)
                         { return std::hypot(args[0], args[1]); }};
        func<T> root_op{2, size_max, [](const T *args)
                        { return std::pow(args[1], T(1) / args[0]); }};
        func<T> min_op{2, size_max, [](const T *args)
                       { return std::min(args[0], args[1]); }};
        func<T> max_op{2, size_max, [](const T *args)
                       { return std::max(args[0], args[1]); }};

        calc.funcs->insert("sin", sin_op);
        calc.funcs->insert("cos", cos_op);
        calc.funcs->insert("tan", tan_op);
        calc.funcs->insert("asin", asin_op);
        calc.funcs->insert("acos", acos_op);
        calc.funcs->insert("atan", atan_op);
        calc.funcs->insert("atan2", atan2_op);
        calc.funcs->insert("sinh", sinh_op);
        calc.funcs->insert("cosh", cosh_op);
        calc.funcs->insert("tanh", tanh_op);
        calc.funcs->insert("asinh", sinh_op);
        calc.funcs->insert("acosh", cosh_op);
        calc.funcs->insert("atanh", tanh_op);
        calc.funcs->insert("log", log_op);
        calc.funcs->insert("lg", lg_op);
        calc.funcs->insert("ln", ln_op);
        calc.funcs->insert("log2", log2_op);
        calc.funcs->insert("sqrt", sqrt_op);
        calc.funcs->insert("cbrt", cbrt_op);
        calc.funcs->insert("abs", abs_op);
        calc.funcs->insert("exp", exp_op);
        calc.funcs->insert("exp2", exp2_op);
        calc.funcs->insert("ceil", ceil_op);
        calc.funcs->insert("floor", floor_op);
        calc.funcs->insert("round", round_op);
        calc.funcs->insert("trunc", trunc_op);
        calc.funcs->insert("erf", erf_op);
        calc.funcs->insert("erfc", erfc_op);
        calc.funcs->insert("tgamma", tgamma_op);
        calc.funcs->insert("lgamma", lgamma_op);
        calc.funcs->insert("hypot", hypot_op);
        calc.funcs->insert("root", root_op);
        calc.funcs->insert("min", min_op);
        calc.funcs->insert("max", max_op);

        // 注册数学常量
        calc.vars->insert("pi", std::acos(T(-1)));
        calc.vars->insert("e", std::exp(T(1)));
        calc.vars->insert("inf", std::numeric_limits<T>::infinity());
        calc.vars->insert("nan", std::numeric_limits<T>::quiet_NaN());

        return calc;
    }
}
#endif