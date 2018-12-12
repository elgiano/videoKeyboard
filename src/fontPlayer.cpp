//
//  fontPlayer.cpp
//  fontTest
//
//  Created by Gnlc Elia on 24/11/2018.
//

#include "fontPlayer.hpp"

bool isPrime(long int n)
{
    if (n == 1)
    {
        return false;
    }
    int i = 2;
    while (i*i <= n)
    {
        if (n % i == 0)
        {
            return false;
        }
        i += 1;
    }
    return true;
}


// LOADING

bool FontPlayer::load(std::string text,int size){

    if(!this->fbo.isAllocated()){
        this->fbo.allocate(ofGetScreenWidth(),ofGetScreenHeight(),GL_RGBA);
        this->maskFbo.allocate(ofGetScreenWidth(),ofGetScreenHeight(),GL_RGBA);
        clearFbos();
    }

    this->text = parseText(text);
    this->parseTargetWords();
    this->setFontSize(size);
    
    makeConstellation();



    return true;
};

bool FontPlayer::load(std::string text){
    this->load(text,fontSize);
};


std::string FontPlayer::parseText(std::string text){
    vector <string> lines = ofSplitString(text,"\n");
    vector <string> txtLines;

    for(auto &line: lines){
        if(line.front() == '#'){
            line.erase(line.begin(),line.begin()+1);
            vector <string> values = ofSplitString(line,",");
            for(int i=0;i<values.size();i++){
                float value = std::stof(values[i]);
                switch(i){
                    case 0:
                        if(value<0){
                            this->margin =-value*getWidth();
                            this->xAlign = FontPlayer::Alignment::END;
                        }else{
                            this->margin =value*getWidth();
                            this->xAlign = FontPlayer::Alignment::START;
                        };
                        break;
                    case 1:
                        if(value<0){
                            this->marginY =-value*getHeight();
                            this->yAlign = FontPlayer::Alignment::END;
                        }else{
                            this->marginY =value*getHeight();
                            this->yAlign = FontPlayer::Alignment::START;
                        };
                        break;
                    case 2:
                        this->widthRatio = value;
                        break;
                    case 3:
                        this->heightRatio = value;
                        break;
                    case 4:
                        this->animationType = static_cast<FontPlayer::AnimationType>(floor(ofClamp(value,0,2)));
                        break;
                    case 5:
                        this->spreadMode = static_cast<FontPlayer::SpreadMode>(floor(ofClamp(value,0,1)));
                        break;
                    case 6:
                        if(value<0){
                            this->lettersPerSecond =-value;
                            this->reverse = true;
                        }else{
                            this->lettersPerSecond = value;
                            this->reverse = false;
                        };
                        break;
                    

                };

            }
        }else{
            txtLines.push_back(line);
        }
    }

    std::string joined = ofJoinString(txtLines,"\n");

    return joined;

};

void FontPlayer::parseTargetWords(){

    targetWords.clear();
    words = getWords(text);
    for(int i =0;i<words.size();i++){
        if(words[i].front() == '$'){
            targetWords.push_back(i);
        }
    }
    for(auto &wordI: targetWords){
        size_t start_pos = text.find(words[wordI]);
        std::string to = words[wordI];
        to.erase(0,1);
        if(start_pos == std::string::npos) continue;
        text.replace(start_pos, words[wordI].length(), to);
    }

}

std::string FontPlayer::setFontSize(int size){
    this->font.load("../NotoSans-SemiCondensed.ttf",size);
    fontSize = size;
    return this->wrappedText = this->wrapText();
}

std::string FontPlayer::setFontScale(float scale){
    this->font.load("../NotoSans-SemiCondensed.ttf",fontSize*scale);
    fontScale = scale;
    return this->wrappedText = this->wrapText();
}


