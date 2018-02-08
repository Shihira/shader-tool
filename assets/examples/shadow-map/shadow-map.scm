(import (shrtool))

`((name . "shadow-map")
  ,shader-def-attr-mesh
  ,(shader-def-prop-transfrm)
  ,(shader-def-prop-camera)
  (sub-shader
    (type . fragment)
    (source . "
      in vec4 v_position;
      out vec4 outColor;

      void main() {
          float depth = v_position.z / v_position.w;
          depth = depth / 2 + 0.5;
          //depth = exp(depth);
          outColor.x = depth;
          outColor.y = depth * depth;
          
          float dx = dFdx(depth);
          float dy = dFdy(depth);
          outColor.y += (dx * dx + dy * dy) / 4;
      }"))
  (sub-shader
    (type . vertex)
    (source . "
      out vec4 v_position;
      void main() {
          gl_Position = vpMatrix * mMatrix * position;
          v_position = gl_Position;
      }")))
