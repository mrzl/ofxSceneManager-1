/*
 *  ofxSceneManager.cpp
 *  Cocoa Test
 *
 *  Created by Oriol Ferrer Mesià on 02/11/09.
 *  Copyright 2009 uri.cat. All rights reserved.
 *
 */

#include "ofxSceneManager.h"
#include "ofxScene.h"

ofxSceneManager* ofxSceneManager::singleton = NULL; 

ofxSceneManager::ofxSceneManager(){

	currentScene = futureScene = lastScene = NULL;
	curtainDropTime = 0.5f;
	curtainStayTime = 0.0f;
	curtainRiseTime = 0.5f;
	drawDebugInfo = false;
	overlapUpdate = false;
	curtain.setAnimationCurve(EASE_IN_EASE_OUT);
}

ofxSceneManager* ofxSceneManager::instance(){
	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new ofxSceneManager();
	}
	return singleton;
}

ofxSceneManager::~ofxSceneManager(){
}

void ofxSceneManager::addScene( ofxScene* newScene, int sceneID ){

	int c = scenes.count( sceneID );
	
	if ( c > 0 ){
		ofLogWarning( "ofxSceneManager::addScene(" + ofToString( sceneID ) + ") >> we already have a scene with that ID" );
	}else{
		newScene->setManager( this );
		newScene->setup();
		newScene->setSceneID( sceneID );
		scenes[sceneID] = newScene;
		//ofLogNotice( "ofxSceneManager::addScene(" + ofToString( sceneID ) + ") >> added scene");
		if (scenes.size() == 1){	//first scene, we activate it			
			goToScene( sceneID, true/*regardless*/, false/*transition*/ ); 
			currentScene = getScene( sceneID );
		}
	}
}


void ofxSceneManager::update( float dt ){

	if ( !curtain.isReady() ){ //curtain is busy, so we are pre or post transition

		curtain.update(dt);
		if ( curtain.hasReachedBottom() ){			
			currentScene->sceneDidDisappear(futureScene);
			futureScene->sceneWillAppear(currentScene);
			updateHistory( futureScene );
			currentScene = futureScene;
			futureScene = NULL;
		}
		
		if ( curtain.hasReachedTop() ){
			currentScene->sceneDidAppear();
		}
	}else{
		futureScene = NULL;	
	}
	
	if (currentScene != NULL){
						
		vector <ofxScene*> screensToUpdate;
		screensToUpdate.push_back(currentScene);
		if (overlapUpdate){
			if (futureScene != NULL){
				screensToUpdate.push_back(futureScene);
			}
			if( lastScene != NULL ) {
				screensToUpdate.push_back( lastScene );
			}
		}
		
		for (int i = 0; i < screensToUpdate.size(); i++){
			ofxScene * s = screensToUpdate[i];
			s->update(dt);
		}
	}
}


void ofxSceneManager::draw(){

	if (currentScene != NULL){
		
		vector <ofxScene*> scenesToDraw;
		scenesToDraw.push_back(currentScene);
		if( futureScene != NULL ) {
			scenesToDraw.push_back( futureScene );
		}
		if( lastScene != NULL ) {
			scenesToDraw.push_back( lastScene );
		}
		
		for (int i = 0; i < scenesToDraw.size(); i++){
			ofxScene * s = scenesToDraw[i];
			s->draw();
			if (drawDebugInfo){
				ofSetColor(255, 0, 0);
				s->drawDebug();
			}
		}
		
		if ( !curtain.isReady() ){
			curtain.draw();
		}
	}
	
	if (drawDebugInfo){
		ofSetColor(255, 0, 0);
		drawDebug();
	}
}

void ofxSceneManager::drawDebug(){

	int y = 20;
	int x = 20;
	int lineHeight = 15;
	if (currentScene){
		ofDrawBitmapString( "Current Scene: " + ofToString(currentScene->getSceneID() ) , x, y);
		y += lineHeight;
	}
	if (futureScene){
		ofDrawBitmapString( "Future Scene: " + ofToString(futureScene->getSceneID()) , x, y);
		y += lineHeight;
	}
	if (!curtain.isReady()){
		ofDrawBitmapString( "Curtain: " + ofToString(curtain.getCurtainAlpha(), 2) , x, y);
	}
}


