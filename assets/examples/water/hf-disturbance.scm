(import (shrtool))

`((name . "hf-disturbance")
  ,shader-def-attr-mesh
  (property-group
    (name . "actions")
    (layout
      (int . "action")
      (col2 . "centerPoint")
      (float . "rippleRadius")
      (float . "rippleHeight")
      (tex2d . "hfMap")))
  (sub-shader
    (type . fragment)
    (source . "
      out vec4 outInfo;

      const float M_PI = 3.141592653589;
      const int CLEAR_HEIGHT_FIELDS = 1;
      const int GENERATE_RIPPLE = 2;

      in vec2 texCoord;

      void clear_height_fields() {
          outInfo = vec4(0, 0, 0, 0);
      }

      void generate_ripple() {
          float cellSize = 1.0 / textureSize(hfMap, 0).x;

          float r = length(texCoord - centerPoint * cellSize);
          if(r > rippleRadius * cellSize) return;

          r /= rippleRadius * cellSize;
          r *= M_PI;

          float drop = cos(r);
          drop *= 0.5;
          drop += 0.5;

          outInfo.r = drop * rippleHeight;
      }

      void main()
      {
          outInfo = texture(hfMap, texCoord);

          if((action & CLEAR_HEIGHT_FIELDS) != 0)
              clear_height_fields();
          if((action & GENERATE_RIPPLE) != 0)
              generate_ripple();
      }"))
  (sub-shader
    (type . vertex)
    (source . "
      out vec2 texCoord;
      void main() {
          gl_Position = vec4(position.z, position.x, 0, 1);
          texCoord = vec2(position.z, position.x) / 2 + vec2(0.5, 0.5);
      }")))
