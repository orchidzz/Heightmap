#version 150

in vec3 position;
in vec4 color;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform int mode;
uniform int topPolygon;
uniform float exponent;
uniform float scale;

in vec3 center;
in vec3 left;
in vec3 right;
in vec3 up;
in vec3 down;


// from https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(float seed) {
    return fract(sin(seed* 12.9898) * 43758.5453);
}

void main()
{
	if(mode == 0){
		// compute the transformed and projected vertex position (into gl_Position) 
		// compute the vertex color (into col)
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
		if(topPolygon == 1){
			col = vec4(0, 0, 0, 1.0f);
		}else{
			col = color;
		}
	}else if (mode == 1){
		//smoothening
		// vertices' averages
		vec3 ave = vec3((center.x+left.x+right.x+up.x+down.x)/5.0f/9.0f, (center.y+left.y+right.y+up.y+down.y)/5.0f, (center.z+left.z+right.z+up.z+down.z)/5.0f/9.0f);
		
		// color
		col = vec4(pow(ave.y, exponent), pow(ave.y, exponent), pow(ave.y, exponent), 1.0f);
		// vertices
		gl_Position = projectionMatrix * modelViewMatrix * vec4(ave.x,scale*pow(ave.y,exponent),ave.z, 1.0f);
	} else if(mode == 2){
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
		col = vec4(rand(color.x), rand(color.y), rand(color.z), 1.0f);
	} 
  
}

