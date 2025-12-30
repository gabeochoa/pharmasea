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

// Additional point lights (e.g. indoor fixtures)
uniform int pointLightCount;
// vec4(x,y,z,radius)
uniform vec4 pointLightsPosRadius[8];
uniform vec3 pointLightsColor[8];

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

bool isInsideRectXZ(vec3 worldPos, vec4 r)
{
    return (worldPos.x >= r.x && worldPos.x <= r.z &&
            worldPos.z >= r.y && worldPos.z <= r.w);
}

vec3 apply_point_light(vec3 albedoRgb, vec3 N, vec3 V, vec3 pos, vec4 plPosRad, vec3 plColor)
{
    vec3 lp = plPosRad.xyz;
    float radius = max(plPosRad.w, 0.0001);
    vec3 L = lp - pos;
    float dist = length(L);
    L = L / max(dist, 0.0001);

    // Simple smooth attenuation: 1 at center, 0 at radius
    float att = clamp(1.0 - (dist / radius), 0.0, 1.0);
    att = att * att;

    float ndl = dot(N, L);
    float diff = (useHalfLambert != 0) ? clamp(ndl * 0.5 + 0.5, 0.0, 1.0) : max(ndl, 0.0);

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    return (plColor * diff) * albedoRgb * att + (plColor * spec) * att;
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

    vec3 lit = ambientColor * albedo.rgb;

    // Sun contribution (outdoors only)
    lit += (lightColor * diff * sunMask) * albedo.rgb;
    lit += (lightColor * spec * sunMask);

    // Indoor point lights: for now, apply only when indoors (inside any roof rect).
    // This matches the "roofs exist" rule and avoids lighting leaking outdoors.
    float indoorMask = isInsideAnyRoof(fragPosition) ? 1.0 : 0.0;
    if (indoorMask > 0.5)
    {
        for (int i = 0; i < pointLightCount; i++)
        {
            lit += apply_point_light(albedo.rgb, N, V, fragPosition, pointLightsPosRadius[i], pointLightsColor[i]);
        }
    }

    finalColor = vec4(lit, albedo.a);
}

