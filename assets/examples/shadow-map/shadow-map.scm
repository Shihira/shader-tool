(import (shrtool))

`((name . "shadow-map")
  ,shader-def-attr-mesh
  ,(shader-def-prop-transfrm)
  ,(shader-def-prop-camera)
  (sub-shader
    (type . fragment)
    (version . "330 core")
    (source . "void main() { }"))
  (sub-shader
    (type . vertex)
    (version . "330 core")
    (source . "
      void main() {
          gl_Position = vpMatrix * mMatrix * position;
      }")))
