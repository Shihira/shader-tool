(use-modules (shrtool))

`((name . "perlin")
  ,shader-def-attr-mesh
  (sub-shader
    (type . vertex)
    (source . "
      out vec2 texCoord;
      void main() {
          gl_Position = vec4(position.z, position.x, 0, 1);
          texCoord = vec2(position.z, position.x) / 2 + vec2(0.5, 0.5);
      }"))
  (sub-shader
    (type . fragment)
    (source . "
      in vec2 texCoord;
      out vec4 color;

      float hash(vec2 co) {
          return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
      }

      float smooth_hash(vec2 co, float step) {
          float corner = hash(co + vec2( step, step)) +
                         hash(co + vec2(-step, step)) +
                         hash(co + vec2( step,-step)) +
                         hash(co + vec2(-step,-step));
          float edge = hash(co + vec2( 0, step)) +
                       hash(co + vec2(-step, 0)) +
                       hash(co + vec2( 0,-step)) +
                       hash(co + vec2( step,0));
          return corner / 16 + edge / 8 + hash(co) / 4;
      }

      float interpolate1(vec2 surrd, float dist) {
          float ft = dist * 3.1415927;
          float f = (1 - cos(ft)) * 0.5;
          return surrd.x * (1-f) + surrd.y * f;
      }

      float interpolate2(vec4 surrd, vec2 dist) {
          float r1 = interpolate1(surrd.rg, dist.r);
          float r2 = interpolate1(surrd.ba, dist.r);

          float r = interpolate1(vec2(r1, r2), dist.g);
          return r;
      }

      float wave(vec2 coord, float consistency, float amplitude) {
          coord = coord * consistency;
          float scale = 1 / consistency;

          vec2 int_coord = vec2(floor(coord.x), floor(coord.y));
          vec4 s = vec4(smooth_hash((int_coord + vec2(0, 0)) * scale, scale),
                        smooth_hash((int_coord + vec2(1, 0)) * scale, scale),
                        smooth_hash((int_coord + vec2(0, 1)) * scale, scale),
                        smooth_hash((int_coord + vec2(1, 1)) * scale, scale));
          float r = interpolate2(s, coord - int_coord);
          return r * amplitude;
      }

      float perlin_noise(vec2 coord) {
          float r = 0;
          r += wave(coord, 8, 3);
          r += wave(coord, 16, 5);
          r += wave(coord, 32, 1);
          r += wave(coord, 64, 1);
          r += wave(coord, 128, 0.5);
          r += wave(coord, 256, 2.5);
          r += wave(coord, 512, 2);
          r /= 15;

          return r;
      }

      void main() {
          float r = perlin_noise(texCoord) * 2 - 0.6;
          color.r = clamp(r, 0, 1);
      }")))
