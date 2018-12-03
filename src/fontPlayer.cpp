//
//  fontPlayer.cpp
//  fontTest
//
//  Created by Gnlc Elia on 24/11/2018.
//

#include "fontPlayer.hpp"

bool FontPlayer::load(std::string text,int size){
    
    if(!this->fbo.isAllocated()){
        this->fbo.allocate(ofGetScreenWidth(),ofGetScreenHeight(),GL_RGBA);
        this->maskFbo.allocate(ofGetScreenWidth(),ofGetScreenHeight(),GL_RGBA);
    }
    
    this->text = text;
    this->setFontSize(size);

    return true;
};
bool FontPlayer::load(std::string text){
    this->load(text,fontSize);
};

std::string FontPlayer::setFontSize(int size){
    this->font.load("../SourceSerifPro-Black.ttf",size);
    fontSize = size;
    return this->wrappedText = this->wrapText();

}

ofTexture* FontPlayer::getTexture(){
    fbo.getTexture().setAlphaMask(maskFbo.getTexture());
    return &fbo.getTexture();
};

void FontPlayer::play(){
    this->playing = true;
    this->animationCurrPos = 0;
    this->lastUpdateTime = 0;
    this->currentLetter = 0;
    updateCurrentLineCount();
};
void FontPlayer::stop(){
    this->playing = false;
    this->animationCurrPos = 0;
    this->lastUpdateTime = 0;
    this->currentLetter = 0;
    updateCurrentLineCount();
};
void FontPlayer::update(){
    
    float callTime = ofGetElapsedTimef();
    

    ofRectangle rect = this->font.getStringBoundingBox(wrappedText, 0, 0);
    
    // ALIGNMENT
    
    float y = 0;
    float x = 0;

    switch(this->xAlign){
        case FontPlayer::Alignment::START:
            x=0+this->margin; break;
        case FontPlayer::Alignment::CENTER:
            x=(this->fbo.getWidth()-rect.width)/2;break;
        case FontPlayer::Alignment::END:
            x=this->fbo.getWidth()-rect.width - this->margin;break;
    };
    switch(this->yAlign){
        case FontPlayer::Alignment::START:
            y=-1*rect.y+this->margin; break;
        case FontPlayer::Alignment::CENTER:
            y=(this->fbo.getHeight()-rect.height-rect.y)/2;break;
        case FontPlayer::Alignment::END:
            y=this->fbo.getHeight()-rect.height-rect.y - this->margin;break;
    }
    
    // move on with animation
    if(this->lastUpdateTime!=0){
        this->animationCurrPos += (callTime - this->lastUpdateTime);
        this->currentLetter =
        ofClamp(this->animationCurrPos  * FontPlayer::LETTERS_PER_S * this->animationSpeed,0,wrappedText.length()-1);
        updateCurrentLineCount();
    }
    
    // ALPHA MASK
    
    this->maskFbo.begin();
    ofClear(255,255,255,0);
    switch(animationType){
        case AnimationType::SLIDE:  this->slideAnimation(x,y);break;
        case AnimationType::WORDFADE:    this->wordFadeAnimation(x,y);break;
    }
    this->maskFbo.end();
    
    
    // DRAW TO FBO
    
    this->fbo.begin();
    ofClear(255,255,255,0);
    /*rect = this->font.getStringBoundingBox(wrappedText,x, y);
    ofSetColor(255,0,0); ofDrawRectangle(rect.x, rect.y, rect.width, rect.height);ofSetColor(color);*/
    this->font.drawString(wrappedText,x,y);
    this->fbo.end();
    
    this->lastUpdateTime = callTime;
    
};

void FontPlayer::slideAnimation(int x,int y){
    
    // Work line by line
    this->showCompletedLines(x,y);
    ofRectangle newLineBox = this->showCompletedLettersInCurrentLine(x,y);
    
    string currStr = currentLineText.substr(floor(this->currentLetter)-completedLineChars,1);
    if(currStr == " " || currStr == "\n"){
        return;
    }
    
    float extendedWidth = this->font.stringWidth(currentLineText.substr(0,floor(this->currentLetter)-completedLineChars+1));
    float extendedHeight = this->font.stringHeight(currentLineText);
    int fadeWidth = floor((extendedWidth-newLineBox.width)*(this->currentLetter-floor(this->currentLetter)));
    for(int i=0;i<fadeWidth;i++){
        ofSetColor(color,255-(ofClamp((((float)i/fadeWidth)-0.5)*2,0,1))*255);
        ofDrawRectangle(newLineBox.getRight()+i, newLineBox.getBottom() - extendedHeight, 1, extendedHeight);
    }
}

