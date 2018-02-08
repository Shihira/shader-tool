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
      (tex2d . "geometryMap")
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
      #define E 2.71828182846
      #define PI 3.14159265359

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
          float fragDepth = texCoord.z;

          /*
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
          */

          //fragDepth = exp(fragDepth);
          vec2 vsm = texture(shadowMap, texCoord.xy).rg;
          float var = max(vsm.y - vsm.x * vsm.x, 0.00002);
          float dist = fragDepth - vsm.x;
          float pmax = var / (var + dist * dist);
          shadow = pmax;//clamp((pmax - 0.3) / (1 - 0.3), 0, 1);
          if(fragDepth < vsm.x) shadow = 1;

          /*
          float esm = 0;
          float k = 50;
          for(int i = -sampleRadius; i <= sampleRadius; i++) {
              for(int j = -sampleRadius; j <= sampleRadius; j++) {
                  esm += texture(shadowMap, texCoord.xy +
                      vec2(i * sampleOffset, j * sampleOffset)).r;
              }
          }
          esm /= (sampleRadius * 2 + 1) * (sampleRadius * 2 + 1);
          shadow = pow(E, k * texture(shadowMap, texCoord.xy).r) * pow(E, -k * fragDepth);
          */

          // AO calculation
          vec4 screenPosition = vpMatrix * fragPosition;
          vec2 geometryMap_uv = (screenPosition / screenPosition.w).xy / 2
              + vec2(0.5, 0.5);
          float occ_factor = 0;
          /*
          float r = 0.001; float a = 0.5;
          for(int i = 0; i <= 50; i++) {
              vec2 off = vec2(r * cos(a), r * sin(a));

              vec3 nml = texture(geometryMap, geometryMap_uv + off).xyz;
              float dpth = texture(geometryMap, geometryMap_uv + off).w;
              occ_factor += max(0, (-dot(nml, fragNormal) + 0.1) *
                  (1 / (1 + abs(dpth - fragPosition.z))));

              r *= 1.1;
              a += 0.77;
          }
          */
          for(float i = -4 * sampleOffset; i <= 4 * sampleOffset; i += sampleOffset)
          for(float j = -4 * sampleOffset; j <= 4 * sampleOffset; j += sampleOffset) {
              vec2 off = vec2(i, j);

              vec3 nml = texture(geometryMap, geometryMap_uv + off).xyz;
              float dpth = texture(geometryMap, geometryMap_uv + off).w;
              occ_factor += max(0, (-dot(nml, fragNormal) + 0.1) *
                  (1 / (1 + abs(dpth - fragPosition.z))));
          }
          occ_factor /= 8;

          float intAmbient = max(0, 1 - occ_factor);
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
