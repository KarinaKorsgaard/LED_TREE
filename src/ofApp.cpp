#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    ofEnableSmoothing();
    ofSetVerticalSync(true);
    
    setupGui();
    syncOSC.setup((ofParameterGroup&)gui.getParameter(),OSCRECEIVEPORT,"localhost",OSCSENDPORT);
    syncOSC.update();
    
    lampVis = *new LampVis;
    lampVis.setup();
    
    //shaders
    
    glowShader.load("shaders/glowLines");
    
    
    //main render setup
    render.allocate(RES_W,RES_H);
    render.begin();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render.end();
    
    //texture render
    meshFbo.allocate(RES_W,RES_H);
    meshFbo.begin();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    meshFbo.end();
    
    lamps.allocate(LAMP_W,LAMP_H);
    lamps.begin();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    lamps.end();
    
    syphon.setName("LED_TREE");
    syphonLamp.setName("LED_TREE_LAMPS");
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
    syncOSC.update();
    
    if(syncColors){
        colorBlink = grafiks;
        cloudColor = textures;
        glowColor = textures;
    }
    
    drawnEdge = drawnEdge*0.9+edge*0.1;
    
    
    
    if(transition && !transistionBegun){
        transistionBegun = true;
        transition = false;
        ofPixels pix;
        vector<ofVec2f>pos;
        vector<ofColor>col;
        render.getTexture().readToPixels(pix);
        for(int x = 0; x<RES_W; x+=RES_W/numTreePoles){
            for(int y = RES_H-1; y > (1-edge)*RES_H; y--){
                col.push_back(pix.getColor(x+2, y));
                pos.push_back(ofVec2f(x, y));
            }
        }
        TransPix tp(pos,col);
        tp.edge = (1-edge)*RES_H;
        tp.orgEdge = (1-edge)*RES_H;
        transPix.push_back(tp);
    }
    for (vector<TransPix>::iterator it=transPix.begin(); it!=transPix.end();)    {
        it->update();
        if(it->pix.size()<=0){
            it = transPix.erase(it);
            transistionBegun = false;
            edge = 0.01;
            drawnEdge = 0;
        }
        else{
            ++it;
        }
    }
    
    
    counterGlow += tempoGlow;
    counterCloud += tempoCloud/100;
    
    //update unwarped image (blinks & dataRain)
    setGraficVals();
    
    //update warped image (glow shader)
    setShaderVals();
    
    lampVis.update(tempoCloud/10,zoomCloud,balance,contrast,enableFBM,enableRMF, cloudColor, intensityLamp/100);
    
    lamps.begin();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ofBackground(0);
    lampVis.draw();
    lamps.end();
    
    
    //tree trunk render
    render.begin();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ofBackground(0);
    
    if(!transistionBegun){
        //draw textures in masks
        if(interActiveMasks){
            if(glow){
                vector<float>vec;
                vec.resize(masksFader.size(), 0);
                for(int i = 0 ; i<rainDrops.size();i++){
                    if(rainDrops[i].updatePoint){
                        vec[rainDrops[i].num] = rainDrops[i].point;
                    }
                }
                drawGradient(0, 0, RES_W, RES_H, drawnEdge, meshFbo.getTexture(), vec);
            }
        }
        
        //draw textures unwarped (stretched)
        if(!interActiveMasks){
            if(glow){
                vector<float>vec;
                for(int i = 0 ; i<masksFader.size();i++){
                    vec.push_back(0);
                }
                drawGradient(0, 0, RES_W, RES_H, drawnEdge, meshFbo.getTexture(), vec);
            }
        }
    }
    
    if(b_blink){
        for(auto Blink:blinks){
            Blink.draw();
        }
    }
    for(auto Rain:rainDrops){ //unused code
        Rain.draw();
    }
    
    if(transistionBegun && transPix.size()>0){
        if(gradientColor){
            ofRectGradient(0, 0, RES_W, RES_H/2, Fx1ColorTopTop, Fx1ColorTopBot, OF_GRADIENT_LINEAR);
        }
        transPix[0].draw();
    }
    render.end();
    
    
    ofFill();
    syphon.publishTexture(&render.getTexture());
    syphonLamp.publishTexture(&lamps.getTexture());
}

