#include "../include/eval.hpp"
#include <iostream>

int main()
{
    using namespace ydog01;
    using namespace ydog01::eval;
    Evaluator<char,double> eval;//这里构建了求值器,默认会加载所有运算，你可以像这样控制开启
    //我来介绍一下新的内核和用法

    std::cout<<eval("-1+1/2");//重载了括号，可以直接求值

    //这里展示一下自带的函数
    return 0;
}