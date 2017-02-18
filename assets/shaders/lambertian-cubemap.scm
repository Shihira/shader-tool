(use-modules (shrtool))
    
`((name . "lambertian-texture")
  ,shader-def-attr-mesh
  ,(shader-def-prop-transfrm)
  ,(shader-def-prop-camera)
  (property-group
    (name . "illum")
    (layout
      (col4 . "lightPosition")
      (color . "lightColor")))
  (property-group
    (name . "material")
    (layout
      (float . "ambientStrength")
      (float . "diffuseStrength")
      (texcube . "texMap")))
  (sub-shader
    (type . fragment)
    (version . "330 core")
    (source . "
      in vec4 fragPosition;
      in vec3 fragNormal;
      in vec2 fragUV;

      out vec4 outColor;

      void main() {
          vec3 lightDir = normalize((fragPosition - lightPosition).xyz);
          float strength =
              max(0, diffuseStrength * dot(-lightDir, fragNormal)) +
              ambientStrength;
          vec4 texColor = texture(texMap, fragPosition.xyz);
          outColor = strength * texColor;
      }"))
  (sub-shader
    (type . vertex)
    (version . "330 core")
    (source . "
      out vec4 fragPosition;
      out vec3 fragNormal;
      out vec2 fragUV;

      void main() {
          fragNormal = (transpose(mMatrix_inv) * vec4(normal, 0)).xyz;

          gl_Position = vpMatrix * mMatrix * position;
          fragPosition = mMatrix * position;
          fragPosition /= fragPosition.w;
          fragUV = (uv / uv.z).xy;
      }")))
