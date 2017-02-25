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
      (texcube . "skyboxMap")
      (tex2d . "hfMap")))
  (sub-shader
    (type . fragment)
    (version . "330 core")
    (source . "
      in vec4 fragPosition;
      in vec3 fragNormal;

      out vec4 outColor;

      // not real. just for better appearance.
      const float nAir = 1.000028, nWater = 1.333333;

      bool in_bottom(vec2 v) {
          return v.x >= -0.5 && v.x <= 0.5 && v.y >= -0.5 && v.y <= 0.5;
      }

      bool in_wall(vec2 v) {
          return v.x >= -0.5 && v.x <= 0.5 && v.y >= -0.5;
      }

      vec4 intersect_wall(vec4 worldPos, vec3 refr) {
           float coefNegY = (-0.5 -worldPos.y) / refr.y;
           // worldPos.x + coef * refr.x = 0.5
           float coefPosX = (0.5  -worldPos.x) / refr.x;
           float coefNegX = (-0.5 -worldPos.x) / refr.x;
           float coefPosZ = (0.5  -worldPos.z) / refr.z;
           float coefNegZ = (-0.5 -worldPos.z) / refr.z;

           vec4 posNegY = worldPos + vec4(refr * coefNegY, 0);
           vec4 posNegX = worldPos + vec4(refr * coefNegX, 0);
           vec4 posPosX = worldPos + vec4(refr * coefPosX, 0);
           vec4 posNegZ = worldPos + vec4(refr * coefNegZ, 0);
           vec4 posPosZ = worldPos + vec4(refr * coefPosZ, 0);

           if(coefNegY > 0 && in_bottom(posNegY.xz)) worldPos = posNegY;
           if(coefPosX > 0 && in_wall(posPosX.zy)) worldPos = posPosX;
           if(coefNegX > 0 && in_wall(posNegX.zy)) worldPos = posNegX;
           if(coefPosZ > 0 && in_wall(posPosZ.xy)) worldPos = posPosZ;
           if(coefNegZ > 0 && in_wall(posNegZ.xy)) worldPos = posNegZ;

           return worldPos / worldPos.w;
      }

      void main() {
          vec4 cameraPosition = vec4(vMatrix_inv[3].xyz /
                  vMatrix_inv[3].w, 1);

          vec3 lightDir = normalize((fragPosition - lightPosition).xyz);
          vec3 viewDir = normalize((fragPosition - cameraPosition).xyz);
          vec3 skyDir = reflect(viewDir, fragNormal);
          vec3 refrDir = refract(viewDir, fragNormal, nAir / nWater);
          vec4 intsct = intersect_wall(fragPosition, refrDir);

          float strength = pow(dot(-fragNormal,
                      normalize(lightDir + viewDir)), 2);
          strength = min(strength, 1);

          outColor = strength * 0.2 * lightColor +
              0.6 * texture(skyboxMap, skyDir) +
              0.4 * texture(envMap, intsct.xyz);
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