//--------------------------------------------------------------
void ofApp::draw(){
    if(pixelPreview){
        render.draw(gui.getWidth()+20,0);
        lamps.draw(gui.getWidth()+20+render.getWidth(),0);
        //        ofBackground(0);
        //        ofPixels pix;
        //        render.getTexture().readToPixels(pix);
        //
        //        ofPushMatrix();
        //        ofTranslate(gui.getWidth()+50-RES_W/2, 20);
        //        ofSetLineWidth(0);
        //
        //        float r,g,b,a;
        //
        //        // pixel preview, draw offset at edge to chech how it stitches
        //        int inc = RES_W/10;
        //        for(int x = RES_W/2 ; x< RES_W ; x+=inc){
        //
        //            for(int y = 0; y<RES_H; y+=1){
        //                ofSetColor(pix.getColor(x+5, y));
        //                ofDrawRectangle(x, y, (inc)/2, 1);
        //            }
        //        }
        //        ofTranslate(RES_W, 0);
        //        for(int x = 0 ; x< RES_W/2 ; x+=inc){
        //
        //            for(int y = 0; y<RES_H; y+=1){
        //                ofSetColor(pix.getColor(x+5, y));
        //                ofDrawRectangle(x, y, (inc)/2, 1);
        //            }
        //        }
        //        ofTranslate(RES_W, 300);
        //       // ofDrawRectangle(0, 700, 500,500);
        //        for(int x = 0 ; x< RES_W ; x+=10){
        //
        //            for(int y = 700; y<RES_H; y+=1){
        //
        //                ofSetColor(pix.getColor(x+5, y));
        //                ofDrawRectangle(x, (y-700)*5, 5, 5);
        //            }
        //        }
        //        ofSetColor(255);
        //        ofNoFill();
        //        //ofDrawRectangle(0, 0, RES_W, RES_H);
        //        ofDrawBitmapString("pixel preview ", 0, -10);
        
        ofPopMatrix();
    }
    
    gui.draw();
    ofSetWindowTitle(ofToString(ofGetFrameRate()));
    
}
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------

//------
void ofApp::setShaderVals(){
    meshFbo.begin();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    
    ofBackground(0);
    if(gradientColor){
        ofRectGradient(0, 0, RES_W, RES_H/2, Fx1ColorTopTop, Fx1ColorTopBot, OF_GRADIENT_LINEAR);
    }
    
    if(glow){
        glowShader.begin();
        glowShader.setUniform2f("iResolution", RES_W, RES_H*3);
        glowShader.setUniform1f("iGlobalTime", counterGlow);
        glowShader.setUniform1f("u_density", glowDensity);
        glowShader.setUniform1f("u_amount", u_amount);
        glowShader.setUniform3f("u_color", float(glowColor->r)/255,float(glowColor->g)/255,float(glowColor->b)/255);
        glowShader.setUniform1f("horisontal", glowHorisontal);
        ofSetColor(255);
        ofFill();
        ofDrawRectangle(0, RES_H/2, RES_W, RES_H/2);
        glowShader.end();
    }
    meshFbo.end();
}

void ofApp::setGraficVals(){
    
    //rainDrops:
    for(int i = 0; i<masksFader.size();i++){
        if(masksFader[i]==1){
            masksFader[i]=0;
            DataRain rain;
            rain.num = i;
            rain.edge = 1-edge;
            rain.pos = ofVec2f(i*(RES_W/numTreePoles),0);
            rainDrops.push_back(rain);
            
        }
    }
    for (vector<DataRain>::iterator it=rainDrops.begin(); it!=rainDrops.end();)    {
        it->update(1-edge);
        if(it->isDead()){
            it = rainDrops.erase(it);
            edge+=(float(1-edge)/EDGEGAIN);
        }
        else{
            ++it;
        }
    }
    
    // Blinkedne blinks
    for(int i = 0; i < 10 ; i++){
        if(ofRandom(100000)/100000 < blinkIntensity){
            Blink blink;
            blink.blinkColor = colorBlink;
            int resH = ofRandom(RES_H);
            blink.location = ofVec2f(((int)ofRandom(RES_W)),(int)ofRandom(resH));
            blink.tempo = blinkTempo;
            blink.hard_soft = hard_soft;
            
            blinks.push_back(blink);
        }
    }
    
    for (vector<Blink>::iterator it=blinks.begin(); it!=blinks.end();)    {
        it->update();
        if(it->isDead())
            it = blinks.erase(it);
        else
            ++it;
    }

    
}

//--------------------------------------------------------------
void ofApp::exit(){
    gui.saveToFile("settings.xml");
}

//--------------------------------------------------------------

