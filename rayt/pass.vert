attribute vec4 inScreenPos;
attribute vec2 inCanvasPos;
varying vec2 fPosition;

void main(void){
	gl_Position = inScreenPos;
	fPosition=inCanvasPos;
}