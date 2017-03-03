# 单元测试框架

单元测试是保证软件质量的重要环节，所以一个“至少够用”的单元测试框架是必不可少的。另外，本框架还提供了特有的、测试图形程序的单元测试策略(`shrtool/tests/test_utils.h`)，这是其它测试框架所不具备的。

## 入门

举例我们要测试这样一个函数：

```cpp
// factorial.h
inline int factorial(int n)
{
    if(n <= 1) return 1;
    return n * factorial(n - 1);
}
```

这是一个求阶乘的函数。为了验证它的正确性，我们要验证它是否与我们预期的结果相符合。建立一个测试文件，并且一般而言，一个测试文件就是一个测试套件（Test Suite）。在包含`unit_test.h`之前，先定义一个宏`TEST_SUITE`为这个测试套件的名字。

```cpp
// test.cc
#define TEST_SUITE "Factorial"

#include "unit_test.h"
#include "factorial.h"

using namespace std;
using namespace shrtool;

TEST_CASE(test_factorial) {
    assert_equal(factorial(5), 120L);
    assert_equal(factorial(6), 720L);
    assert_equal(factorial(7), 5040L);
}

int main(int argc, char* argv[])
{
    return unit_test::test_main(argc, argv);
}
```

编译这个文件并且运行：

```cpp
$ g++ test.cc -o test
$ ./test
Running test_factorial...Passed
```

## 更有效地测试

可以看到，我们刚才通过了所有的断言，也就是通过了测试。这时我们加强测试的强度。我们要求这个阶乘函数能够支持到 n < 21 的结果计算 —— 在这个简单的例子里面，我们不要求它能算到超出机器表示的结果。

```cpp
TEST_CASE(test_big_number_factorial) {
    assert_equal(factorial(19), 121645100408832000L);
    assert_equal(factorial(20), 2432902008176640000L);
}
```

这次的测试则出现了失败。

```cpp
$ g++ test.cc -o test
$ ./test
Running test_factorial...Passed
Running test_big_number_factorial...Failed: factorial(19) != 121645100408832000L
```

但是我对为什么会失败还没有任何的头绪。我仅仅是收到了失败这一条信息而已。要想得到更加具有针对性的修复，我想知道等号两边的两个值究竟是什么。我们引入`assert_equal_print`这一个功能：

```cpp
TEST_CASE(test_big_number_factorial) {
    assert_equal_print(factorial(19), 121645100408832000L);
    assert_equal_print(factorial(20), 2432902008176640000L);
}
```

对于任何能输出到C++流的内容，`assert_equal_print`会把失败的值打印出来。而对于不能打印但是使用了它的，它会引发一个编译错误，以指导你去修改。

```cpp
Running test_big_number_factorial...Failed: factorial(19)(109641728)
    != 121645100408832000L(121645100408832000)
```

在看到这个反常的小的数字之后，我应该知道原因了。我用的数据类型太小了。把`int`改成`long long int`，测试便通过了。除却这两个之外，本框架还提供了其它一系列非常有用的断言：

## 在异常中测试

在测试当中我们经常可能会被异常砸中。区区异常可不能把整个测试崩溃掉。

```cpp
//你们自己本身也判断，一分钟有多少秒，识得唔识得啊
inline void judge_things_by_yourselves(int seconds_in_one_minute)
{
    if(seconds_in_one_minute == 59)
        throw temper_error("I'm angry. 我今天算得罪了你们一下。");
}

// test.cc
TEST_CASE(test_exceptions) {
    judge_things_by_yourselves(60);
    judge_things_by_yourselves(59);
}

```

编译运行的结果如下：

```cpp
$ ./test
Running test_factorial...Passed
Running test_big_number_factorial...Passed
Running test_exceptions...Error: I'm angry. 我今天算得罪了你们一下。
```

但是在这个情况下，可以说我们是明知这个异常会发生，或者说我们是特意去触发它，以测试这个程序的健壮性。这时我们可以采用`assert_except`来Guard住这个语句。比如说，我预设这个地方会出现一个`temper_error`：

```cpp
TEST_CASE(test_exceptions) {
    assert_no_except(judge_things_by_yourselves(60));
    assert_except(judge_things_by_yourselves(59), temper_error);
}
```

这样编写的测试便符合语义，并且成功通过了。

## 使用ctest

我们知道C++标准库提供了一系列的流（`cout, cin, cerr`）来方便我们格式化输出输入。`ctest`就是类似这样的一个流，它存在的目的是想要获取测试当中的输出。但是它又有些别样的特性。

