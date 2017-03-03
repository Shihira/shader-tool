# 脚本引擎

灵活和结构清晰是本框架的第一要义。因此我需要一种脚本语言来控制逻辑。

## 为什么是GUILE Scheme

首先是为什么是Scheme。我初初只是想用XML，它是一种灵活的方式去定义数据，还可以用Xinclude来达到复用，等等。不过，在调研适用性的途中发现，XML简直是魔幻。正所谓它想调和计算机可解析性和人类可读性，然后发明了一种对计算机来说完全不好解析，而且对人类来说相当不可读（而且写起来相当冗长）的方案。详见各种XML Sucks。至于JSON之类的确实从开始就没有入过眼，因为它除了相对XML而言比较可读，一点都不灵活，没什么优点。

于是S-Expression就是一个很好的选项了。语法十分易学（基本上只有括号，顶多就是quote要理解一下），表达树状数据也是一流。对于计算机来说也是解析也非常轻松，基本上对于一个语言来说，很少有哪个是可以在词法分析阶段就已经完成解析了。另外抽象能力也是一流，而且可以自定义语法（很多人觉得括号党编起程来不太舒服，但是通过适当地定义宏，是可以得到很强的可读性的）。

于是我考虑过Clojure（带一个java虚拟机太大了）、Common Lisp（流派太多）等等，最后决定采用Scheme。它有不少极其轻量的脚本引擎，TinyScheme、Chibi之类，可惜他们实在就太久没有更新了。GUILE在这种情况下成为了唯一选择：有GNU巨头在后面支持，软件应该是很稳定的。而且体积是如此之小，在Fedora的官方仓库中，guile只有3.5M，guile-devel只有135k。要知道，guile已经包含了完整srfi标准库的内容。

如果要问为什么不试着采用Python, Javascript等流行语言？答案就是他们实在是太大了。而且也远远达不到S-expr那种数据即代码、代码即数据的理念。

## REPL

REPL（Read-Eval-Print-Loop）永远是最令人激动的特性，它诠释了无数的可能。`shrtool_repl`提供了与guile有大部分功能的REPL。在它的主循环当中，是这样的逻辑：检查标准输入 -> 对输入表达式求值 -> 对`main-rtask`所指向的Render Task进行渲染 -> 检查标准输入。换言之，标准输入的求值是在两次渲染的间隙中进行的。

有没有必要引入多线程呢？乍看这种情景之下，多线程似乎非常适用。但是事实上，在渲染的同时进行数据更新是非常危险的事情，造成十分不稳定的结果。再者，在渲染途中，OpenGL Context基本上都是锁在一个线程上的，所以就算有多个线程，也起不到多线程的作用。如果有大规模的CPU计算，尽量在启动渲染之前就算好而不是在渲染途中去算，而且尽量用C++而不是Scheme去算，应该遵循这样的原则。所以在这样的考虑下，引入多线程只是徒增不稳定性，对功能毫无提升，是下下策。

我们知道Lisp最为人诟病的就是括号，但是`shrtool_repl`有这样的功能：但凡它读取标准输入，第一个符号是函数或者宏，则自动添加两边的括号。如果有想特别显示的函数或宏，你应该使用display函数。

```
> cos (* pi 2) ; no need to write (cos (* pi 2))
$1 = 1.0
> / (+ $1 11) 3
$2 = 4.0
> system "echo hello world"
hello world
$3 = 0
> display "hello world\n"
hello world
```

返回值若不是`#nil, #<undefined>, #<unspecified>`，则会打印这个返回值，并且将其用一个`$n`记录下来。这是REPL当中十分有用的功能。运行过程中触发的异常/错误，会中断执行，然后返回错误。目前回溯调试的功能还未实现（TODO）

```
> mesh-gen-box 1 2
Uncaught exception: (cpp-error restriction_error Number of arguments does not match: 3 required, 2 provided)
```

还有一个从GDB抄过来的特性。当你输入一个空行的时候，它将会重复你的上一条输入。这解决了控制台无法连续调整参数的弊病。比如下面的输入，这样执行过后它事实上转了`pi / 40`的角度。

```
> $- transfrm-mesh : rotate (/ pi 120) 'zOx
>
>
```

## 函数映射

Scheme引擎中提供的功能虽然是C++中功能的一个子集（为了降低复杂度而隐藏了一些细节），但是是足够强大和完善的。一般来说，C++中的函数映射到Scheme当中遵循这样的规则：

- 所有的下划线转换为短横杠（减号）
- 如果是构造函数，则注册为`make-<Class Name>`，如`make-propset`就是`propset::propset()`
- 如果是成员函数，则注册为`<Class Name>-<Function Name>`，如`mesh-gen-box`就是`mesh::gen_box(...)`。除了`builtin`中的函数，它们不会有Class Name前缀。

