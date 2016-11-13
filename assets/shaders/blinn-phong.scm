(use-modules (shrtool))

(append
  (list attr-mesh prop-transfrm)
  '((name . "blinn-phong")
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
            vec3 lightDir = normalize((fragPosition - lightPosition).xyz);
            vec3 viewDir = normalize((fragPosition - cameraPosition).xyz);

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
            fragNormal = (transpose(inverse(mMatrix)) * vec4(normal, 0)).xyz;

            gl_Position = mvpMatrix * position;
            fragPosition = mMatrix * position;
            fragPosition /= fragPosition.w;
        }"))))
