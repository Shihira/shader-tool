(import (shrtool))

`((name . "shadow-map")
  ,shader-def-attr-mesh
  ,(shader-def-prop-transfrm)
  ,(shader-def-prop-camera)
  (sub-shader
    (type . fragment)
    (source . "
      in vec3 fragNormal;
      in float fragDepth;

      out vec4 geomap; /* some embryos of gbuffer? */

      void main() {
        geomap.xyz = fragNormal;
        geomap.w = fragDepth;
      }"))
  (sub-shader
    (type . vertex)
    (source . "
      out vec3 fragNormal;
      out float fragDepth;

      void main() {
          gl_Position = vpMatrix * mMatrix * position;

          fragNormal = (inverse(transpose(mMatrix)) * vec4(normal, 0)).xyz;
          vec4 fragPosition = mMatrix * position;
          fragDepth = fragPosition.z / fragPosition.w;
      }")))
