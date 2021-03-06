(use-modules (shrtool))

`((name . "blinn-phong")
  ,shader-def-attr-mesh
  ,(shader-def-prop-transfrm)
  ,(shader-def-prop-camera)
  (property-group
    (name . "illum")
    (layout
      (col4 . "lightPosition")
      (color . "lightColor")
      (col4 . "cameraPosition")))
  (property-group
    (name . "material")
    (layout
      (color . "ambientColor")
      (color . "diffuseColor")
      (color . "specularColor")))
  (sub-shader
    (type . fragment)
    (version . "330 core")
    (source . "
      in vec4 fragPosition;
      in vec3 fragNormal;

      layout (location = 0) out vec4 outColor;

      void main() {
          vec4 camPos = vec4(vMatrix_inv[3].xyz, vMatrix_inv[3].w);
          vec3 viewDir = normalize((fragPosition - camPos).xyz);
          vec3 lightDir = normalize((fragPosition - lightPosition).xyz);

          float intAmbient = 1;
          float intDiffuse = dot(-fragNormal, lightDir);
          float intSpecular = dot(-fragNormal, normalize(lightDir + viewDir));

          intDiffuse = max(intDiffuse, 0);
          intSpecular = pow(intSpecular, 3);
          intSpecular = max(intSpecular, 0);

          outColor =
              ambientColor * intAmbient +
              diffuseColor * intDiffuse +
              specularColor * intSpecular;
      }"))
  (sub-shader
    (type . vertex)
    (version . "330 core")
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
