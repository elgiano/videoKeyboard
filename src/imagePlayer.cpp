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

void ImagePlayer::play(){
    this->playing = true;
};
void ImagePlayer::stop(){
    this->playing = false;
};


float ImagePlayer::getWidth() const{ return this->image.getWidth();};
float ImagePlayer::getHeight() const{ return this->image.getHeight();};

bool  ImagePlayer::isPlaying() const{ return this->playing;};
