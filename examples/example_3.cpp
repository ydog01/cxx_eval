#include "../include/eval.hpp"
#include <iostream>

int main()
{
    using namespace ydog01;
    using namespace ydog01::eval;
    Evaluator<char,double> eval;
    //然后就是变量啦
    eval.add_variable("x",3);//这个就是直接创建变量
    double MyVar = 0;
    eval.add_variable("MyVar",&MyVar);//这个是外部变量导入

    constexpr const char* expression = "x+MyVar*MyVar";//3+0 = 3呃呃我是笨蛋
    std::cout<<expression<<" = "<<eval(expression)<<std::endl;//忘记换行了
    MyVar = 5;//3+5*5 = 28
    std::cout<<expression<<" = "<<eval(expression)<<std::endl;

    //如何获取已经导入的变量
    auto& x=*eval.find_variable("x");//这样子就得到啦
    x = 5;
    std::cout<<expression<<" = "<<eval(expression);//5+5*5= 30

    //当然了后缀前缀（其实函数就是前缀）你也是可以注册的，也可以查询删除修改
    //这个主要是核心比较重要，你可以按照core写出不同求值器（语法复杂点可以写解释器的）
    //刚才用了这么多C指针是因为其实eval.hpp只是测试用的，core才比较重要
    //之后我也会不断更新这个库，加入符号计算，高精度计算这些(其实吧core里面就是按照一定规则构建了语法树)
    //来看看怎么实现高级功能
    //就看到这里了吧，时间关系=w=
    return 0;
}