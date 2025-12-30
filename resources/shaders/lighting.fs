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
uniform int lightsPerBuilding; // e.g. 8
// Rects used to decide which building we're inside for indoor lighting
uniform int lightRectCount;
// vec4(minX, minZ, maxX, maxZ)
uniform vec4 lightRects[8];
// Packed lights: grouped per building, contiguous.
// vec4(x,y,z,radius)
uniform vec4 pointLightsPosRadius[64];
uniform vec3 pointLightsColor[64];

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

int getLightRectIndex(vec3 worldPos)
{
    for (int i = 0; i < lightRectCount; i++)
    {
        if (isInsideRectXZ(worldPos, lightRects[i])) return i;
    }
    return -1;
}

vec3 apply_point_light(vec3 albedoRgb, vec3 N, vec3 pos, vec4 plPosRad, vec3 plColor)
{
    vec3 lp = plPosRad.xyz;
    float radius = max(plPosRad.w, 0.0001);
    vec3 L = lp - pos;
    float dist = length(L);
    L = L / max(dist, 0.0001);

    // Simple smooth attenuation: 1 at center, 0 at radius
    float t = clamp(1.0 - (dist / radius), 0.0, 1.0);
    // Smoothstep-ish rolloff to avoid "pointy" hotspots
    float att = t * t * (3.0 - 2.0 * t);

    float ndl = dot(N, L);
    float diff = (useHalfLambert != 0) ? clamp(ndl * 0.5 + 0.5, 0.0, 1.0) : max(ndl, 0.0);
    // Indoor lights: diffuse only (no spec) so they feel softer/less "pinpoint".
    return (plColor * diff) * albedoRgb * att;
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

    // Indoor point lights:
    // - Determine which building we are inside (using lightRects, which can be tighter than roofRects)
    // - Only iterate that building's lights (perf: 8 instead of 48)
    int idx = getLightRectIndex(fragPosition);
    if (idx >= 0 && lightsPerBuilding > 0)
    {
        int base = idx * lightsPerBuilding;
        for (int i = 0; i < lightsPerBuilding; i++)
        {
            int li = base + i;
            if (li >= pointLightCount) break;
            lit += apply_point_light(albedo.rgb, N, fragPosition, pointLightsPosRadius[li], pointLightsColor[li]);
        }
    }

    finalColor = vec4(lit, albedo.a);
}