void FontPlayer::wordFadeAnimation(int x,int y){
    
    if(reverse) return wordFadeAnimationReverse(x, y);
    
    ofRectangle completedLinesBox = this->showCompletedLines(x,y);
    
    // get current word
    vector <string> words = ofSplitString(currentLineText," ");
    if(reverse) std::reverse(words.begin(),words.end());
    float linePos = this->currentLetter - completedLineChars;
    int currentWordI = 0;
    int completedWordsLength = 0;
    for(int i=0;i<words.size();i++){
        if(linePos>=completedWordsLength && linePos < completedWordsLength + words[i].length()+1){
            currentWordI = i;
            break;
        }else{
            completedWordsLength += words[i].length()+1;
        }
    }
    
    string currentWord = words[currentWordI];

    
    // draw completed words
    float completedWidth = this->font.stringWidth(currentLineText.substr(0,completedWordsLength));
    float lineHeight = this->font.stringHeight(currentLineText);

    ofSetColor(color,255);
    ofRectangle currentLineBox  = this->font.getStringBoundingBox(wrappedText.substr(0,completedLineChars+currentLineText.length()),x,y);
    ofDrawRectangle(currentLineBox.x, currentLineBox.y, completedWidth, currentLineBox.height);
    
    // calc current word's progress
    float wordProgress = ofClamp((linePos-completedWordsLength) / (currentWord.length()-1),0,1);
    wordProgress = 3*pow(wordProgress,2) - (2*pow(wordProgress,3));
    
    // fade in current word's block
    ofSetColor(color,255*wordProgress);
    //ofDrawRectangle(currentLineBox.getLeft()+completedWidth, currentLineBox.y, this->font.stringWidth(" "+currentWord), currentLineBox.height);
    ofDrawRectangle(currentLineBox.getLeft()+completedWidth, currentLineBox.getBottom()-lineHeight, this->font.stringWidth(" "+currentWord), lineHeight);
    
}

void FontPlayer::wordFadeAnimationReverse(int x,int y){
    
    ofRectangle completedLinesBox = this->showCompletedLines(x,y);
    
    // get current word
    vector <string> words = ofSplitString(currentLineText," ");
    if(reverse) std::reverse(words.begin(),words.end());
    float linePos = this->currentLetter - completedLineChars;
    int currentWordI = 0;
    int completedWordsLength = 0;
    for(int i=0;i<words.size();i++){
        if(linePos>=completedWordsLength && linePos < completedWordsLength + words[i].length()+1){
            currentWordI = i;
            break;
        }else{
            completedWordsLength += words[i].length()+1;
        }
    }
    
    string currentWord = words[currentWordI];
    
    // draw completed words
    float completedWidth;
    if(reverse){
        completedWidth = this->font.stringWidth(currentLineText.substr(currentLineText.length()-completedWordsLength,completedWordsLength));
    }{
        this->font.stringWidth(currentLineText.substr(0,completedWordsLength));
    };
    float lineHeight = this->font.stringHeight(currentLineText);
    
    ofSetColor(color,255);
    ofRectangle currentLineBox  = getTextBoxToCurrentLine(x, y, currentLineText.length());
    if(reverse){
        ofDrawRectangle(currentLineBox.getRight()-completedWidth, currentLineBox.y, completedWidth, lineHeight);
    }else{
        ofDrawRectangle(currentLineBox.x, currentLineBox.y, completedWidth, lineHeight);
    }
    
    // calc current word's progress
    float wordProgress = ofClamp((linePos-completedWordsLength) / (currentWord.length()-1),0,1);
    wordProgress = 3*pow(wordProgress,2) - (2*pow(wordProgress,3));
    
    // fade in current word's block
    ofSetColor(color,255*wordProgress);
    if(reverse){
        ofDrawRectangle(currentLineBox.getRight()-completedWidth-this->font.stringWidth(" "+currentWord), currentLineBox.y, this->font.stringWidth(" "+currentWord), lineHeight);
    }else{
        ofDrawRectangle(currentLineBox.getLeft()+completedWidth, currentLineBox.getBottom()-lineHeight, this->font.stringWidth(" "+currentWord), lineHeight);
    }
    
    
    
}