// ANIMATION


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
            y=-1*rect.y+this->marginY; break;
        case FontPlayer::Alignment::CENTER:
            y=(this->fbo.getHeight()-rect.height-rect.y)/2;break;
        case FontPlayer::Alignment::END:
            y=this->fbo.getHeight()-rect.height-rect.y - this->marginY;break;
    }

    // move on with animation
    if(this->lastUpdateTime!=0){
        this->animationCurrPos += (callTime - this->lastUpdateTime);
        this->currentLetter =
        ofClamp(this->animationCurrPos  * lettersPerSecond * this->animationSpeed,0,wrappedText.length()-1);
        updateCurrentLineCount();
    }

    // ALPHA MASK

    this->maskFbo.begin();
    ofClear(255,255,255,0);
    if(spreadMode==SpreadMode::TOGETHER){
        switch(animationType){
            case AnimationType::SLIDE:  this->slideAnimation(x,y);break;
            case AnimationType::WORDFADE:    this->wordFadeAnimation(x,y);break;
            case AnimationType::TARGETWORD:    this->targetWordAnimation(x,y);break;
        }
    }else{
        switch(animationType){
            case AnimationType::SLIDE:  this->drawConstellationMask();break;
            case AnimationType::WORDFADE:    this->wordFadeAnimationConstellation();break;
            case AnimationType::TARGETWORD:    this->targetWordAnimation(x,y);break;
        }
        //ofDrawRectangle(0, 0, getWidth(), getHeight());
    }
    this->maskFbo.end();


    // DRAW TO FBO

    this->fbo.begin();
    ofClear(255,255,255,0);
    /*rect = this->font.getStringBoundingBox(wrappedText,x, y);
    ofSetColor(255,0,0); ofDrawRectangle(rect.x, rect.y, rect.width, rect.height);ofSetColor(color);*/
    if(spreadMode==SpreadMode::TOGETHER){
        this->font.drawString(wrappedText,x,y);
    }else if(spreadMode==SpreadMode::SPREAD){
        this->drawConstellation();
        //this->drawConstellationMask();
    }
    this->fbo.end();

    this->lastUpdateTime = callTime;

};

void FontPlayer::slideAnimation(int x,int y){
    ofEnableAlphaBlending();
    
    
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
        ofSetColor(color,255-((float)i/fadeWidth*255));
        ofFill();
        ofDrawRectangle(newLineBox.getRight()+i, newLineBox.getBottom() - extendedHeight, 1, extendedHeight);
    }
}

void FontPlayer::wordFadeAnimation(int x,int y){

    if(reverse) return wordFadeAnimationReverse(x, y);

    this->showCompletedLines(x,y);

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
    //wordProgress = 3*pow(wordProgress,2) - (2*pow(wordProgress,3));

    // fade in current word's block
    ofSetColor(color,255*wordProgress);
    ofFill();
    //ofDrawRectangle(currentLineBox.getLeft()+completedWidth, currentLineBox.y, this->font.stringWidth(" "+currentWord), currentLineBox.height);
    ofDrawRectangle(currentLineBox.getLeft()+completedWidth, currentLineBox.getBottom()-lineHeight, this->font.stringWidth(" "+currentWord), lineHeight);

}

