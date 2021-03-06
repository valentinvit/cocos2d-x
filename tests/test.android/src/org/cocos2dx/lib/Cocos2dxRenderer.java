package org.cocos2dx.lib;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLSurfaceView;

public class Cocos2dxRenderer implements GLSurfaceView.Renderer {
	private final static long NANOSECONDSPERSECOND = 1000000000L;
	private final static long NANOSECONDSPERMINISECOND = 1000000;
	private static long animationInterval = (long)(1.0 / 60 * NANOSECONDSPERSECOND);
	private long last;
	
    public void onSurfaceCreated(GL10 gl, EGLConfig config) { 	
    	nativeInit(Cocos2dxActivity.screenWidth, Cocos2dxActivity.screenHeight); 
    	last = System.nanoTime();
    }

    public void onSurfaceChanged(GL10 gl, int w, int h) {  	
    }
    
    public void onDrawFrame(GL10 gl) {
    	
    	long now = System.nanoTime();
    	long interval = now - last;
    	
    	// should render a frame when onDrawFrame() is called
    	// or there is a "ghost"
    	nativeRender();   	
   	
    	// fps controlling
    	if (interval < animationInterval){ 
    		try {
    			// because we render it before, so we should sleep twice time interval
    			Thread.sleep((animationInterval - interval) * 2 / NANOSECONDSPERMINISECOND);
    		} catch (Exception e){}
    	}	
    	
    	last = now;
    }
    
    public void handleActionDown(float x, float y)
    {
    	nativeTouchesBegin(x, y);
    }
    
    public void handleActionUp(float x, float y)
    {
    	nativeTouchesEnd(x, y);
    }
    
    public void handleActionCancel(float x, float y)
    {
    	nativeTouchesCancel(x, y);
    }
    
    public void handleActionMove(float x, float y)
    {
    	nativeTouchesMove(x, y);
    }
    
    public static void setAnimationInterval(double interval){
    	animationInterval = (long)(interval * NANOSECONDSPERSECOND);
    }
    
    private static native void nativeTouchesBegin(float x, float y);
    private static native void nativeTouchesEnd(float x, float y);
    private static native void nativeTouchesMove(float x, float y);
    private static native void nativeTouchesCancel(float x, float y);
    private static native void nativeRender();
    private static native void nativeInit(int w, int h);
}