```cpp
TEST_CASE(test_use_ctest) {
    ctest << "Output something to ctest" << endl;
    ctest << "Output something to ctest" << endl;
    ctest << "Output something to ctest" << endl;
}

TEST_CASE(test_use_cout) {
    cout << "Output something to cout" << endl;
    cout << "Output something to cout" << endl;
    cout << "Output something to cout" << endl;
}
```

我们把`test_use_ctest`放到测试样例们的中间而不是末尾，以使得效果对比更加清晰：

```cpp
$ ./test

Running test_factorial...Passed
Running test_use_ctest...Passed
Running test_use_cout...Output something to cout
Output something to cout
Output something to cout
Passed
Running test_big_number_factorial...Passed
Running test_exceptions...Passed
----- Factorial/test_use_ctest -----
Output something to ctest
Output something to ctest
Output something to ctest
```

可以看到，直接从`cout`输出大量数据的话，整个测试的输出日志会被打乱，嵌入很多无用的信息。而从ctest输出，则直到末尾一次性输出。这样整个测试日志会很简洁、便于查看。

## 使用Fixture

Fixture是固定在一个测试样例的上的__配置__。我们先声明一个fixture类，然后用`TEST_CASE_FIXTURE`将它绑定到一个Test Case上。


```cpp
struct some_fixture {
    FILE* fid;
    static constexpr char FIRST = 'a';
    static constexpr char SECOND = 'b';

    some_fixture() {
        fid = fopen("a.txt", "w+");
        fprintf(fid, "abc");
        fseek(fid, 0, SEEK_SET);
    }

    ~some_fixture() {
        fclose(fid);
        remove("a.txt");
    }
};

TEST_CASE_FIXTURE(test_read_file, some_fixture) {
    assert_equal(fgetc(fid), FIRST);
    assert_equal(fgetc(fid), SECOND);
}
```

之后有很多依赖于一个特定格式的文件的函数就可以复用这个fixture进行测试。可以看见`fid, FRIST, SECOND`这些变量不会存在于全局产生名字污染，同时也能很直接得访问到。另外，fixture的构造函数和析构函数分别进行了资源的申请和回收，使得每一次的测试都是__幂等的__（也就是不会因为重复多次测试而导致结果不相同）。

## 使用runner自定义测试流程

你可以通过直接访问`runner`来自定义测试流程，而不是让默认的runner将全部的都完整测试。玩法有多种多样，这里举例我们可以用一个正则表达式的规则来筛选哪些要进行测试哪些不用。

```cpp
#include <regex>

bool custom_runner(const unit_test::test_case& tc) {
    if(regex_match(tc.name, regex(".*factorial.*")))
        return unit_test::test_context::default_runner(tc);
    else {
        std::cout << "Skipped" << std::endl;
        return true;
    }
}

int main(int argc, char* argv[])
{
    unit_test::test_context::inst().runner = custom_runner;

    return unit_test::test_main(argc, argv);
}
```

下面是编译运行的输出，可以看到，只有名字带factorial的样例才被运行了。

```cpp
Running test_factorial...Passed
Running test_use_ctest...Skipped
Running test_use_cout...Skipped
Running test_big_number_factorial...Passed
Running test_exceptions...Skipped
Running test_read_file...Skipped
```

## 与其它测试框架的区别

这个测试框架从思想上跟CPPUnit或者Boost.Test并没有太大的区别，虽然从形式上有不同，也只是实现上的区别。相比起CPPUnit，它更带C++风格的简约。CPPUnit过分注重接近JUnit的接口模式，但是因为C++原生不支持反射的缘故，对于实现者和用户而言都非常吃力和繁琐。CPPUnit也采取了很多Java风格的设计模式，这在一定程度上与一个纯粹的C++项目格格不入。另外，本框架只有数百行，仅有头文件，所以非常方便小型项目的使用，不必引入外部的目标文件，对于编译速度也有大大的提升。从刚才的示例来看，编译的命令是如此简单，只有一个输入文件和一个输出文件。这是一个动则数十个文件的大框架所没有的优势 —— 开箱即用。

另外，由于测试样例是自动添加的，所以可以通过链接，来将两个Test Suite直接链接起来就可以联合测试，而原本的两个测试文件，几乎完全不需要改动（当然，你还是需要删掉多余的`main`函数不是么）。

```cpp
$ g++ test_suite1.cc -c -o test_suite1.o
$ g++ test_suite2.cc -c -o test_suite2.o
$ g++ test_suite1.o test_suite2.o -o combination
$ ./combination 
Running test_from_suite1...Passed
Running test_from_suite2...Passed
----- Suite1/test_from_suite1 -----
Greetings From Suite1
----- Suite2/test_from_suite2 -----
Greetings From Suite2
```