void FontPlayer::wordFadeAnimationReverse(int x,int y){

    this->showCompletedLines(x,y);

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
        completedWidth = this->font.stringWidth(currentLineText.substr(0,completedWordsLength));
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

void FontPlayer::wordFadeAnimationConstellation(){
    
    int currentWordI = this->showCompletedWords();
    
    // calc current word's progress
    int completedWordsLength = 0;
    if(currentWordI>0) completedWordsLength = std::accumulate(words.begin(), words.begin()+currentWordI-1, 0,[](int a, std::string b) {
        return std::move(a) + b.length()+1;
    });
    float wordProgress = ofClamp((currentLetter-completedWordsLength) / (words[currentWordI].length()-1),0,1);
    //wordProgress = 3*pow(wordProgress,2) - (2*pow(wordProgress,3));
    
    // fade in current word's block
    ofSetColor(color,255*wordProgress);
    ofFill();
    drawConstellationWord(currentWordI);

    
}

void FontPlayer::targetWordAnimation(int x, int y){

    ofEnableAlphaBlending();
    float turningPoint = 100.0f/lettersPerSecond/animationSpeed;
    float delay = 0;//turningPoint;
    float endPoint = 4*turningPoint;
    // FIRST FADE IN TARGET WORDS

    float progress=1.0;
    if(animationCurrPos <= turningPoint){
        progress = (animationCurrPos)/(turningPoint);
        //progress = (2*pow(progress,3)-3*pow(progress,2));
        //std::cout << 255*progress << std::endl;
    };
    for(auto &wordIndex: targetWords){
        ofRectangle rect =  getWordBoundingBox(wordIndex,x,y);
        if(spreadMode == SpreadMode::SPREAD){
            rect = constellation[wordIndex];
        }
        ofSetColor(color,progress*255);
        ofFill();
        ofDrawRectangle(rect.x, rect.y, rect.width, rect.height);
    }
    if(animationCurrPos > turningPoint+delay){
        // THEN FADE IN THE REST
        float progress = (animationCurrPos - (turningPoint+delay))/(endPoint-(turningPoint+delay));
        ofSetColor(color,progress*255);
        ofRectangle rect = this->font.getStringBoundingBox(wrappedText,x, y);
        ofDrawRectangle(rect.x, rect.y, rect.width, rect.height);
    }
}


// UTILS

void FontPlayer::showCompletedLines(int x,int y){

    // draw completed lines
    ofSetColor(color,255);
    ofRectangle completedLinesBox = getTextBoxToCurrentLine(x, y, 0);
    ofDrawRectangle(completedLinesBox.x, completedLinesBox.y, completedLinesBox.width, completedLinesBox.height);
}

int FontPlayer::showCompletedWords(){
    int i = 0;
    int l = 0;
    while(l<currentLetter && i<words.size()){
        drawConstellationWord(i++);
        l+=words[i].length();
    }
    return ofClamp(i,0,words.size()-1);
    
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


ofRectangle FontPlayer::getWordBoundingBox(int wordIndex, int x, int y){

    int lineIndex = 0;
    int charIndex = 0;
    int wordI = 0;
    int wordInLineIndex = 0;
    while(wordI<wordIndex && lineIndex < lines.size()){
        std::string line =lines[lineIndex];
        std::vector<std::string> lineWords = ofSplitString(lines[lineIndex], " ",true);
        if(wordI+lineWords.size()>=wordIndex){
            wordInLineIndex = wordIndex-wordI; // num of preceding words
            wordI += wordInLineIndex;
            wordInLineIndex = std::accumulate(lineWords.begin(),lineWords.begin()+wordInLineIndex,0,[](int a, std::string b) {
                return std::move(a) + b.length()+1;
            }); // num of precedingChars
            break;
        }
        wordI += lineWords.size();
        charIndex += lines[lineIndex].size()+1;
        lineIndex++;
    }

    charIndex += wordInLineIndex;

    this->currentLetter = charIndex;
    updateCurrentLineCount();
    std::string currentPartInLine = currentLineText.substr(0,charIndex-completedLineChars);
    // draw completed words
    float completedWidth = this->font.stringWidth(currentLineText.substr(0,charIndex-completedLineChars));
    float lineHeight = this->font.stringHeight(currentLineText);

    ofSetColor(color,255);
    ofRectangle currentLineBox  = getTextBoxToCurrentLine(x, y, currentLineText.length());


    return ofRectangle(currentLineBox.getLeft()+completedWidth, currentLineBox.getBottom()-lineHeight, this->font.stringWidth(" "+words[wordIndex])+2, lineHeight);

}

// MARK: constellation

void FontPlayer::makeConstellation(){
    constellation.clear();
    
    int n = words.size();
    if(isPrime(n)) n++;
    int columns;
    for (columns = (int)ceil(sqrt(n)); columns > 1; --columns) {
        if (n % columns == 0) break;
    }
    int rows = n / columns;
    
    int i = 0,j=0;
    int cellW = getWidth()/columns;
    int cellH = getHeight()/rows;
    for(auto &word: words){
        int h = font.stringHeight(word);
        int w = font.stringWidth(word);
        int x = cellW*i + (rand()%abs(cellW-w));
        int y = cellH*j + (rand()%abs(cellH-h));
        constellation.push_back(font.getStringBoundingBox(word+" ", x,y));
        if(++i>=columns){i=0;j++;}
    }
}

void FontPlayer::drawConstellation(){
    for(int i=0;i<words.size();i++){
        font.drawString(words[i],constellation[i].x,constellation[i].y);
    }
}
void FontPlayer::drawConstellationMask(){
    
    /*int n = words.size();
    if(isPrime(n)) n++;
    int columns;
    for (columns = (int)ceil(sqrt(n)); columns > 1; --columns) {
        if (n % columns == 0) break;
    }
    int rows = n / columns;
    
    int i = 0,j=0;
    int cellW = getWidth()/columns;
    int cellH = getHeight()/rows;
    for(auto &word: words){
        ofSetColor(rand()%255,rand()%255,rand()%255,200);
        ofDrawRectangle(cellW*i, cellH*j, cellW, cellH);
        if(++i>=columns){i=0;j++;}
    }*/
    
    for(int i =0; i< constellation.size(); i++){
            ofSetColor(color,255);
            drawConstellationWord(i);
    }
    

}

void FontPlayer::drawConstellationWord(int i){
    ofRectangle rect = constellation[i];
    ofDrawRectangle(rect.x, rect.y-rect.height, rect.width, rect.height-font.getDescenderHeight());
}

std::vector<std::string> FontPlayer::getWords(std::string text){
    std::vector<std::string> splitWords;
    std::string line;
    std::string word;
    std::istringstream textInput;
    textInput.str(text);
    while(std::getline(textInput,line,'\n')){
        std::istringstream lineInput;
        lineInput.str(line);
        while(std::getline(lineInput,word,' ')){
            splitWords.push_back(word);
        }
    }

    return splitWords;
}


std::string FontPlayer::wrapText(){

    string typeWrapped = "";
    string tempString = "";
    words = ofSplitString(text, " ");



    float marginCorrection = margin;
    if(this->xAlign == FontPlayer::Alignment::CENTER){
        marginCorrection *= 2;
    };

    float width = this->getWidth()*this->widthRatio - marginCorrection;


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

    if(autoResize){
        if(this->font.stringHeight(typeWrapped) >= this->getHeight()*this->heightRatio && this->fontSize >= 8){
                    return this->setFontSize((int) this->fontSize*0.8);
        }
    }

    lines = ofSplitString(typeWrapped,"\n");
    words = this->getWords(text);

    return typeWrapped;

}

void FontPlayer::clearFbos(){
    this->maskFbo.begin();
    ofClear(255,255,255,0);
    this->maskFbo.end();
    this->fbo.begin();
    ofClear(255,255,255,0);
    this->fbo.end();
}

// OTHER

ofTexture* FontPlayer::getTexture(){
    fbo.getTexture().setAlphaMask(maskFbo.getTexture());
    return &fbo.getTexture();
};

void FontPlayer::play(){
    this->animationCurrPos = 0;
    this->lastUpdateTime = 0;
    this->currentLetter = 0;
    updateCurrentLineCount();
    clearFbos();
    this->playing = true;
};
void FontPlayer::stop(){
    this->playing = false;
    this->animationCurrPos = 0;
    this->lastUpdateTime = 0;
    this->currentLetter = 0;
    clearFbos();
    updateCurrentLineCount();
};

void FontPlayer::nextFrame(){};

float FontPlayer::getWidth() const{ return this->fbo.getWidth();};
float FontPlayer::getHeight() const{ return this->fbo.getHeight();};

float FontPlayer::getPosition() const{ return this->animationCurrPos;};
float FontPlayer::setPosition(float pct){ this->animationCurrPos = pct;};
float FontPlayer::getDuration() const{ return this->text.length();};

bool  FontPlayer::isPlaying() const{ return this->playing;};
void  FontPlayer::setSpeed(float speed){ this->animationSpeed = speed;};
