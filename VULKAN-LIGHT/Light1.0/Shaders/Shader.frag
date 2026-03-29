#version 450 core

// ============================================================
// OVERVIEW:
// This shader implements Blinn-Phong lighting model.
//
// Final color = Ambient + Diffuse + Specular
//
// All calculations are done in WORLD SPACE.
// ============================================================

// ============================
// INPUT FROM VERTEX SHADER
// ============================
// These are interpolated per-pixel



layout(location=0) in vec3 fragPosition;     // world-space position of pixel
layout(location=1) in vec3 outNormal;        // interpolated normal


layout(location=0) out vec4 FragColor;

layout(binding=0) uniform mvpMatrix
{
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 lightPos;
	vec3 viewPos;       //// camera position in world space
}uMVP;

void main(void)
{
     // ============================
    // STEP 1: NORMALIZE NORMAL
    // ============================
    // Interpolation between vertices distorts normals.
    // We must normalize them before using.
	vec3 norm = normalize(outNormal);

    // ============================
    // STEP 2: LIGHT DIRECTION
    // ============================
    // Direction from fragment → light
    //
    // IMPORTANT:
    // Order matters! (lightPos - fragPos)
    vec3 lightDir = normalize(uMVP.lightPos - fragPosition);

    // ============================
    // STEP 3: DIFFUSE LIGHTING (Lambert)
    // ============================
    //
    // Formula:
    // intensity = max(dot(N, L), 0)
    //
    // N = surface normal
    // L = light direction
    //
    // dot(N, L) = cos(angle between them)
    //
    // Cases:
    // Facing light ->positive -->bright
    // Perpendicular -> 0 -> dark
    // Facing away -->negative --> clamped to 0
   
    float diff = max(dot(norm, lightDir), 0.0);

     // White diffuse color (you can replace with material color)
    vec3 diffuse = diff *vec3(1.0, 1.0, 1.0);

    // ============================
    // STEP 4: AMBIENT LIGHT
    // ============================
    //
    // Simulates indirect light bouncing in the environment.
    // Prevents completely black surfaces.
    //
    vec3 ambient = 0.1 * vec3(1.0);


    // ============================
    // STEP 6: HALF VECTOR (Blinn-Phong)
    // ============================
    //
    // Instead of computing reflection vector, we use:
    // halfway = normalize(lightDir + viewDir)
    //
    // This is faster and commonly used in real-time rendering.
    //
    vec3 viewDir = normalize(uMVP.viewPos - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);

    // ============================
    // STEP 7: SPECULAR LIGHTING
    // ============================
    //
    // Formula:
    // spec = pow(max(dot(N, H), 0), shininess)
    //
    // - H = halfway vector
    // - shininess controls highlight sharpness
    //
    float spec = pow(max(dot(norm, halfDir), 0.0), 32);
    vec3 specular = spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;

	FragColor = vec4(result, 1.0);
	//FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

