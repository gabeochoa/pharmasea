#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Lighting uniforms
uniform vec3 viewPos;

// "Sun" (can be point or directional)
// lightType: 0 = directional, 1 = point
uniform int lightType;
uniform vec3 lightDir;   // used if directional (direction the light points)
uniform vec3 lightPos;   // used if point
uniform vec3 lightColor; // linear RGB

uniform vec3 ambientColor; // linear RGB

// Shading model controls
uniform float shininess;      // e.g. 16..128
uniform int useHalfLambert;   // 0/1 (raylib sets bools as ints)

// Roof masking (disable direct sun indoors)
uniform int roofRectCount;
// vec4(minX, minZ, maxX, maxZ)
uniform vec4 roofRects[8];

out vec4 finalColor;

bool isInsideAnyRoof(vec3 worldPos)
{
    for (int i = 0; i < roofRectCount; i++)
    {
        vec4 r = roofRects[i];
        if (worldPos.x >= r.x && worldPos.x <= r.z &&
            worldPos.z >= r.y && worldPos.z <= r.w)
        {
            return true;
        }
    }
    return false;
}

void main()
{
    vec4 albedo = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    vec3 N = normalize(fragNormal);

    vec3 L;
    if (lightType == 0)
    {
        // Directional light: lightDir is the direction the light points *toward*.
        // Light vector is opposite.
        L = normalize(-lightDir);
    }
    else
    {
        L = normalize(lightPos - fragPosition);
    }

    float ndl = dot(N, L);
    float diff = (useHalfLambert != 0) ? clamp(ndl * 0.5 + 0.5, 0.0, 1.0) : max(ndl, 0.0);

    vec3 V = normalize(viewPos - fragPosition);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    // Disable direct sun indoors (but keep ambient).
    float sunMask = isInsideAnyRoof(fragPosition) ? 0.0 : 1.0;

    vec3 lit = ambientColor * albedo.rgb +
               (lightColor * diff * sunMask) * albedo.rgb +
               (lightColor * spec * sunMask);

    finalColor = vec4(lit, albedo.a);
}

