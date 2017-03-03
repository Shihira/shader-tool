# 反射框架

C++为了性能上的需要并不内置反射功能。但是因为要将C++所提供的功能能等价地映射到脚本引擎上，一个反射框架是如此地必要。只是，这个强烈依赖C++模板的反射框架虽然节省开发者很多时间，却将编译时间延长了近3倍。

## 何谓反射

我们对比一下一份功能上等价的C++代码和Scheme代码：

```
//////////////////////////////////////////////////
// C++ code

provided_render_task render_rtask;
render_rtask.set_attributes(main_model);
render_rtask.set_property("illum", propset_illum);
render_rtask.set_property("transfrm", transfrm_mesh);
render_rtask.set_property("camera", main_cam);
render_rtask.set_shader(main_shader);
render_rtask.set_target(main_cam);

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Scheme code

(define render-rtask
  (make-instance shading-rtask '()
    (set-attributes main-model)
    (set-property "illum" propset-illum)
    (set-property-transfrm "transfrm" transfrm-mesh)
    (set-property-camera "camera" main-cam)
    (set-shader main-shader)
    (set-target main-cam)))
```

是的，两份代码看上去是如此相似，甚至Scheme由于它自身元编程过于强大的缘故，看上去还省事儿多了。问题就来了，出于性能的缘故，绝大多数的代码都必须用C++编写。但是出于灵活性的缘故，高视角下的代码需要用Scheme编写。两者功能是等价的。

如果我手动把所有的C++函数都写一个调用wrapper，然后注册到Scheme引擎当中，估计我一年都干不完这事。这个过程必须要是自动的。从Scheme的动态类型，转换到C++的静态类型。从Scheme引擎的函数调用，必须转换到C++的函数调用。内存、类型上要足够安全，而不是每一步存在着被内核发出Segmentation Fault的风险。

这就是反射框架产生的动机。

## 类和函数的注册

这个框架仅适用于面向对象的编程方法，所以它是以“类”为核心进行工作的。包括`int`, `bool`, `uint`, `float`都是类，有各自的成员方法。而我们自己定义的类也是类，我们需要为它们注册成员方法。

```
struct A {
    int a;
    float b;

    void set_a(int i) { a = i; }
    void set_b(float f) { b = f; }
    int get_a() const { return a; }
    int& get_ref_a() { return a; }
    float multiply const { return a * b; }

    // 静态函数也是允许的
    static A new_A(int a_, float b_) {
        A i();
        i.a = a_; i.b = b_;
        return i;
    }
}
```

假设我们有这样一个类A，要怎样为它注册呢？对于Qt这样的大型框架而言，它们有一个外置的语法分析器，从中解析出所有的成员函数并自动注册。这个框架并不会有这样重型的机制。你需要定义个静态成员函数`meta_reg_`：

```
static meta_reg_() {
    meta_manager::reg_class<A>("A")
        .function("set_a", &A::set_a)
        .function("set_b", &A::set_b)
        .function("get_a", &A::get_a)
        .function("get_ref_a", &A::get_ref_a)
        .function("multiply", &A::multiply);
}
```

这里每一个函数都用function注册到了对应`meta`的函数表里面。至于别的工作，比如类型转换，框架会帮你搞定了（笑）。

在注册之后，你可以通过`meta_manager::find_meta`或者`meta_manager::get_meta`来取回元类型信息，前者找不到时返回空指针，后者找不到时产生异常，这是两者的唯一区别。

## 实例（或曰对象）

类是一个静态概念，而实例是一个动态概念。类实例化得到实例（对象）。这是面向对象的基本常识。类的注册基本上在初期就完成了，之后就可以由这些类型去进行实例化。这里介绍`instance`类，它是C++的对象的wrapper：

```
instance ins = instance::make(A());
ins.call("set_a", instance::make(8));
ins.call("set_b", instance::make(7.0f));
instance ref_a = ins.call("get_ref_a"); //这里是取得了a的引用
instance obj_a = ins.call("get_ref_a"); //这里是取得了a的值
ins.set_a(instance::make(6));

assert(ref_a.get<int>() == 6); // 发生了改变
assert(obj_a.get<int>() == 7);
assert(ins.call("multiply").get<float>() == 42.0f);
```

