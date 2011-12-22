uniform sampler2D tex;
varying vec2 fPosition;
void main(void){
	gl_FragColor = texture2D(tex,fPosition);
	//outColor = vec4(fPosition,fPosition);
}
