(use-modules (shrtool))

`((name . "caustics")
   ,shader-def-attr-mesh
   ,(shader-def-prop-camera)
   (property-group
     (name . "illum")
     (layout
       (col4 . "lightPosition")
       (color . "lightColor")
       (tex2d . "hfMap")))
   (sub-shader
     (type . vertex)
     (source . "
       const float nAir = 1.000028, nWater = 1.333333;

       out vec4 worldPos;

       bool in_bottom(vec2 v) {
           return v.x >= -0.5 && v.x <= 0.5 && v.y >= -0.5 && v.y <= 0.5;
       }

       bool in_wall(vec2 v) {
           return v.x >= -0.5 && v.x <= 0.5 && v.y >= -0.5;
       }

       void main() {
           float cellSize = 1.0 / textureSize(hfMap, 0).x;

           worldPos = position;
           worldPos /= worldPos.w;

           vec2 texCoord = position.xz * (1-3*cellSize) + vec2(0.5, 0.5);
           vec3 lightDir = normalize((worldPos - lightPosition).xyz);

           vec3 hfNormal = vec3(
               textureLod(hfMap, texCoord + vec2( cellSize, 0), 0).r -
               textureLod(hfMap, texCoord + vec2(-cellSize, 0), 0).r,
               2,
               textureLod(hfMap, texCoord + vec2(0,  cellSize), 0).r -
               textureLod(hfMap, texCoord + vec2(0, -cellSize), 0).r);

           hfNormal = normalize(hfNormal);
           vec3 refr = refract(lightDir, hfNormal, nAir / nWater);

           float coefNegY = (-0.5 -worldPos.y) / refr.y;
           // worldPos.x + coef * refr.x = 0.5
           float coefPosX = (0.5  -worldPos.x) / refr.x;
           float coefNegX = (-0.5 -worldPos.x) / refr.x;
           float coefPosZ = (0.5  -worldPos.z) / refr.z;
           float coefNegZ = (-0.5 -worldPos.z) / refr.z;

           vec4 posNegY = worldPos + vec4(refr * coefNegY, 0);
           vec4 posNegX = worldPos + vec4(refr * coefNegX, 0);
           vec4 posPosX = worldPos + vec4(refr * coefPosX, 0);
           vec4 posNegZ = worldPos + vec4(refr * coefNegZ, 0);
           vec4 posPosZ = worldPos + vec4(refr * coefPosZ, 0);

           if(coefNegY > 0 && in_bottom(posNegY.xz)) worldPos = posNegY;
           if(coefPosX > 0 && in_wall(posPosX.zy)) worldPos = posPosX;
           if(coefNegX > 0 && in_wall(posNegX.zy)) worldPos = posNegX;
           if(coefPosZ > 0 && in_wall(posPosZ.xy)) worldPos = posPosZ;
           if(coefNegZ > 0 && in_wall(posNegZ.xy)) worldPos = posNegZ;

           gl_Position = vpMatrix * worldPos;
       }"))
   (sub-shader
     (type . geometry)
     (source . "
       layout(triangles) in;
       layout(triangle_strip, max_vertices = 3) out;

       in vec4 worldPos[];
       out float strength;
       out vec4 fragPosition;

       void main()
       {
           float cellSize = 1.0 / textureSize(hfMap, 0).x;

           float area = length(cross(
                   (worldPos[1] - worldPos[0]).xyz,
                   (worldPos[2] - worldPos[1]).xyz
               ));
           area /= pow(cellSize, 2);

           strength = clamp(1 / area, 0, 2);

           for(int i = 0; i < gl_in.length(); i++) {
               gl_Position = gl_in[i].gl_Position;
               fragPosition = worldPos[i];
               EmitVertex();
           }

          EndPrimitive();
       }"))
   (sub-shader
     (type . fragment)
     (source . "
       layout(location = 0) out vec4 color;

       in float strength;
       in vec4 fragPosition;

       void main()
       {
           color.r = strength;
       }")))