bool ofxSceneManager::goToScene( int ID, bool regardless, bool doTransition){
	
	if ( curtain.isReady() || regardless){

		ofxScene * next = getScene( ID );
		
		if (doTransition){
			
			if (next != NULL){
				futureScene = next;
				curtain.dropAndRaiseCurtain(curtainDropTime, curtainStayTime, curtainRiseTime, regardless);
				if (currentScene) currentScene->sceneWillDisappear(futureScene);				
				return true;
			}else{
				printf("ofxSceneManager::goToScene(%d) >> scene not found!\n", ID);
			}			
		}else{	
			
			if (next != NULL){
				//notify
				if (currentScene) currentScene->sceneWillDisappear(next);
				next->sceneWillAppear(currentScene);
				if (currentScene) currentScene->sceneDidDisappear(futureScene);
				next->sceneDidAppear();
				//hot-swap current scenes
				lastScene = currentScene;
				currentScene = next;
				futureScene = NULL;
				return true;
			}
		}

	}else{
		if( futureScene != NULL ) {
			ofLogWarning( "ofxSceneManager::goToScene(" + ofToString( ID ) + ") >> CANT DO! we are in the middle of a transition to " + ofToString( futureScene->getSceneID() ) + "!" );
		}
		else {
			ofLogWarning( "ofxSceneManager::goToScene(" + ofToString( ID ) + ") >> CANT DO! we are in the middle of a transition to another scene!" );
		}
	}
	return false;
}


bool ofxSceneManager::nextScene( bool regardless /*= false*/, bool doTransition /*= true */ )
{
	return goToScene( ( currentScene->getSceneID() + 1 ) % scenes.size(), regardless, doTransition );
}

void ofxSceneManager::updateHistory( ofxScene * s ) {
	
	history.push_back(s);
	if (history.size() > MAX_HISTORY_NR){
		history.erase(history.begin());
	}	
}


int ofxSceneManager::getNumScenes(){ 
	return scenes.size(); 
}


ofxScene * ofxSceneManager::getCurrentScene(){
	return currentScene;
}


int ofxSceneManager::getCurrentSceneID(){
	if (currentScene != NULL )
		return currentScene->getSceneID();
	else
		return NULL_SCENE;
}


ofxScene * ofxSceneManager::getScene(int sceneID){
	if ( scenes.count( sceneID ) > 0 ){
		return scenes[sceneID];
	}else{
		return NULL;
	}
};

void ofxSceneManager::setCurtainDropTime(float t){
	curtainDropTime = t;
}

void ofxSceneManager::setCurtainStayTime(float t){
	curtainStayTime = t;
}

void ofxSceneManager::setCurtainRiseTime(float t){
	curtainRiseTime = t;
}

void ofxSceneManager::setCurtainTimes(float drop, float stay, float rise){
	curtainDropTime = drop;
	curtainStayTime = stay;
	curtainRiseTime = rise;
}

void ofxSceneManager::setOverlapUpdate(bool t){
	overlapUpdate = t;
}


/// ALL EVENTS HERE /////////////////////////////////////////////////////////// TODO

void ofxSceneManager::keyPressed(int key){
	if (currentScene != NULL) currentScene->keyPressed( key );
}

void ofxSceneManager::keyReleased(int key){
	if (currentScene != NULL) currentScene->keyReleased( key );
}

void ofxSceneManager::mouseMoved(int x, int y){	
	if (currentScene != NULL) currentScene->mouseMoved( x, y );
}

void ofxSceneManager::mouseDragged(int x, int y, int button){	
	if (currentScene != NULL) currentScene->mouseDragged( x, y, button );
}

void ofxSceneManager::mousePressed(int x, int y, int button){	
	if (currentScene != NULL) currentScene->mousePressed( x, y, button );
}

void ofxSceneManager::mouseReleased(int x, int y, int button){	
	if (currentScene != NULL) currentScene->mouseReleased( x, y, button );
}

void ofxSceneManager::windowResized (int w, int h){
	curtain.setScreenSize( ofRectangle(0, 0, w, h) );
	for( map<int,ofxScene*>::iterator ii = scenes.begin(); ii != scenes.end(); ++ii ){
		//string key = (*ii).first;
		ofxScene* t = (*ii).second;
		t->windowResized(w, h);
	}
}

#ifdef TARGET_OF_IPHONE
void ofxSceneManager::touchDown(ofTouchEventArgs &touch){
	if (currentScene != NULL) currentScene->touchDown( touch );
}

void ofxSceneManager::touchMoved(ofTouchEventArgs &touch){
	if (currentScene != NULL) currentScene->touchMoved( touch );
}

void ofxSceneManager::touchUp(ofTouchEventArgs &touch){
	if (currentScene != NULL) currentScene->touchUp( touch );
}

void ofxSceneManager::touchDoubleTap(ofTouchEventArgs &touch){
	if (currentScene != NULL) currentScene->touchDoubleTap( touch );
}

void ofxSceneManager::touchCancelled(ofTouchEventArgs &touch){
	if (currentScene != NULL) currentScene->touchCancelled( touch );
}
#endif