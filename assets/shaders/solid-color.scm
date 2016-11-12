(append
  (list attr-mesh prop-transfrm)
  '((name . "solid-color")
    (property-group
      (name . "material")
      (layout
       (col4 . "color")))
    (sub-shader
      (type . fragment)
      (version . "330 core")
      (source . "
        out vec4 outColor;
        void main() {
            outColor = color;
        }"))
    (sub-shader
      (type . vertex)
      (version . "330 core")
      (source . "
        void main() {
            gl_Position = mvpMatrix * position;
        }"))))
