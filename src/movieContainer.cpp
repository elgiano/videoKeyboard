//
//  movieContainer.cpp
//  videoKeyboard64
//
//  Created by Gnlc Elia on 24/11/2018.
//

#include "movieContainer.hpp"

bool MovieContainer::load(std::string name){
    
    // identify content format
    std::string ext = ofFilePath::getFileExt(name);
    if(ext == "mov"){
        contentType=MovieType::hap;
        hapMovie = new ofxHapPlayer();
        return hapMovie->load(name);
    }else if (ext == "txt"){
        contentType=MovieType::txt;
        fontPlayer = new FontPlayer();
        ofBuffer buffer = ofBufferFromFile(name); // reading into the buffer
        return fontPlayer->load(buffer.getText());
    }
    
};

ofTexture * MovieContainer::getTexture(){
    if(contentType==MovieType::hap){
        return hapMovie->getTexture();
    }else if(contentType==MovieType::txt){
        return fontPlayer->getTexture();
    }
    
}
float  MovieContainer::getWidth() const{
    if(contentType==MovieType::hap){
        return hapMovie->getWidth();
    }else if(contentType==MovieType::txt){
        return fontPlayer->getWidth();
    }
}
float MovieContainer::getHeight() const{
    if(contentType==MovieType::hap){
        return hapMovie->getHeight();
    }else if(contentType==MovieType::txt){
        return fontPlayer->getHeight();
    }
};

float MovieContainer::getPosition() const{
    if(contentType==MovieType::hap){
        return hapMovie->getPosition();
    }else if(contentType==MovieType::txt){
        return fontPlayer->getPosition();
    }
};
void  MovieContainer::setPosition(float pct){
    if(contentType==MovieType::hap){
        return hapMovie->setPosition(pct);
    }else if(contentType==MovieType::txt){
        return fontPlayer->setPosition(pct);
    }
};
float MovieContainer::getDuration() const{
    if(contentType==MovieType::hap){
        return hapMovie->getDuration();
    }else if(contentType==MovieType::txt){
        return fontPlayer->getDuration();
    }
};

void  MovieContainer::setLoopState(ofLoopType state){
    if(contentType==MovieType::hap){
        return hapMovie->setLoopState(state);
    }/*else if(contentType==MovieType::txt){
        return fontPlayer->setLoopState(state);
    }*/
};

ofLoopType MovieContainer::getLoopState() const{
    if(contentType==MovieType::hap){
        return hapMovie->getLoopState();
    }/*else if(contentType==MovieType::txt){
        return fontPlayer->getLoopState();
    }*/
};

bool  MovieContainer::isPlaying() const{
    if(contentType==MovieType::hap){
        return hapMovie->isPlaying();
    }else if(contentType==MovieType::txt){
        return fontPlayer->isPlaying();
    }
};
void  MovieContainer::setSpeed(float speed){
    if(contentType==MovieType::hap){
        return hapMovie->setSpeed(speed);
    }else if(contentType==MovieType::txt){
        return fontPlayer->setSpeed(speed);
    }
};
void  MovieContainer::setVolume(float volume){
    if(contentType==MovieType::hap){
        return hapMovie->setVolume(1.0);
    }
}; // 0..1

void  MovieContainer::play(){
    if(contentType==MovieType::hap){
        return hapMovie->play();
    }else if(contentType==MovieType::txt){
        return fontPlayer->play();
    }
};

void  MovieContainer::stop(){
    if(contentType==MovieType::hap){
        return hapMovie->stop();
    }else if(contentType==MovieType::txt){
        return fontPlayer->stop();
    }
};

void  MovieContainer::nextFrame(){
    if(contentType==MovieType::hap){
        return hapMovie->nextFrame();
    }else if(contentType==MovieType::txt){
        return fontPlayer->nextFrame();
    }
};

void  MovieContainer::update(){
    if(contentType==MovieType::hap){
        return hapMovie->update();
    }else if(contentType==MovieType::txt){
        return fontPlayer->update();
    }
};

void  MovieContainer::setColor(ofColor col){
    if(contentType==MovieType::txt){
        return fontPlayer->color = col;
    }
};