下面是映射的列表。

 Reflection Name | Register Name
-----------------|---------------
`shading_rtask::set_attributes` | `shading-rtask-set-attributes`
`shading_rtask::set_property` | `shading-rtask-set-property`
`shading_rtask::set_property_camera` | `shading-rtask-set-property-camera`
`shading_rtask::set_property_transfrm` | `shading-rtask-set-property-transfrm`
`shading_rtask::set_shader` | `shading-rtask-set-shader`
`shading_rtask::set_target` | `shading-rtask-set-target`
`shading_rtask::set_texture` | `shading-rtask-set-texture`
`shading_rtask::set_texture2d_image` | `shading-rtask-set-texture2d-image`
`shading_rtask::set_texture_cubemap_image` | `shading-rtask-set-texture-cubemap-image`
`texture_cubemap::__init_1` | `make-texture-cubemap`
`builtin::image_from_ppm` | `image-from-ppm`
`builtin::image_save_ppm` | `image-save-ppm`
`builtin::instance_get_type` | `instance-get-type`
`builtin::instance_search_function` | `instance-search-function`
`builtin::make_propset` | `make-propset`
`builtin::make_shading_rtask` | `make-shading-rtask`
`builtin::meshes_from_wavefront` | `meshes-from-wavefront`
`builtin::search_function` | `search-function`
`builtin::set_log_level` | `set-log-level`
`builtin::shader_from_config` | `shader-from-config`
`builtin::texture_to_image` | `texture-to-image`
`render_target::attach_texture` | `render-target-attach-texture`
`render_target::clear_buffer` | `render-target-clear-buffer`
`render_target::get_bgcolor` | `render-target-get-bgcolor`
`render_target::get_blend_func` | `render-target-get-blend-func`
`render_target::get_depth_test` | `render-target-get-depth-test`
`render_target::get_draw_face` | `render-target-get-draw-face`
`render_target::get_infdepth` | `render-target-get-infdepth`
`render_target::get_viewport` | `render-target-get-viewport`
`render_target::get_wire_frame` | `render-target-get-wire-frame`
`render_target::is_screen` | `render-target-is-screen`
`render_target::screen` | `render-target-screen`
`render_target::set_bgcolor` | `render-target-set-bgcolor`
`render_target::set_blend_func` | `render-target-set-blend-func`
`render_target::set_depth_test` | `render-target-set-depth-test`
`render_target::set_draw_face` | `render-target-set-draw-face`
`render_target::set_infdepth` | `render-target-set-infdepth`
`render_target::set_viewport` | `render-target-set-viewport`
`render_target::set_wire_frame` | `render-target-set-wire-frame`
`texture2d::__init_2` | `make-texture2d`
`camera::__init_0` | `make-camera`
`camera::calc_projection_mat` | `camera-calc-projection-mat`
`camera::calc_vp_mat` | `camera-calc-vp-mat`
`camera::get_aspect` | `camera-get-aspect`
`camera::get_far_clip_plane` | `camera-get-far-clip-plane`
`camera::get_near_clip_plane` | `camera-get-near-clip-plane`
`camera::get_view_mat` | `camera-get-view-mat`
`camera::get_view_mat_inv` | `camera-get-view-mat-inv`
`camera::get_visible_angle` | `camera-get-visible-angle`
`camera::set_aspect` | `camera-set-aspect`
`camera::set_auto_viewport` | `camera-set-auto-viewport`
`camera::set_far_clip_plane` | `camera-set-far-clip-plane`
`camera::set_near_clip_plane` | `camera-set-near-clip-plane`
`camera::set_transformation` | `camera-set-transformation`
`camera::set_visible_angle` | `camera-set-visible-angle`
`camera::transformation` | `camera-transformation`
`texture::get_format` | `texture-get-format`
`texture::get_height` | `texture-get-height`
`texture::get_width` | `texture-get-width`
`texture::reserve` | `texture-reserve`
`texture::set_format` | `texture-set-format`
`texture::set_height` | `texture-set-height`
`texture::set_width` | `texture-set-width`
`image::extract_cubemap` | `image-extract-cubemap`
`image::flip_h` | `image-flip-h`
`image::flip_v` | `image-flip-v`
`image::height` | `image-height`
`image::pixel` | `image-pixel`
`image::width` | `image-width`
`fcolor::a` | `fcolor-a`
`fcolor::b` | `fcolor-b`
`fcolor::g` | `fcolor-g`
`fcolor::r` | `fcolor-r`
`fcolor::rgba` | `fcolor-rgba`
`transfrm::__init_0` | `make-transfrm`
`transfrm::get_inverse_mat` | `transfrm-get-inverse-mat`
`transfrm::get_mat` | `transfrm-get-mat`
`transfrm::rotate` | `transfrm-rotate`
`transfrm::scale` | `transfrm-scale`
`transfrm::translate` | `transfrm-translate`
`color::__init_1` | `make-color`
`color::a` | `color-a`
`color::b` | `color-b`
`color::from_rgba` | `color-from-rgba`
`color::from_string` | `color-from-string`
`color::from_value` | `color-from-value`
`color::g` | `color-g`
`color::r` | `color-r`
`color::rgba` | `color-rgba`
`mesh::gen_box` | `mesh-gen-box`
`mesh::gen_plane` | `mesh-gen-plane`
`mesh::gen_uv_sphere` | `mesh-gen-uv-sphere`
`mesh::get_normals` | `mesh-get-normals`
`mesh::get_position` | `mesh-get-position`
`mesh::get_uv` | `mesh-get-uv`
`mesh::has_normals` | `mesh-has-normals`
`mesh::has_positions` | `mesh-has-positions`
`mesh::has_uvs` | `mesh-has-uvs`
`mesh::triangles` | `mesh-triangles`
`mesh::vertices` | `mesh-vertices`
`propset::__init_0` | `make-propset`
`propset::append` | `propset-append`
`propset::append-float` | `propset-append-float`
`propset::definition` | `propset-definition`
`propset::get` | `propset-get`
`propset::resize` | `propset-resize`
`propset::set` | `propset-set`
`propset::set_float` | `propset-set-float`
`propset::size_in_bytes` | `propset-size-in-bytes`
`rect::__init_2` | `make-rect`
`rect::area` | `rect-area`
`rect::from_size` | `rect-from-size`
`rect::get_size` | `rect-get-size`
`rect::height` | `rect-height`
`rect::regulate` | `rect-regulate`
`rect::width` | `rect-width`
`queue_rtask::__init_0` | `make-queue-rtask`
`queue_rtask::append` | `queue-rtask-append`
`queue_rtask::clear` | `queue-rtask-clear`
`queue_rtask::get_prof_enabled` | `queue-rtask-get-prof-enabled`
`queue_rtask::print_profile_log` | `queue-rtask-print-profile-log`
`queue_rtask::remove` | `queue-rtask-remove`
`queue_rtask::set_prof_enabled` | `queue-rtask-set-prof-enabled`
`queue_rtask::size` | `queue-rtask-size`
`queue_rtask::sort` | `queue-rtask-sort`
`rtask::__init_0` | `make-rtask`
`rtask::is_successor` | `rtask-is-successor`
`rtask::reclaim` | `rtask-reclaim`
`rtask::rely_on` | `rtask-rely-on`
`rtask::render` | `rtask-render`
`proc_rtask::__init_0` | `make-proc-rtask`
`proc_rtask::set_proc` | `proc-rtask-set-proc`
`shader::gen_source` | `shader-gen-source`

