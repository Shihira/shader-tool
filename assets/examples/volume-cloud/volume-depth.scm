(use-modules (shrtool))

`((name . "volume-depth")
  ,shader-def-attr-mesh
  ,(shader-def-prop-transfrm)
  ,(shader-def-prop-camera)
  (property-group
    (name . "parameter")
    (layout
      (tex2d . "hfMap")))
  (sub-shader
    (type . fragment)
    (source . "
      in float viewDepth;
      in vec4 fragPosition;

      out vec4 outColor;

      void main() {
          vec2 texCoord = (fragPosition.xz + vec2(0.5, 0.5)) * 50;
          texCoord.x -= floor(texCoord.x);
          texCoord.y -= floor(texCoord.y);

          outColor.r = viewDepth + (texture(hfMap, texCoord).r - 0.5) / 8;
      }"))
  (sub-shader
    (type . geometry)
    (source . "
      layout(triangles) in;
      layout(triangle_strip, max_vertices = 3) out;

      in geoData {
          vec4 viewPosition;
          vec4 fragPosition;
      } inData[];

      out float viewDepth;
      out vec4 fragPosition;

      void main() {
          vec3 crs = cross(
              (gl_in[1].gl_Position / gl_in[1].gl_Position.w -
               gl_in[0].gl_Position / gl_in[0].gl_Position.w).xyz,
              (gl_in[2].gl_Position / gl_in[2].gl_Position.w -
               gl_in[1].gl_Position / gl_in[1].gl_Position.w).xyz);

          float coef = crs.z < 0 ? 1 : -1;
          coef *= 5;

          for(int i = 0; i < 3; i++) {
              viewDepth = coef * inData[i].viewPosition.z;
              fragPosition = inData[i].fragPosition;

              gl_Position = gl_in[i].gl_Position;
              EmitVertex();
          }

          EndPrimitive();
      }"))
  (sub-shader
    (type . vertex)
    (source . "
      out geoData {
          vec4 viewPosition;
          vec4 fragPosition;
      } outData;

      void main() {
          outData.fragPosition = mMatrix * position;
          vec4 fragNormal = transpose(mMatrix_inv) * vec4(normal, 0);
          if(uv.x != 0 && uv.x != 1 && uv.y != 0 && uv.y != 1) {
              outData.fragPosition += fragNormal *
                  textureLod(hfMap, uv.xy, 0).r / 7;
          }

          outData.viewPosition = vMatrix * outData.fragPosition;
          outData.viewPosition /= outData.viewPosition.w;

          gl_Position = vpMatrix * outData.fragPosition;
      }")))