ofRectangle FontPlayer::showCompletedLines(int x,int y){
    
    // draw completed lines
    ofSetColor(color,255);
    ofRectangle completedLinesBox = getTextBoxToCurrentLine(x, y, 0);
    ofDrawRectangle(completedLinesBox.x, completedLinesBox.y, completedLinesBox.width, completedLinesBox.height);
    
    return completedLinesBox;
}

ofRectangle FontPlayer::getTextBoxToCurrentLine(int x, int y, int additionalChars){
    return getTextBoxToCurrentLine(x,y,additionalChars,wrappedText);
}
ofRectangle FontPlayer::getTextBoxToCurrentLine(int x, int y, int additionalChars,string txt){
    int end = completedLineChars+additionalChars;
    if(!reverse){
        return this->font.getStringBoundingBox(txt.substr(0,end),x,y);
    }else{
        ofRectangle fullBox = this->font.getStringBoundingBox(txt,x,y);
        ofRectangle completedLinesBox  = this->font.getStringBoundingBox(txt.substr(txt.length()-end,end),x,y);
        return ofRectangle(fullBox.x,fullBox.getBottom()-completedLinesBox.height,completedLinesBox.width, completedLinesBox.height);
    }
}


ofRectangle FontPlayer::showCompletedLettersInCurrentLine(int x,int y){
    // draw completed part of current line
    ofSetColor(color,255);

    ofRectangle currentLineBox  = this->font.getStringBoundingBox(wrappedText.substr(0,completedLineChars+currentLineText.length()),x,y);
    
    ofDrawRectangle(currentLineBox.x, currentLineBox.y, this->font.stringWidth(wrappedText.substr(completedLineChars,floor(this->currentLetter)-completedLineChars)), currentLineBox.height);
    
    return ofRectangle(currentLineBox.x, currentLineBox.y, this->font.stringWidth(wrappedText.substr(completedLineChars,floor(this->currentLetter)-completedLineChars)), currentLineBox.height);
}

void FontPlayer::updateCurrentLineCount(){
    vector <string> lines = ofSplitString(wrappedText,"\n");
    if(reverse) std::reverse(lines.begin(),lines.end());
    completedLineChars = 0;
    currentLine = 0;
    for(int l=0; l<lines.size(); l++){
        currentLine = l;
        currentLineText = lines[l];
        if(floor(this->currentLetter) < completedLineChars+currentLineText.length()+1){
            break;
        }
        completedLineChars += lines[l].length()+1;
    }
    currentLineText = lines[currentLine];
    
}


std::string FontPlayer::wrapText(){

    string typeWrapped = "";
    string tempString = "";
    vector <string> words = ofSplitString(text, " ");
    
    float width = this->getWidth() - 2*margin;

    
    for(int i=0; i<words.size(); i++) {
        
        string wrd = words[i];
        
        // if we aren't on the first word, add a space
        if (i > 0) {
            tempString += " ";
        }
        tempString += wrd;
        
        int stringwidth = this->font.stringWidth(tempString);
        
        if(stringwidth >= width) {
            typeWrapped += "\n";
            tempString = wrd;        // make sure we're including the extra word on the next line
        } else if (i > 0) {
            // if we aren't on the first word, add a space
            typeWrapped += " ";
        }
        
        typeWrapped += wrd;
    }
    
    if(this->font.stringHeight(typeWrapped) >= this->getHeight() && this->fontSize >= 8){
        return this->setFontSize((int) this->fontSize*0.8);
    }
        
        return typeWrapped;
    
}

void FontPlayer::nextFrame(){};

float FontPlayer::getWidth() const{ return this->fbo.getWidth();};
float FontPlayer::getHeight() const{ return this->fbo.getHeight();};

float FontPlayer::getPosition() const{ return this->animationCurrPos;};
float FontPlayer::setPosition(float pct){ this->animationCurrPos = pct;};
float FontPlayer::getDuration() const{ return this->text.length();};

bool  FontPlayer::isPlaying() const{ return this->playing;};
void  FontPlayer::setSpeed(float speed){ this->animationSpeed = speed;};