void ofApp::setupGui(){
    guiGroup.setName("guiGroup");
    
    vector<string>n;
    n.push_back("maskI");
    n.push_back("maskII");
    n.push_back("maskIII");
    n.push_back("maskIV");
    n.push_back("maskV");
    n.push_back("maskVI");
    n.push_back("maskVII");
    n.push_back("maskVIII");
    n.push_back("maskIX");
    n.push_back("maskX");
    
    enable.setName("enable");
    masksFader.resize(numTreePoles);
    for(int i =0; i<numTreePoles ; i++){
        masksFader[i].set(n[i],0,0,1);
        enable.add(masksFader[i]);
    }
    
    backgroud_Grad.setName("backgroud_Grad");
    backgroud_Grad.add(Fx1ColorTopTop.set("Backgroud_TopTop", ofColor(0,0), ofColor(0,0),ofColor(255)));
    backgroud_Grad.add(Fx1ColorTopBot.set("Backgroud_TopBot", ofColor(0,0), ofColor(0,0),ofColor(255)));
    
    mainControl.setName("mainControl");
    mainControl.add(pixelPreview.set("preview", false));
    mainControl.add(gradientColor.set("gradientColor", false));
    mainControl.add(glow.set("glow", false));
    mainControl.add(b_blink.set("b_blink", false));
    mainControl.add(interActiveMasks.set("interActiveMasks", false));
    mainControl.add(transition.set("transition", false));
    mainControl.add(intensityLamp.set("intensityLamp", 0.5, 0., 1));
    mainControl.add(edge.set("edge", 0.5, 0., 1));
    mainControl.add(syncColors.set("syncColors", false));
    mainControl.add(grafiks.set("grafiks", ofColor(255,255), ofColor(0,0), ofColor(255,255)));
    mainControl.add(textures.set("textures", ofColor(255,255), ofColor(0,0), ofColor(255,255)));
    
    ofParameterGroup waveControl;
    waveControl.setName("waveControl");
    ofParameterGroup waveControlBlink;
    waveControlBlink.setName("waveControlBlink");
    waveControlBlink.add(blinkIntensity.set("blinkIntensity", 0.0, 0.0, 1.0));
    waveControlBlink.add(blinkTempo.set("blinkTempo", 0.02, 0.0001, 0.1));
    waveControlBlink.add(hard_soft.set("hard/soft", true));
    waveControlBlink.add(colorBlink.set("colorBlink", ofColor(255,255), ofColor(0,0), ofColor(255,255)));
    
    waveControl.add(waveControlBlink);
    //textures
    paramGeneral.setName("paramGeneral");
    paramGlow.setName("paramGlow");
    paramGlow.add(tempoGlow.set("tempoGlow", 0.1, 0., 1));
    paramGlow.add(glowDensity.set("glowDensity",0.5,0.,1.));
    paramGlow.add(u_amount.set("u_amount",0.5,0.,1.));
    paramGlow.add(glowHorisontal.set("glowHorisontal",false));
    paramGlow.add(glowColor.set("glowColor", ofColor(0,154,255,255), ofColor(0,0),ofColor(255)));
    
    // param for Cloud
    paramCloud.setName("paramCloud");
    paramCloud.add(tempoCloud.set("tempoCloud", 0.1, 0., 1));
    paramCloud.add(zoomCloud.set("zoomCloud", 0.1, 0., 1));
    paramCloud.add(balance.set("balance",0.5,0.,1.));
    paramCloud.add(contrast.set("contrast",0.,-1,1.));
    paramCloud.add(enableFBM.set("enableFBM", true));
    paramCloud.add(enableRMF.set("enableRMF", true));
    paramCloud.add(cloudColor.set("cloudColor", ofColor(0,154,255,255), ofColor(0,0),ofColor(255)));

    paramGeneral.add(paramGlow);
    paramGeneral.add(paramCloud);
    
    guiGroup.add(enable);
    guiGroup.add(mainControl);
    guiGroup.add(waveControl);
    guiGroup.add(backgroud_Grad);
    guiGroup.add(paramGeneral);
    
    gui.setup(guiGroup);
    
    gui.loadFromFile("settings.xml");
    gui.minimizeAll();
    
}