## 新增的语法

通过`define-syntax`，我新增了一些方便面向对象操作的语法。

```
(make-instance <Class Name> '(<Constructor Parameters> ...)
  (<Function Name> <Parameters> ...)
  (<Function Name> <Parameters> ...)
  (<Function Name> <Parameters> ...) ...)

;;;; 例如
(define cam-transfrm
  (make-instance transfrm '()
    (translate 0 0 20)
    (rotate (/ pi 8) 'yOz)
    (rotate (/ pi 12) 'zOx)))
```

这省去了写类名的麻烦。

另外一个是dollar语法。

```
($ <Instance> : <Member Function Name> <Parameters> ...)
($- <Instance> : <Member Function Name> <Parameters> ...)

;;;; 后者会将返回值消除
($ screen-rtask : set-texture "bumpMap" tex)
($- ($ light-cam : transformation) : rotate angle 'zOx)))
```

## 复用资源

为了方便开发，库中的复用资源在未来会进一步增加。目前值得说明的是`shader-def.scm`和`rtask-def.scm`中的预定义数据。

- `shader-def-attr-mesh`是非常经典的网格顶点布局，可以直接在Shader Config当中引用。
- `shader-def-prop-camera`是经典的相机变换矩阵等内容的布局。
- `shader-def-prop-transfrm`是经典的模型变换矩阵等内容的布局。
- `rtask-def-clear`是清除一个Render Target的渲染缓冲区的Render Task。
- `rtask-def-display-texture`方便你查看Render To Texture的结果。
- `rtask-def-display-cubemap`方便你查看Render To Cubemap的结果。

另外有一个用纯Scheme实现的矩阵库。只是由于存在两套矩阵库让人非常不舒服，所以未来会将C++那套矩阵库反射出来供Scheme使用。（TODO）

