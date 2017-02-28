(use-modules (shrtool))

`((name . "blinn-phong-with-shadow")
  ,shader-def-attr-mesh
  ,(shader-def-prop-transfrm)
  ,(shader-def-prop-camera)
  ,(shader-def-prop-camera #:name "lightCamera" #:prefix "light_")
  (property-group
    (name . "illum")
    (layout
      (tex2d . "shadowMap")
      (float . "sampleOffset")
      (int . "sampleRadius")
      (color . "lightColor")))
  (property-group
    (name . "material")
    (layout
      (color . "ambientColor")
      (color . "diffuseColor")
      (color . "specularColor")))
  (sub-shader
    (type . fragment)
    (source . "
      in vec4 fragPosition;
      in vec3 fragNormal;

      layout (location = 0) out vec4 outColor;

      #define BIAS 0.0001

      void main() {
          vec4 lightPosition = vec4(light_vMatrix_inv[3].xyz /
                  light_vMatrix_inv[3].w, 1);
          vec4 cameraPosition = vec4(vMatrix_inv[3].xyz /
                  vMatrix_inv[3].w, 1);
          vec3 lightDir = normalize((fragPosition - lightPosition).xyz);
          vec3 viewDir = normalize((fragPosition - cameraPosition).xyz);

          vec4 light_FragCoord = light_vpMatrix * fragPosition;
          light_FragCoord /= light_FragCoord.w;
          vec3 texCoord = light_FragCoord.xyz / 2 + vec3(0.5, 0.5, 0.5);

          float shadow = 0;
          float fragDepth = texCoord.z - BIAS;

          for(int i = -sampleRadius; i <= sampleRadius; i++) {
              for(int j = -sampleRadius; j <= sampleRadius; j++) {
                  float shelterDepth = texture(shadowMap, texCoord.xy +
                      vec2(i * sampleOffset, j * sampleOffset)).r;
                  shadow += step(fragDepth, shelterDepth);
              }
          }

          // not proven. practical value.
          shadow /= pow(sampleRadius * 2 + 1, 2) / 2;
          shadow = min(shadow + 0.1, 1);

          float intAmbient = 1;
          float intDiffuse = dot(-fragNormal, lightDir);
          float intSpecular = dot(-fragNormal, normalize(lightDir + viewDir));

          intDiffuse = max(intDiffuse, 0) * shadow;
          intSpecular = pow(intSpecular, 3);
          intSpecular = max(intSpecular, 0) * shadow;

          outColor =
              ambientColor * intAmbient +
              diffuseColor * intDiffuse +
              specularColor * intSpecular;
      }"))
  (sub-shader
    (type . vertex)
    (source . "
      out vec4 fragPosition;
      out vec3 fragNormal;

      void main() {
          fragNormal = (transpose(mMatrix_inv) * vec4(normal, 0)).xyz;
          fragNormal = normalize(fragNormal);

          gl_Position = vpMatrix * mMatrix * position;
          fragPosition = mMatrix * position;
          fragPosition /= fragPosition.w;
      }")))
