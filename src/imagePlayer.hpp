//
//  imagePlayer.hpp
//  videoKeyboard
//
//  Created by Gnlc Elia on 11/12/2018.
//

#ifndef imagePlayer_hpp
#define imagePlayer_hpp

#include <stdio.h>
#include <string>
#include "ofMain.h"

class ImagePlayer{
    
    
public:


    
    bool load(std::string text);
    
    
    ofTexture *getTexture();
    
    void play();
    void stop();
    
    float getWidth() const;
    float getHeight() const;

    
    bool  isPlaying() const;
    
private:
    ofImage image;
    bool playing;
    
};


#endif /* imagePlayer_hpp */
