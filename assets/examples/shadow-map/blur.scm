(import (shrtool))

(shader-def-quad
  `(property-group
     (name . "blurParam")
     (layout
       (int . "size")
       (int . "direction")
       (mat4 . "kernel")))
  "
  void main() {
      ivec2 texSize = textureSize(texMap, 0);
      float x_texel = 1.0 / texSize.x;
      float y_texel = 1.0 / texSize.y;

      vec2 off = vec2(x_texel, y_texel);
      off[direction] = 0;

      vec2 target_color = textureLod(texMap, texCoord, 0).rg;
      vec2 color = vec2(0, 0);

      for(int i = 0; i < size; i++) {
          int m = i / 4;
          int n = i - m * 4;

          vec2 c1 = textureLod(texMap, texCoord + i * off, 0).rg;
          vec2 c2 = textureLod(texMap, texCoord - i * off, 0).rg;
          color += kernel[n][m] * (c1 + c2);
      }

      outColor.rg = color;
  }
  ")
