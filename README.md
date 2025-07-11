<h2 id="概述">主要功能</h2>
<table class="api-table">
  <tr><th>1. 基本算术运算</th></tr>
  <tr><th>2. 自定义函数</th></tr>
  <tr><th>3. 变量系统</th></tr>
  <tr><th>4. 运算符优先级</th></tr>
  <tr><th>5. 前缀/中缀/后缀运算符</th></tr>
  <tr><th>6. 常量定义</th></tr>
  <tr><th>7. 复数运算</th></tr>
  <tr><th>8. LaTeX解析(还不稳定)</th></tr>
</table>

<h2 id="功能特性">功能特性</h2>
<ul>
  <li><strong>核心特性</strong>：
    <ul>
      <li>支持自定义类型：运行自己编写规则</li>
      <li>支持实数类型：float/double/long double</li>
      <li>复数运算支持：std::complex&lt;T&gt;</li>
      <li>LaTeX数学表达式解析为计算表达式</li>
      <li>动态符号管理：前缀树存储变量、函数和运算符</li>
      <li>运算符优先级自动处理</li>
    </ul>
  </li>
  <li><strong>扩展能力</strong>：
    <ul>
      <li>自定义函数、运算符和变量</li>
      <li>自由变量与托管变量分离管理</li>
      <li>支持多参数函数（如atan2, log）</li>
    </ul>
  </li>
</ul>

<h2 id="快速开始">快速开始</h2>
<h3>实数计算示例</h3>
<pre><code class="language-cpp">#include "eval_init.hpp"
int main() 
{
  auto calc = eval_init::create_real_eval&lt;double&gt;();
  auto expr = calc.parse("64 + sqrt(16) * sin(pi/2)");
  double result = calc.evaluate(expr); // 结果：64 + 4*1 = 68
}</code></pre>

<h3>复数计算示例</h3>
<pre><code class="language-cpp">#include "eval_init_complex.hpp"
int main() 
{
  auto c_calc = eval_init::create_complex_eval&lt;double&gt;();
  c_calc.vars->insert("z", std::complex&lt;double&gt;(1, 2));
  auto expr = c_calc.parse("z * e^(i*pi) + conj(3+4i)");
  auto result = c_calc.evaluate(expr); // 结果：(-1-2i) + (3-4i) = 2-6i
}</code></pre>

<h3>LaTeX解析示例</h3>
<pre><code class="language-cpp">#include "eval_latex.hpp"
int main() 
{
  eval::latex_parser&lt;char&gt; parser;
  parser.register_variable("x");
  std::string expr = parser.parse("\frac{\sqrt[3]{x}}{2^{n}}");
  // 输出：root(3,x)/(2^(n))
}</code></pre>

<h2 id="核心类说明">核心类说明</h2>
<table class="api-table">
  <tr><th>类名</th><th>功能</th><th>关键成员</th></tr>
  <tr><td>sstree&lt;C,T&gt;</td><td>前缀树符号表</td><td><code>insert()</code>, <code>search()</code>, <code>erase()</code></td></tr>
  <tr><td>evaluator&lt;C,T&gt;</td><td>表达式解析器</td><td><code>parse()</code>, <code>evaluate()</code></td></tr>
  <tr><td>epre&lt;T&gt;</td><td>表达式中间表示</td><td><code>funcs</code>, <code>vars</code>, <code>index</code></td></tr>
  <tr><td>latex_parser&lt;C&gt;</td><td>LaTeX转换器</td><td><code>parse()</code>, <code>register_variable()</code></td></tr>
</table>

<h2 id="表达式语法">表达式语法</h2>
<table class="api-table">
  <tr><th>类别</th><th>语法示例</th><th>说明</th></tr>
  <tr><td>基本运算</td><td><code>3 + 5 * (2 - x)</code></td><td>支持嵌套括号</td></tr>
  <tr><td>函数调用</td><td><code>sin(pi/2)</code></td><td>必须使用括号</td></tr>
  <tr><td>复数运算</td><td><code>(2+3i) * 4i</code></td><td>虚数单位用<code>i</code>后缀</td></tr>
  <tr><td>LaTeX转换</td><td><code>\sqrt{\frac{x}{2}}</code> → <code>sqrt(x/2)</code></td><td>支持分式、根号等</td></tr>
</table>

<h2 id="自定义扩展">自定义扩展</h2>
<h3>1. 添加自定义函数</h3>
<pre><code class="language-cpp">eval::func&lt;double&gt; custom_op
{
  2,              // 参数个数
  eval::size_max, // 最高优先级
  [](const double* args) { return args[0] + std::exp(args[1]); }
};
calc.funcs->insert("custom", custom_op);</code></pre>

<h3>2. 注册变量</h3>
<pre><code class="language-cpp">// 托管变量（值存储）
calc.vars->insert("PI", 3.1415926);

// 自由变量（指针绑定）
double x = 2.0;
calc.vars->insert("x", &x);</code></pre>

<h2 id="错误处理">错误处理</h2>
<div class="warning">
  <ul>
    <li><strong>语法错误</strong>：未闭合括号时抛出<code>size_t</code>指明错误下标</li>
    <li><strong>符号未定义</strong>：未注册变量返回<code>nullptr</code></li>
    <li><strong>运算错误</strong>：抛出std::runtime_error指明错误原因</li>
  </ul>
</div>
<pre><code class="language-cpp">try 
{ 
  auto expr = calc.parse("2 + * 3");
} 
catch (const std::runtime_error& e) 
{ 
  std::cerr << "解析错误: " << e.what(); 
}</code></pre>

<h2 id="性能考虑">性能优化建议</h2>
<div class="note">
  <ul>
    <li><strong>表达式重用</strong>：多次计算时保留<code>epre</code>对象</li>
  </ul>
</div>
<pre><code class="language-cpp">// 预编译表达式
auto expr = calc.parse("x^2 + y^2"); 
for (int i=0; i&lt;1000; i++) 
{ 
  x = i; 
  y = i+1; 
  double result = calc.evaluate(expr); // 避免重复解析 
}</code></pre>

<h2 id="注意事项">关键注意事项</h2>
<div class="warning">
  <ul>
    <li>本项目仅需要标准的cxx_11标准并包含头即可使用</li>
    <li>函数参数分隔符必须使用英文逗号</li>
  </ul>
</div>

<h2 id="高级特性">高级特性示例</h2>
<h3>自定义运算符优先级</h3>
<pre><code class="language-cpp">eval::func&lt;double&gt; mod_op{
  2, 4, 
  [](auto args) { return f(args[0], args[1]); }
};
calc.infix_ops->insert("&", mod_op); // 设置运算优先级为4的中缀运算符</code></pre>