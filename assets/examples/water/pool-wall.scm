(use-modules (shrtool))

(lambda (cubemap?)
  (let ((prop-camera
          (if cubemap?
            '(property-group
              (name . "camera")
              (layout
                (mat4 . "vpMats[6]")))
            (shader-def-prop-camera)))
        (procVertex
          (if cubemap?
            "return vpMats[renderPass_] * mpos;"
            "return vpMatrix * mpos;"))
        (passUniform
          (if cubemap?
            "uniform int renderPass_;" "")))
    `((name . "pool-wall")
      ,shader-def-attr-mesh
      ,prop-camera
      ,(shader-def-prop-transfrm)
      ,(shader-def-prop-camera #:name "causticsCamera" #:prefix "caus_")
      (property-group
        (name . "material")
        (layout
          (color . "waterColor")
          (tex2d . "causticsMap")))
      (property-group
        (name . "illum")
        (layout
          (col4 . "lightPosition")
          (color . "lightColor")))
      (sub-shader
        (type . vertex)
        (source . ,(string-append passUniform "
          out vec4 worldPos;
          out vec3 fragNormal;

          vec4 procVertex(vec4 mpos) {" procVertex "}

          void main()
          {
              worldPos = mMatrix * position;
              worldPos /= worldPos.w;

              fragNormal = (transpose(mMatrix_inv) * vec4(normal, 0)).xyz;
              fragNormal = normalize(fragNormal);
              if(dot(worldPos.xyz, fragNormal) > 0)
                  fragNormal = -fragNormal;

              gl_Position = procVertex(worldPos);
          }")))
      (sub-shader
        (type . fragment)
        (source . "
          in vec4 worldPos;
          in vec3 fragNormal;

          out vec4 color;

          const float gaussKernel[25] = float[25](
              0.003765, 0.015019, 0.023792, 0.015019, 0.003765,
              0.015019, 0.059912, 0.094907, 0.059912, 0.015019,
              0.023792, 0.094907, 0.150342, 0.094907, 0.023792,
              0.015019, 0.059912, 0.094907, 0.059912, 0.015019,
              0.003765, 0.015019, 0.023792, 0.015019, 0.003765);

          void main()
          {
              if(worldPos.y > 0)
                  discard;

              vec4 causPos = caus_vpMatrix * worldPos;
              causPos /= causPos.w;
              vec2 texCoord = causPos.xy * 0.5 + vec2(0.5, 0.5);

              float strength = 0;

              float causStrength = 0;
              for(int i = -2; i <= 2; i++)
                  for(int j = -2; j <= 2; j++)
                      causStrength += gaussKernel[(i+2)*5+j+2] *
                          texture(causticsMap, texCoord +
                              vec2(i / 512., j / 512.)).r;
              causStrength = clamp(causStrength, 0.2, 1.5);

              float textureStrength = 1;
              ivec3 pos = ivec3((worldPos.x + 0.5) * 320,
                      (worldPos.y + 0.5) * 320, (worldPos.z + 0.5) * 320);

              if(1 - abs(dot(fragNormal, vec3(0, 1, 0))) < 0.001) {
                  if(pos.x % 10 == 0 || pos.z % 10 == 0)
                      textureStrength *= 0.8;
              } else if(1 - abs(dot(fragNormal, vec3(1, 0, 0))) < 0.001) {
                  if(pos.y % 10 == 0 || pos.z % 10 == 0)
                      textureStrength *= 0.8;
              } else if(1 - abs(dot(fragNormal, vec3(0, 0, 1))) < 0.001) {
                  if(pos.x % 10 == 0 || pos.y % 10 == 0)
                      textureStrength *= 0.8;
              }

              strength += causStrength * 0.6;
              strength += 0.2 * dot(-fragNormal, normalize(worldPos.xyz -
                              lightPosition.xyz / lightPosition.w));
              strength += 0.2;
              strength *= textureStrength;

              color = strength * waterColor;
          }")))))
