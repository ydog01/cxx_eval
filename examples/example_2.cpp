#include "../include/eval.hpp"
#include <iostream>

int main()
{
    using namespace ydog01;
    using namespace ydog01::eval;
    Evaluator<char,double> eval;
    //现在来试试自己注册
    eval.add_infix("->", [](core::ParamViewer<double> param)//这里其实可以自动推导
    {
        int size = param.size();//获取参数数量,当然了，这个是中缀运算，一般只有两个，但是你熟悉一点的话可以利用core里面的hook弄一些高级语法
        return param[0]*(param[1]-1);

    }, 100);//这个是优先级，设置高一些，加法减法是10，乘除法是20

    //再来试试函数多参数
    eval.add_function("max", [](core::ParamViewer<double>param)->double
    {
        double res = param[0];
        for(auto &i:param)//支持范围迭代的
        {
            if(i>res)
                res = i;
        }
        return res;
    });
    //再来个求和
    eval.add_function("sum", [](core::ParamViewer<double>param)->double
    {
        double res = 0;
        for(auto &i:param)
            res+=i;
        return res;
    });
    constexpr const char* expression = "sum(max(1,2,3),sum(1,max(3)),sin(pi/2)*cos(0))";//这次来个嵌套sum(max(1,2,3),sum(1,max(3)),sin(pi/2)*cos(0)) => sum(3,4,1) => 8
    std::cout<<expression<<" = "<<eval(expression);//嘻嘻忘了。。
    return 0;
}