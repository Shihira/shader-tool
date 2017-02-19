(use-modules (shrtool))
    
`((name . "water-surface")
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
      (texcube . "envMap")
      (tex2d . "hfMap")))
  (sub-shader
    (type . fragment)
    (version . "330 core")
    (source . "
      in vec4 fragPosition;
      in vec3 fragNormal;

      out vec4 outColor;

      void main() {
          vec4 cameraPosition = vec4(vMatrix_inv[3].xyz /
                  vMatrix_inv[3].w, 1);

          vec3 lightDir = normalize((fragPosition - lightPosition).xyz);
          vec3 viewDir = normalize((fragPosition - cameraPosition).xyz);
          vec3 envDir = reflect(viewDir, fragNormal);

          float strength = pow(dot(-fragNormal, normalize(lightDir + viewDir)), 15);
          strength = min(strength, 1);

          outColor = strength * 0.5 * lightColor + texture(envMap, envDir);
      }"))
  (sub-shader
    (type . vertex)
    (version . "330 core")
    (source . "
      out vec4 fragPosition;
      out vec3 fragNormal;

      void main() {
          float cellSize = 1.0 / textureSize(hfMap, 0).x;

          vec2 texCoord = position.xz * (1-3*cellSize) + vec2(0.5, 0.5);

          fragPosition = mMatrix * position;
          fragPosition /= fragPosition.w;
          fragPosition.y += textureLod(hfMap, texCoord, 0).r * cellSize;

          gl_Position = vpMatrix * fragPosition;

          fragNormal = vec3(
              textureLod(hfMap, texCoord + vec2( cellSize, 0), 0).r -
              textureLod(hfMap, texCoord + vec2(-cellSize, 0), 0).r,
              2,
              textureLod(hfMap, texCoord + vec2(0,  cellSize), 0).r -
              textureLod(hfMap, texCoord + vec2(0, -cellSize), 0).r);
          fragNormal = normalize(fragNormal);
      }")))
