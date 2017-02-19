(import (shrtool))

`((name . "height-fields")
  ,shader-def-attr-mesh
  (property-group
    (name . "hfInfo")
    (layout
      (tex2d . "hfMap")))
  (sub-shader
    (type . fragment)
    (version . "330 core")
    (source . "
      out vec4 outInfo;

      in vec2 texCoord;

      const float v = 3.2;
      const float dt = 0.22;

      void main()
      {
          float cellSize = 1.0 / textureSize(hfMap, 0).x;

          vec2 adj[4];
          adj[0] = vec2(cellSize, 0); adj[1] = vec2(-cellSize, 0);
          adj[2] = vec2(0, cellSize); adj[3] = vec2(0, -cellSize);

          outInfo = texture(hfMap, texCoord);

          vec4 adjInfo[4];
          float f = 0;
          for(int i = 0; i < 4; i++) {
              vec2 texCoord_i = texCoord + adj[i];
              texCoord_i.x = clamp(texCoord_i.x, cellSize, 1 - cellSize);
              texCoord_i.y = clamp(texCoord_i.y, cellSize, 1 - cellSize);
      
              adjInfo[i] = texture(hfMap, texCoord_i);
              f += adjInfo[i].r;
          }

          f -= 4 * outInfo.r;
          f *= v * v;

          outInfo.g += f * dt;
          outInfo.g *= 0.998;
          outInfo.r += outInfo.g * dt;
      }"))
  (sub-shader
    (type . vertex)
    (version . "330 core")
    (source . "
      out vec2 texCoord;
      void main() {
          gl_Position = vec4(position.z, position.x, 0, 1);
          texCoord = vec2(position.z, position.x) / 2 + vec2(0.5, 0.5);
      }")))