`instance`继承了一个诸多动态语言/解释型语言都有的一个概念：名字不带有类型，但是对象带有。也就是，你可以随时给`instance`用等号赋值，赋什么都可以，这并不会令`instance`原来保管的对象发生改变。真实发生的事情是，`instance`保管的对象被立刻释放。留意本反射框架并冇乜Q垃圾收集器。但是GUILE Scheme虚拟机带有。在进行脚本编写的时候，借用它的GC就好了。如果你不编写脚本，反射也没太大用。

## 元选项

在进行函数注册**之前**，有一些关于类本身的元选项可供使用。

```
meta_manager::reg_class<A>("A")
    .enable_cast<float>()
    .enable_clone()
    .enable_equal()
    .enable_print()
    // ......
    .function("set_a", &A::set_a)
    // ......
```

我们来看看开启了元选项之后有什么有趣的事情发生。

```
instance ins1 = instance::make(A());
instance ins2 = instance::make(A());
// ...

// enable_cast<float>()
// 这个选项会搜索A::operator float() const之类的函数，提供隐式类型转换。
// 比如set_b的参数类型为float，如果不开启这个选项，将会产生运行时类型错误。
ins1.call("set_b", ins2);

// enable_equal<float>()
// 这个选项搜索bool A::operator ==(const A&) const并提供相等的比较。
// 对于Scheme，提供了 (eq? ins1 ins2) 这样的表达式存在。
if(ins1 == ins2) { ... }

// enable_print()
// 这个选项搜索std::ostream& A::operator<<(std::ostream&) const这样的函数。
// 你将可以在Scheme中见到友善的显示，用这样的表达式：(display ins1)
cout << ins1.call("__print").get<string>();

// enable_callable()
// 这个选项搜索void A::operator() ()这样的函数。
ins1.call("__call")

// enable_serialize()
// 这个选项是在序列化时用

// enable_construct<int, float>()
// 这个选项提供了构造函数，并且允许重载。如上面的例子将搜索A::A(int, float)
// 在Scheme当中允许了(make-A 7 2.0)这类构造器的存在
instance ins3 = meta_manager.get_meta<A>().call("__init_2",
        instance::make(7), instance::make(2.0));

// enable_auto_register()
// 这个选项搜索static void A::meta_reg_()这样的函数。
// 它的开启将允许shrtool启动时自动注册该类型而无须手动调用meta_reg_()
```

## 实现注记

要实现这样一个动态弱类型（因为存在大量无警告的隐式类型转换）的反射框架，关键是在于解析函数签名。这个函数签名由模板匹配而获得，可以看到`meta::function`的实现。它将函数签名分割为三个部分，返回类型、对象类型（如果是成员函数的话）以及参数类型，分别有不同的类型转换策略。这些类型转换策略由`arg_type_`和`ret_type_`特化实现（对象类型转换为参数类型）。总的来说思路就是这样。不过这些策略的权衡，倒是很琐碎的。

若反射的函数调用流程出现了不正确的结果，你可以打开`SHRTOOL_LOG_LEVEL=0`的环境变量来查看Debug日志，它会记录所有的函数调用，以及其中的类型、所有的必要信息。因为实在是太琐碎了，所以默认是不会让它显示的。想要观察系统中所有注册的函数，也可以打开这个选项去查看。

元选项的实现主要是在函数表当中添加一些由双下划线开头的函数（这点是抄Python的 括弧笑）。值得一说的是构造函数`__init_n`，N是一个数字，代表了参数的数量。一般而言，重载是不允许的，因为不像编译时候的重载决议，动态调用函数是每一次重载都要花时间去试错，是很不值得的。但是构造函数提供了这一选项，主要是因为构造函数只有一个，为了程序的美观（点头），这样做还是值得的。



