#ifndef OPTIONS_HPP
#define OPTIONS_HPP

namespace ydog01
{
    enum class Options
    {
        None = 0,
        WhitespaceSkip = 1,
        ConstantParser = 2,
        Parentheses = 4,
        Comma = 8,
        BuiltinOps = 16,
        BuiltinConstants = 32,
        BuiltinFuncs = 64,
        All = 127
    };

    inline Options operator|(Options a, Options b)
    {
        return static_cast<Options>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline Options operator&(Options a, Options b)
    {
        return static_cast<Options>(static_cast<int>(a) & static_cast<int>(b));
    }

    inline Options operator^(Options a, Options b)
    {
        return static_cast<Options>(static_cast<int>(a) ^ static_cast<int>(b));
    }

    inline Options operator~(Options a)
    {
        return static_cast<Options>(~static_cast<int>(a));
    }

    inline Options &operator|=(Options &a, Options b)
    {
        a = a | b;
        return a;
    }

    inline Options &operator&=(Options &a, Options b)
    {
        a = a & b;
        return a;
    }

    inline Options &operator^=(Options &a, Options b)
    {
        a = a ^ b;
        return a;
    }
}

#endif