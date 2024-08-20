 #version 410 core
out vec4 FragColor;
//layout(location = 1) in vec2 fragCoord;
in vec3 ourColor;
in vec2 TexCoord;
in vec4 gl_FragCoord;

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
uniform sampler2D iChannel0;

struct PointLight {
    vec2 pos;
    vec3 col;
    float intensity;
};


float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float palette( in float a, in float b, in float c, in float d, in float x ) {
    return a + b * cos(6.28318 * (c * x + d));
}


float Noise2D( in vec2 x )
{
    ivec2 p = ivec2(floor(x));
    vec2 f = fract(x);
	f = f*f*(3.0-2.0*f);
	ivec2 uv = p.xy;
	float rgA = texelFetch( iChannel0, (uv+ivec2(0,0))&255, 0 ).x;
    float rgB = texelFetch( iChannel0, (uv+ivec2(1,0))&255, 0 ).x;
    float rgC = texelFetch( iChannel0, (uv+ivec2(0,1))&255, 0 ).x;
    float rgD = texelFetch( iChannel0, (uv+ivec2(1,1))&255, 0 ).x;
    return mix( mix( rgA, rgB, f.x ),
                mix( rgC, rgD, f.x ), f.y );
}
/*float Noise2D(vec2 p){
	vec2 ip = floor(p);
	vec2 u = fract(p);
	u = u*u*(3.0-2.0*u);
	
	float res = mix(
		mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
		mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
	return res*res;
}*/

float ComputeFBM( in vec2 pos )
{
    float amplitude = 0.75;
    float sum = 0.0;
    float maxAmp = 0.0;
    for(int i = 0; i < 6; ++i)
    {
        sum += Noise2D(pos) * amplitude;
        maxAmp += amplitude;
        amplitude *= 0.5;
        pos *= 2.2;
    }
    return sum / maxAmp;
}

// Same function but with a different, constant amount of octaves
float ComputeFBMStars( in vec2 pos )
{
    float amplitude = 0.75;
    float sum = 0.0;
    float maxAmp = 0.0;
    for(int i = 0; i < 5; ++i)
    {
        sum += Noise2D(pos) * amplitude;
        maxAmp += amplitude;
        amplitude *= 0.5;
        pos *= 2.0;
    }
    return sum / maxAmp * 1.15;
}

vec3 BackgroundColor( in vec2 uv ) {
    
    // Sample various noises and multiply them
    float noise1 = ComputeFBMStars(uv * 5.0);
    float noise2 = ComputeFBMStars(uv * vec2(15.125, 25.7));
    float noise3 = ComputeFBMStars((uv + vec2(0.5, 0.1)) * 4.0 + u_time * 0.35);
    float starShape = noise1 * noise2 * noise3;
    
    // Compute star falloff - not really doing what i hoped it would, i wanted smooth falloff around each star
    float falloffRadius = 0.2;
    float baseThreshold = 0.45; // higher = less stars 0.6
    
    starShape = clamp(starShape - baseThreshold + falloffRadius, 0.0, 1.0);
    
    float weight = starShape / (2.0 * falloffRadius);
    return weight * vec3(noise1 * 0.55, noise2 * 0.4, noise3 * 1.0) * 6.0; // artificial scale just makes the stars brighter
}

void main()
{
	vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    vec2 scrPt = uv * 2.0 - 1.0;
    
    vec4 finalColor; 
    
	// Define density for some shape representing the milky way galaxy
    
    float milkywayShape;
    
    // Distort input screen pos slightly so the galaxy isnt perfectly axis aligned
    float galaxyOffset = 0.0; //(cos(scrPt.x * 5.0) * sin(scrPt.x * 2.0) * 0.5 + 0.5) * 0.0;
    
    // Apply a slight rotation to the screen point, similar to the galaxy
    float theta = length(scrPt) * 0.25; // Visualy tweaked until it looked natural
    mat2 rot;
    
    // cache calls to sin/cos(theta)
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    
    rot[0][0] = cosTheta;
    rot[0][1] = -sinTheta;
    rot[1][0] = sinTheta;
    rot[1][1] = cosTheta;
    
    vec2 rotatedScrPt = scrPt * rot;
    
    float noiseVal = ComputeFBM(rotatedScrPt * 5.0 + 50.0 + u_time * 0.015625 * 1.5);
    
    rotatedScrPt += vec2(noiseVal) * 0.3;
    
    float centralFalloff = clamp(1.0 - length(scrPt.y + galaxyOffset), 0.0, 1.0);
    float xDirFalloff = (cos(scrPt.x * 2.0) * 0.5 + 0.5);
    
    float centralFalloff_rot = 1.0 - length(rotatedScrPt.y + galaxyOffset);
    float xDirFalloff_rot = (cos(rotatedScrPt.x * 2.0) * 0.5 + 0.5);
    
    // Falloff in y dir and x-dir fade
    float lowFreqNoiseForFalloff = ComputeFBM(rotatedScrPt * 4.0 - u_time * 0.015625 * 1.5); // 1/64
    //milkywayShape = clamp(pow(centralFalloff_rot, 3.0) - lowFreqNoiseForFalloff * 0.5, 0.0, 1.0) * xDirFalloff_rot;
    
    // Add some blue to the background
    vec3 backgroundCol = BackgroundColor(gl_FragCoord.xy * 0.125) * pow(centralFalloff, 0.5) * pow(xDirFalloff, 0.5);
    vec3 blueish = vec3(0.2, 0.2, 0.4);
    backgroundCol += blueish * (5.0 - milkywayShape) * pow(centralFalloff_rot, 2.0) * lowFreqNoiseForFalloff * pow(xDirFalloff, 0.75);
    
    vec3 whiteish = vec3(0.5, 1.0, 0.85);
    backgroundCol += whiteish * 0.95 * pow(centralFalloff, 1.5) * lowFreqNoiseForFalloff * pow(xDirFalloff, 2.0);
    
    finalColor = vec4(backgroundCol, 1);
    //finalColor = vec4(mix(backgroundCol, vec3(0.968, 0.964, 0.937), milkywayShape), 1);
	FragColor = finalColor;
    //FragColor = vec4(1.0, 0, 0, 1.0);
}