void ofApp::ofRectGradient(int px, int py, int w, int h,const ofColor& start, const ofColor& end, ofGradientMode mode){
    ofVboMesh gradientMesh;
    gradientMesh.clear();
    gradientMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
#ifndef TARGET_EMSCRIPTEN
#ifdef TARGET_OPENGLES
    if(ofIsGLProgrammableRenderer()) gradientMesh.setUsage(GL_STREAM_DRAW);
#else
    gradientMesh.setUsage(GL_STREAM_DRAW);
#endif
#endif
    if(mode == OF_GRADIENT_CIRCULAR) {
        // this could be optimized by building a single mesh once, then copying
        // it and just adding the colors whenever the function is called.
        ofVec2f center(w / 2 + px, h / 2 + py);
        gradientMesh.addVertex(center);
        gradientMesh.addColor(start);
        int n = 32; // circular gradient resolution
        float angleBisector = TWO_PI / (n * 2);
        float smallRadius = ofDist(0, 0, w / 2, h / 2);
        float bigRadius = smallRadius / cos(angleBisector);
        for(int i = 0; i <= n; i++) {
            float theta = i * TWO_PI / n;
            gradientMesh.addVertex(center + ofVec2f(sin(theta), cos(theta)) * bigRadius);
            gradientMesh.addColor(end);
        }
    } else if(mode == OF_GRADIENT_LINEAR) {
        gradientMesh.addVertex(ofVec2f(px, py));
        gradientMesh.addVertex(ofVec2f(px+w, py));
        gradientMesh.addVertex(ofVec2f(px+w, py+h));
        gradientMesh.addVertex(ofVec2f(px, py+h));
        gradientMesh.addColor(start);
        gradientMesh.addColor(start);
        gradientMesh.addColor(end);
        gradientMesh.addColor(end);
    } else if(mode == OF_GRADIENT_BAR) {
        gradientMesh.addVertex(ofVec2f(px+w / 2, py+h / 2));
        gradientMesh.addVertex(ofVec2f(px, py+h / 2));
        gradientMesh.addVertex(ofVec2f(px, py));
        gradientMesh.addVertex(ofVec2f(px+w, py));
        gradientMesh.addVertex(ofVec2f(px+w, py+h / 2));
        gradientMesh.addVertex(ofVec2f(px+w, py+h));
        gradientMesh.addVertex(ofVec2f(px, py+h));
        gradientMesh.addVertex(ofVec2f(px, py+h / 2));
        gradientMesh.addColor(start);
        gradientMesh.addColor(start);
        gradientMesh.addColor(end);
        gradientMesh.addColor(end);
        gradientMesh.addColor(start);
        gradientMesh.addColor(end);
        gradientMesh.addColor(end);
        gradientMesh.addColor(start);
    }
    GLboolean depthMaskEnabled;
    glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMaskEnabled);
    glDepthMask(GL_FALSE);
    gradientMesh.draw();
    if(depthMaskEnabled){
        glDepthMask(GL_TRUE);
    }
}

void ofApp:: drawGradient(int _x, int _y, int _w, int _h, float _posHWave , ofTexture& _texture, vector<float>vec){
    ofPushMatrix();
    ofTranslate(_x,_y);
    float inc = float(vec.size())/_w;
    
    float incTex = _texture.getWidth() / _w;
    float texHeight = _texture.getHeight();
    float texHalfHeight = texHeight/2;
    
    ofMesh mesh;
    mesh.clear();
    mesh.enableTextures();
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);
    
    float min = 0.;
    float max = 1.;
    
    for(int i = 0; i < _w; i++){
        float x = i;
        int u =int(float(i)*inc);
        
        float y;
        
        y = (2*(vec[u])/2+1-_posHWave)*_h;
        
        
        ofFloatColor pColor = ofFloatColor(1., ofMap( vec[u]*-1, min, max, 0.9, 1, true));
        
        mesh.addVertex(ofVec3f(x, 0, 0));
        mesh.addTexCoord(ofVec2f(x,0));
        mesh.addColor(pColor);
        
        mesh.addVertex(ofVec3f(x, y, 0));
        mesh.addTexCoord(ofVec2f(x,texHalfHeight));
        mesh.addColor(pColor);
        
        mesh.addVertex(ofVec3f(x, _h, 0));
        mesh.addTexCoord(ofVec2f(x,texHeight));
        mesh.addColor(pColor);
    }
    
    for(int i = 0; i < mesh.getNumVertices()/3-1; i++){
        mesh.addIndex(i*3+1);
        mesh.addIndex(i*3+3);
        mesh.addIndex(i*3+0);
        
        mesh.addIndex(i*3+1);
        mesh.addIndex(i*3+4);
        mesh.addIndex(i*3+3);
        
        mesh.addIndex(i*3+2);
        mesh.addIndex(i*3+4);
        mesh.addIndex(i*3+1);
        
        mesh.addIndex(i*3+2);
        mesh.addIndex(i*3+5);
        mesh.addIndex(i*3+4);
    }
    ofSetColor( 255, 255, 255 );  //Set white color
    _texture.bind();
    mesh.draw();
    _texture.unbind();
    
    ofPopMatrix();
}


//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    for(int i = 0; i<10 ;i ++){
        char c = ofToChar(ofToString(i));
        if(key == c ){
            masksFader[i] = 1;
        }
    }
}
