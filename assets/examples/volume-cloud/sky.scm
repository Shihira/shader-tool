(use-modules (shrtool))

`((name . "solid-color")
  ,shader-def-attr-mesh
  (property-group
    (name . "textures")
    (layout
     (tex2d . "densityMap")))
  (sub-shader
    (type . fragment)
    (source . ,(string-append "
      in vec2 texCoord;
      out vec4 outColor;
      
      void main() {
          float density = texture(densityMap, texCoord).r;
          outColor = vec4(1, 1, 1, density);
      }")))
  (sub-shader
    (type . vertex)
    (source . ,(string-append "
      out vec2 texCoord;
      void main() {
          gl_Position = vec4(position.z, position.x, 0, 1);
          texCoord = vec2(position.z, position.x) / 2 + vec2(0.5, 0.5);
      }"))))
