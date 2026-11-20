#version 330 core
in vec3 vNear;
in vec3 vFar;
out vec4 FragColor;

uniform mat4  u_viewProj;    // for correct depth output
uniform vec3  u_camPos;
uniform float u_spacing;     // minor cell size (world units)
uniform float u_fadeStart;   // distance where fading begins
uniform float u_fadeEnd;     // distance where the grid is fully gone

// Anti-aliased grid factor for a given cell size (0..1, 1 = on a line).
float gridFactor(vec2 worldXZ, float cell)
{
	vec2 coord = worldXZ / cell;
	vec2 deriv = fwidth(coord);
	vec2 g = abs(fract(coord - 0.5) - 0.5) / max(deriv, vec2(1e-6));
	float line = min(g.x, g.y);
	return 1.0 - min(line, 1.0);
}

void main()
{
	// Intersect the view ray with the y=0 plane.
	float denom = vFar.y - vNear.y;
	float t = -vNear.y / denom;
	if (t <= 0.0 || abs(denom) < 1e-7) discard; // plane behind camera / parallel

	vec3 pos = vNear + t * (vFar - vNear);
	vec2 xz = pos.xz;

	// Minor + major lines.
	float minorA = gridFactor(xz, u_spacing);
	float majorA = gridFactor(xz, u_spacing * 10.0);

	vec3 minorColor = vec3(0.32, 0.32, 0.36);
	vec3 majorColor = vec3(0.50, 0.50, 0.56);
	vec3 color = minorColor;
	float alpha = minorA * 0.6;
	if (majorA > alpha) { color = majorColor; alpha = majorA; }

	// World axis highlights (within ~half a minor cell of the axis).
	vec2 deriv = fwidth(xz);
	if (abs(pos.z) < deriv.y * 1.0) { color = vec3(0.80, 0.25, 0.25); alpha = max(alpha, 1.0); } // X axis
	if (abs(pos.x) < deriv.x * 1.0) { color = vec3(0.25, 0.45, 0.85); alpha = max(alpha, 1.0); } // Z axis

	// Distance fade (prevents harsh clipping / shimmer at the horizon).
	float dist = length(pos - u_camPos);
	float fade = 1.0 - smoothstep(u_fadeStart, u_fadeEnd, dist);
	alpha *= fade;
	if (alpha <= 0.002) discard;

	// Correct depth so scene objects occlude the grid.
	vec4 clip = u_viewProj * vec4(pos, 1.0);
	float ndcDepth = clip.z / clip.w;
	gl_FragDepth = ndcDepth * 0.5 + 0.5;

	FragColor = vec4(color, alpha);
}
