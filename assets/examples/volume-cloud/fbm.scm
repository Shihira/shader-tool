(use-modules (shrtool))

`((name . "perlin")
  ,shader-def-attr-mesh
  (property-group
    (name . "prop")
    (layout
      (float . "movement")
      (float . "thickness")
      (float . "rotation")
      (col3 . "sunlight")))
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

      float hash(vec3 co) {
          return fract(sin(dot(co.xyz ,vec3(12.9898,78.233, 173.89))) * 43758.5453);
      }

      float interpolate_1(vec2 surrd, float dist) {
          float ft = dist * 3.1415927;
          float f = (1 - cos(ft)) * 0.5;
          return surrd.x * (1-f) + surrd.y * f;
      }

      float noise(in vec3 x){
          vec3 tl = floor(x);
          vec3 coef = x - tl;
          float n1 = hash(tl + vec3(0, 0, 0)),
                n2 = hash(tl + vec3(1, 0, 0)),
                n3 = hash(tl + vec3(0, 1, 0)),
                n4 = hash(tl + vec3(1, 1, 0)),
                n5 = hash(tl + vec3(0, 0, 1)),
                n6 = hash(tl + vec3(1, 0, 1)),
                n7 = hash(tl + vec3(0, 1, 1)),
                n8 = hash(tl + vec3(1, 1, 1));

          float i1 = interpolate_1(vec2(n1, n2), coef.x),
                i2 = interpolate_1(vec2(n3, n4), coef.x),
                i3 = interpolate_1(vec2(i1, i2), coef.y),

                i4 = interpolate_1(vec2(n5, n6), coef.x),
                i5 = interpolate_1(vec2(n7, n8), coef.x),
                i6 = interpolate_1(vec2(i4, i5), coef.y),

                i7 = interpolate_1(vec2(i3, i6), coef.z);

          i7 /= 2;
          i7 += 0.5;

          return i7;
      }

      float worley( in vec3 x )
      {
          vec3 p = floor( x );
          vec3 f = fract( x );

          float id = 0.0;
          vec2 res = vec2( 100.0 );
          for( int k=-1; k<=1; k++ )
          for( int j=-1; j<=1; j++ )
          for( int i=-1; i<=1; i++ )
          {
              vec3 b = vec3( float(i), float(j), float(k) );
              vec3 r = vec3( b ) - f + hash( p + b );
              float d = dot( r, r );

              if( d < res.x )
              {
                  id = dot( p+b, vec3(1.0,57.0,113.0 ) );
                  res = vec2( d, res.x );			
              }
              else if( d < res.y )
              {
                  res.y = d;
              }
          }

          //return vec3( sqrt( res ), abs(id) );
          return res.x;
      }

      float fbm_perlin(vec3 co, int N) {
          float v = 0;
          for(int i = 1; i <= N; i++)
              v += pow(0.5, i) * noise(co * pow(2, i));
          return v;
      }

      float fbm_worley(vec3 co, int N) {
          float v = 0;
          for(int i = 1; i <= N; i++)
              v += pow(0.5, i) * worley(co * pow(2, i));
          return v;
      }

      float gradient(float h) {
          /*
          float l1 = 0.1,
                l2 = 1.5,
                l3 = 2.6,
                l4 = 9.0;

          if(h < l1) return 0.0;
          if(h > l4) return 0.0;

          if(h < l2 && h >= l1)
            return (h - l1) / (l2 - l1);
          if(h > l3 && h <= l4)
            return (l4 - h) / (l4 - l3);
          return 1.0;
          */
          if(h < 0) return 0.0;
          h /= 5;
          return 1.1 * exp(-0.2 * h) * (1 - exp(-30 * h));
      }

      float cloud(vec3 co) {
          float p = fbm_perlin(vec3(co.xz * 0.1, co.y * 0.1), 8) * gradient(co.y);
          float w = fbm_worley(vec3(co.xzy * 0.5), 2);
          p = (p * 4 - 3.2 + thickness);
          w = 1 - w;
          w = pow(w, 20 * pow(mix(0, 1, (co.y - 1) / 10), 1/1.3));

          float c = pow(w * p, 1) * 2;

          return clamp(c, 0, 1);
      }


      float HenyeyGreenstein(vec3 inLightVector, vec3 inViewVector, float inG) {
          float cos_angle = dot(normalize(inLightVector), normalize(inViewVector));
          return ((1.0 - inG*inG) / pow((1.0 + inG*inG -
              2.0 * inG * cos_angle), 3.0 / 2.0)) / 4.0 * 3.1415;
      }

      vec4 raytrace() {
          int recur = 100;

          mat4 rot = mat4(
            cos(rotation), 0, -sin(rotation), 0,
            0, 1, 0, 0,
            sin(rotation), 0, cos(rotation), 0,
            0, 0, 0, 1
          ) * mat4(
            1, 0, 0, 0,
            0, cos(0.53), -sin(0.53), 0,
            0, sin(0.53), cos(0.53), 0,
            0, 0, 0, 1
          );

          vec3 start = vec3(vec2(0, -2), -1 + movement * 50);
          vec3 end = vec3(texCoord.xy - vec2(0.5, 0.5) * vec2(1, 1) + vec2(0, -2), movement * 50);

          vec3 d = normalize(end - start);
          d = (rot * vec4(d, 0)).xyz;

          if(d.y < 0.0) return vec4(0, 0, 0, 0);

          vec3 s = start + d * (-start.y / d.y);
          vec3 e = start + d * ((10 - start.y) / d.y);
          float len = length(e - s) / recur;
          d *= len;
          vec3 sl = normalize(sunlight);
 
          int times = 1;
          float phase = HenyeyGreenstein(sunlight, end - start, 0.5);

          float transmit = 1;
          float scatter = 0;
          float ds = 0, dt = 0, es = 1, et = 10;

          s += d * hash(s * vec3(1.2, 3.4, 5.6)) * 4 - d * 2;

          for(int i = 0; i < recur; i++) {
              float den = cloud(s);
              s += d;

              if(times > recur / 9) break;

              if(den < 0.00001) {
                  times += 1;
                  s += clamp(times, 1, 5) * d;
                  continue;
              }

              if(times > 1 && den > 0.00001) {
                  s = s - clamp(times, 1, 5) * d + d;
                  times = 1;
                  den = cloud(s);
              }

              if(den > 0.0001) {
                  float light_sl = 0.5;
                  vec3 light_s = s + light_sl * sl;
                  float light_den = den * light_sl;
                  for(int j = 0; j < 6; j++) {
                       light_den += cloud(light_s) * light_sl;
                       light_sl *= 1.5;
                       light_s += light_sl * sl;
                  }

                  light_den *= 1. / 6 * 60 + 0.5;

                  ds = phase * (1 - exp(-light_den * 2)) * exp(-light_den) * 2;
                  dt = exp(-den * len * et);
                  ds = (ds - ds * exp(-den * len * et)) / (den);

                  transmit *= dt;
                  scatter += transmit * ds;

                  if(transmit < 0.01) break;
              }
          }

          /*
          v *= 1 - pow(length(s - start) / 200, 2);
          v = clamp(v, 0, 1);
          */

          float v = (1 - transmit) * 2;

          vec4 clr = vec4(74./255, 116./255, 138./255, v) * (1 - scatter) + vec4(1, 1, 1, v) * scatter;

          if(v < 1) {
              float ss = clamp(dot(normalize(d), normalize(sunlight)), 0, 1);
              clr += vec4(1, 1, 1, 1) * (pow(ss, 10) / 2 + pow(ss, 300));
          }

          return clr;
      }

      void main() {
          //float r = hash(texCoord);
          //float r = fbm_perlin(vec3(texCoord / L / 3, movement), 10);
          //float r = (1 - fbm(vec3(texCoord / L, 1), 5)) * (fbm(texCoord / L / 3, 10) * 4 - 2.5);
          //r = pow(r, 2.2);
          //float r = cloud(vec3(texCoord.x, movement, texCoord.y));
          vec4 c = clamp(raytrace(), 0, 1);

          //color.r = clamp(r, 0, 1);
          color = vec4(c.rgb * c.a +
              (vec3(91, 148, 168) / vec3(255, 255, 255)) * (1 - c.a), 1);
      }")))


