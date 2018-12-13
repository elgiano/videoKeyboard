//
//  imagePlayer.cpp
//  videoKeyboard
//
//  Created by Gnlc Elia on 11/12/2018.
//

#include "imagePlayer.hpp"

// LOADING

bool ImagePlayer::load(std::string text){
    image.load(text);
    
};

// OTHER

ofTexture* ImagePlayer::getTexture(){
    return &image.getTexture();
};


void ImagePlayer::invert(){
    isInverted = !isInverted;

    ofPixels invertedPixs = image.getPixels();
    
    if(isInverted){
        for(int i = 0; i < invertedPixs.getHeight()*invertedPixs.getWidth();i++) {
            int index = i*invertedPixs.getNumChannels();
            invertedPixs[index] =  255-invertedPixs[index];
            invertedPixs[index+1] =  255-invertedPixs[index+1];
            invertedPixs[index+2] =  255-invertedPixs[index+2];
            
        }
    }
    
    image.getTexture().loadData(invertedPixs);
};

void ImagePlayer::update(){
    if(inverted!=isInverted) invert();
}


void ImagePlayer::play(){
    this->playing = true;
};
void ImagePlayer::stop(){
    this->playing = false;
};


float ImagePlayer::getWidth() const{ return this->image.getWidth();};
float ImagePlayer::getHeight() const{ return this->image.getHeight();};

bool  ImagePlayer::isPlaying() const{ return this->playing;};
