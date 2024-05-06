#include "Wad.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <cctype>
#include <algorithm>

Descriptor::Descriptor(){
    offset = 0;
    length = 0;
    isFile = true;
}

Descriptor::Descriptor(uint32_t offset, uint32_t length, char name[9], bool isFile){
    this->offset = offset;
    this->length = length;
    this->isFile = isFile;
    isFile = true;
    std::strncpy(this->name, name, 8);
    this->name[8] = '\0';
}

Wad::Wad(const std::string &path){
    //read WAD header
    originalFile = path;
    std::fstream file;
    file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    file.read(magic, 4);
    magic[4] = '\0';
    file.read((char*)&numOfDesciptors, 4);
    file.read((char*)&descriptorOffset, 4);

    // std::cout << "Magic: " << magic << '\n'
    // << "Num. of Desc: " << numOfDesciptors << '\n'
    // << "Desc. Offset: " << descriptorOffset << '\n' << '\n';

    //jump to WAD descriptors
    file.seekg(descriptorOffset, std::ios::beg);
    for(int i = 0; i < numOfDesciptors; i++){
        std::shared_ptr<Descriptor> newDescriptor = std::make_shared<Descriptor>();
        newDescriptor->currPos = file.tellg();
        file.read((char*)&(newDescriptor->offset), 4);
        file.read((char*)&(newDescriptor->length), 4);
        file.read(newDescriptor->name, 8);
        (newDescriptor->name)[8] = '\0';
        wadInfo.push_back(newDescriptor);
        // std::cout << "Name: " << newDescriptor->name << '\n'
        // << "Desc. Offset: " << newDescriptor->offset << '\n'
        // << "Desc. Length: " << newDescriptor->length << '\n' << '\n';
    }

    file.close();

    // //root directory is '/'
    char rootName[9];
    char rootNameS[9];
    char rootNameE[9];
    rootName[0] = '/';
    rootName[1] = '\0';
    //make the strings to convert later
    std::string strRootNameS = rootName;
    strRootNameS += "_START";
    std::string strRootNameE = rootName;
    strRootNameE += "_END";
    //convert to char arrays
    std::copy(strRootNameS.begin(), strRootNameS.end(), rootNameS);
    rootNameS[strRootNameS.length()] = '\0';
    std::copy(strRootNameE.begin(), strRootNameE.end(), rootNameE);
    rootNameE[strRootNameE.length()] = '\0';
    //make descriptor pointers for each start, end, and regular descriptor
    std::shared_ptr<Descriptor> rootStart = std::make_shared<Descriptor>(0, 0, rootNameS, false);
    std::shared_ptr<Descriptor> rootDescriptor = std::make_shared<Descriptor>(0, 0, rootName, false);
    std::shared_ptr<Descriptor> rootEnd = std::make_shared<Descriptor>(0, 0, rootNameE, false);
    //give root it's file pointer as the end of the file
    // file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    // file.seekg(0, std::ios::end);
    // rootEnd->currPos = file.tellg();
    // file.close();
    //put start root at front of wadInfo and end root at end of wadInfo
    wadInfo.insert(wadInfo.begin(), rootStart);
    wadInfo.push_back(rootEnd);
    //add to stack and map
    //sudoStack.push_back(rootDescriptor);
    //filePaths["/"] = rootDescriptor;
    // root = rootDescriptor;
    // std::cout << "Name: " << rootDescriptor->name << '\n'
    //     << "Desc. Offset: " << rootDescriptor->offset << '\n'
    //     << "Desc. Length: " << rootDescriptor->length << '\n' << '\n';    

    //iterate through all WAD file information
    for(int i = 0; i < wadInfo.size(); ){
        //std::cout << wadInfo[i]->name << " ";
        bool mapMarker = false;
        bool endNamespace = false;
        bool namespaceMarker = false;

        //convert these char array to string
        std::string convert(wadInfo[i]->name);
        
        int underScoreIndex = 0;
        for(int i = 0; i < convert.length(); i++){
            if(convert[i] == '_'){
                underScoreIndex = i;
                break;
            }
        }

        std::string confirm = convert.substr(underScoreIndex);

        if(confirm == "_START"){
            namespaceMarker = true;
        }
        if (confirm == "_END"){
            endNamespace = true;
        }
        if (convert.length() == 4 && convert[0] == 'E' && convert[2] == 'M' && isdigit(convert[1]) && isdigit(convert[3])){
            mapMarker = true;
        }

        //namespace maker directory
        if(namespaceMarker){
            //std::cout << "Namespace directory" << '\n';
            //get rid of _START
            for(auto &car : wadInfo[i]->name){
                if(car == '_'){
                    car = '\0';
                    break;
                }
            }
            //make this directory the child of the current directory which is the top of the stack
            wadInfo[i]->isFile = false;
            if(sudoStack.empty()){
                sudoStack.push_back(wadInfo[i]);
            } else {
                sudoStack.back()->files.push_back(wadInfo[i]);
                sudoStack.push_back(wadInfo[i]);
            }
            addToMap(sudoStack, wadInfo[i], filePaths);
            i++;
            continue;
        }
        if(endNamespace){
            //std::cout << "End of Namespace directory" << '\n';
            sudoStack.pop_back();
            i++;
            continue;
        }
        //map maker directory
        if(mapMarker){
            //current directory is the map maker directory
            //std::cout << "Map maker" << '\n';
            wadInfo[i]->isFile = false;
            sudoStack.back()->files.push_back(wadInfo[i]);
            sudoStack.push_back(wadInfo[i]);
            addToMap(sudoStack, wadInfo[i], filePaths);
            for(int j = 1; j < 11; j++){
                //place the next 10 descriptors in this map maker
                sudoStack.back()->files.push_back(wadInfo[i + j]);
                sudoStack.push_back(wadInfo[i + j]);
                addToMap(sudoStack, wadInfo[i + j], filePaths);
                sudoStack.pop_back();
                // std::cout << wadInfo[i + j]->name << " ";
                // std::cout << "Regular file" << '\n';
            }
            //jump past the next 10 descriptors
            sudoStack.pop_back();
            i += 11;
            continue;
        }  
        //any other files
        //std::cout << "Regular file" << '\n';
        //add the file to the directory
        sudoStack.back()->files.push_back(wadInfo[i]);
        //add directory to the stack to get the current file's path
        sudoStack.push_back(wadInfo[i]);
        //add path to map
        addToMap(sudoStack, wadInfo[i], filePaths);
        //remove current file from path since files can't have files within them, only direcoties can
        sudoStack.pop_back();
        i++;
    }    
}

Wad* Wad::loadWad(const std::string &path){
    return new Wad(path);
}

std::string Wad::getMagic(){
    return magic;
}

bool Wad::isContent(const std::string &path){
    if(path.empty()){
        return false;
    }
    std::string pathWithNoSlash = path;
    //get rid of last slash if applicable
    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }

    //if path exist in map and it's corresponding descriptor is a file return true
    if(filePaths.count(pathWithNoSlash) > 0){
        if(filePaths[pathWithNoSlash]->isFile){
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool Wad::isDirectory(const std::string &path){
   if(path.empty()){
        return false;
    }
    std::string pathWithNoSlash = path;
    //get rid of last slash if applicable
    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }

    //if path exist in map and it's corresponding descriptor is a file return true
    if(filePaths.count(pathWithNoSlash) > 0){
        if(!filePaths[pathWithNoSlash]->isFile){
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

int Wad::getSize(const std::string &path){
    if(!isContent(path)){
        return -1;
    }
    std::string pathWithNoSlash = path;
    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }
    return filePaths[pathWithNoSlash]->length;
}

int Wad::getContents(const std::string &path, char *buffer, int length, int offset){
    if(!isContent(path)){
        return -1;
    }
    std::string pathWithNoSlash = path;
    //get rid of last slash if applicable
    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }
    
    if(offset >= filePaths[pathWithNoSlash]->length){
        return 0;
    }
    
    if(offset + length > filePaths[pathWithNoSlash]->length){
        length = filePaths[pathWithNoSlash]->length - offset;
    }

    //open file
    std::fstream file;
    file.open(originalFile, std::ios::in | std::ios::out | std::ios::binary);
    //jump to lump data along with offset
    file.seekg(filePaths[pathWithNoSlash]->offset + offset, std::ios::beg);
    file.read(buffer, length);
    file.close();   
    
    return length;
}

int Wad::getDirectory(const std::string &path, std::vector<std::string> *directory){
    if(!isDirectory(path)){
        return -1;
    }
    std::string pathWithNoSlash = path;
    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }
    //Descriptor* respectiveDes = filePaths[pathWithNoSlash];
    std::shared_ptr<Descriptor> respectiveDes = filePaths[pathWithNoSlash];
    if(!respectiveDes->files.empty()){
        for(const auto file : respectiveDes->files){
            directory->push_back(file->name);
        }
    }
    return directory->size();
}

void Wad::createDirectory(const std::string &path){
    //if path already exist
    if(isDirectory(path)){
        return;
    }
    std::string pathWithNoSlash = path;
    std::string pathParent;
    std::string newDirectory;
    std::string checkForMapMake;

    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }    

    //get directory of parent path of new directory
    for(int i = pathWithNoSlash.length() - 1; i >= 0; i--){
        if(pathWithNoSlash[i] == '/'){
            pathParent = pathWithNoSlash.substr(0, i + 1);
            newDirectory = pathWithNoSlash.substr(i + 1);
            break;
        }
    }
    //a directory name should be either one or two characters only
    if(newDirectory.length() > 2){
        return;
    }

    if(pathParent != "/" && pathParent[pathParent.length() - 1] == '/'){
        pathParent.pop_back();
    }  

    //leave if parent directory doesn't exist
    if(!isDirectory(pathParent)){
        return;
    }

    for(int i = pathParent.length() - 1; i >= 0; i--){
        if(pathParent[i] == '/'){
            checkForMapMake = pathParent.substr(i + 1);
            break;
        }
    }

    //Map directories cannot have other directories in them
    if(checkForMapMake.length() == 4 && checkForMapMake[0] == 'E' && checkForMapMake[2] == 'M' && isdigit(checkForMapMake[1]) && isdigit(checkForMapMake[3])){
        return;
    }

    std::string dStart = newDirectory + "_START";
    std::string dEnd = newDirectory + "_END";
    
    //convert string to char array to make new descriptor
    char new_d[9];
    char new_dStart[9];
    char new_dEnd[9];

    std::copy(newDirectory.begin(), newDirectory.end(), new_d);
    new_d[newDirectory.length()] = '\0';

    std::copy(dStart.begin(), dStart.end(), new_dStart);
    new_dStart[dStart.length()] = '\0';

    std::copy(dEnd.begin(), dEnd.end(), new_dEnd);
    new_dEnd[dEnd.length()] = '\0';

    std::shared_ptr<Descriptor> parentPath = filePaths[pathParent];
    std::shared_ptr<Descriptor> newDirectoryDescriptor = std::make_shared<Descriptor>(0, 0, new_d, false);

    //update the tree
    parentPath->files.push_back(newDirectoryDescriptor);

    //add new descriptor to map
    filePaths[pathWithNoSlash] = newDirectoryDescriptor;

    //make the two descriptors for start and end of directory
    std::shared_ptr<Descriptor> startDes = std::make_shared<Descriptor>(0, 0, new_dStart, false);
    std::shared_ptr<Descriptor> endDes = std::make_shared<Descriptor>(0, 0, new_dEnd, false);

    //open file and update number of descriptors
    std::fstream file;
    file.open(originalFile, std::ios::in | std::ios::out | std::ios::binary);
    file.seekp(4, std::ios::beg);
    numOfDesciptors += 2;
    file.write((char*)&numOfDesciptors, 4);
    file.flush();

    uint64_t zeros = 0;

    int k = 0;
    //ignore root directory and directories belonging to the root
    if(pathWithNoSlash.length() > 3){
        std::vector<std::string> rawPath;
        std::istringstream split(pathWithNoSlash);
        std::string dir;
        //split up the directories and store them
        while(std::getline(split, dir, '/')){
            if(!dir.empty()){
                rawPath.push_back(dir);
            }
        }

        for(int a = 0; a < wadInfo.size(); a++){
            for(int b = 0; b < rawPath.size(); b++){
                std::string dirWithStart = rawPath[b] + "_START";
                if(wadInfo[a]->name == dirWithStart || wadInfo[a]->name == rawPath[b]){
                    //update k by continuing to move it closer to where the directory will be in order to avoid placing directories in directories with the same name
                    k += a - k;
                }
            }
        }
    }

    int j = 0;
    //append the _END to parent name 
    std::string addEnd = parentPath->name;
    addEnd += "_END";
    char charAddEnd[9];
    std::copy(addEnd.begin(), addEnd.end(), charAddEnd);
    charAddEnd[addEnd.length()] = '\0';
    //find the parents end descriptor, this is where we want to add the new descriptors
    //std::shared_ptr<Descriptor> parentPathEnd;
    for(int i = k; i < wadInfo.size(); i++){
        if(strcmp(wadInfo[i]->name, charAddEnd) == 0){
            //std::cout << wadInfo[i]->name << " ";
            //parentPathEnd = wadInfo[i];
            j = i;
            break;
        }
    }

    //add to wadInfo
    auto it = wadInfo.begin() + j;
    wadInfo.insert(it, startDes);
    it = wadInfo.begin() + j + 1; 
    wadInfo.insert(it, endDes);

    for(int i = 1; i < wadInfo.size() - 1; i++){
        wadInfo[i + 1]->currPos = wadInfo[i]->currPos + (long int)16;
    }

    for(int i = j; i < wadInfo.size() - 1; i++){
        file.seekp(wadInfo[i]->currPos);

        file.write((char*)&zeros, 8);
        file.flush();

        file.write(wadInfo[i]->name, 8);
        file.flush();
    }

    file.close();
    return;
}

void Wad::createFile(const std::string &path){
        //if path already exist
    if(isContent(path)){
        return;
    }
    std::string pathWithNoSlash = path;
    std::string pathParent;
    std::string newFile;
    std::string checkForMapMake;

    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }    

    //get directory of parent path of new directory
    for(int i = pathWithNoSlash.length() - 1; i >= 0; i--){
        if(pathWithNoSlash[i] == '/'){
            pathParent = pathWithNoSlash.substr(0, i + 1);
            newFile = pathWithNoSlash.substr(i + 1);
            break;
        }
    }
    //a directory name should be either one or two characters only
    if(newFile.length() > 8){
        return;
    }

    if(pathParent != "/" && pathParent[pathParent.length() - 1] == '/'){
        pathParent.pop_back();
    }  

    //leave if parent directory doesn't exist
    if(!isDirectory(pathParent)){
        return;
    }

    for(int i = pathParent.length() - 1; i >= 0; i--){
        if(pathParent[i] == '/'){
            checkForMapMake = pathParent.substr(i + 1);
            break;
        }
    }

    //Map directories cannot have other directories in them or file since they will always have 10 files in them
    if(checkForMapMake.length() == 4 && checkForMapMake[0] == 'E' && checkForMapMake[2] == 'M' && isdigit(checkForMapMake[1]) && isdigit(checkForMapMake[3])){
        return;
    }
    std::string checkEMDir = pathWithNoSlash.substr(1);

    if(checkEMDir.length() == 4 && checkEMDir[0] == 'E' && checkEMDir[2] == 'M' && isdigit(checkEMDir[1]) && isdigit(checkEMDir[3])){
        return;
    }
    
    //convert string to char array to make new descriptor
    char newFileChar[9];

    std::copy(newFile.begin(), newFile.end(), newFileChar);
    newFileChar[newFile.length()] = '\0';


    std::shared_ptr<Descriptor> parentPath = filePaths[pathParent];
    std::shared_ptr<Descriptor> newFileDescriptor = std::make_shared<Descriptor>(0, 0, newFileChar, true);

    //update the tree
    parentPath->files.push_back(newFileDescriptor);

    //add new descriptor to map
    filePaths[pathWithNoSlash] = newFileDescriptor;


    //open file and update number of descriptors
    std::fstream file;
    file.open(originalFile, std::ios::in | std::ios::out | std::ios::binary);
    file.seekp(4, std::ios::beg);
    numOfDesciptors += 1;
    file.write((char*)&numOfDesciptors, 4);
    file.flush();

    uint64_t zeros = 0;

    int k = 0;
    //ignore root directory and directories belonging to the root
    if(pathWithNoSlash.length() > 3){
        std::vector<std::string> rawPath;
        std::istringstream split(pathWithNoSlash);
        std::string dir;
        //split up the directories and store them
        while(std::getline(split, dir, '/')){
            if(!dir.empty()){
                rawPath.push_back(dir);
            }
        }
        //eliminate the file being added since it does yet exist in wadInfo, it will not be found
        rawPath.pop_back();

        for(int a = 0; a < wadInfo.size(); a++){
            for(int b = 0; b < rawPath.size(); b++){
                std::string dirWithStart = rawPath[b] + "_START";
                if(wadInfo[a]->name == dirWithStart || wadInfo[a]->name == rawPath[b]){
                    //update k by continuing to move it closer to where the file will be in order to avoid placing files in directories with the same name
                    k += a - k;
                }
            }
        }
    }

    int j = 0;
    //append the _END to parent name 
    std::string findPar = parentPath->name;
    findPar += "_END";
    char charFindPar[9];
    std::copy(findPar.begin(), findPar.end(), charFindPar);
    charFindPar[findPar.length()] = '\0';
    //find the parents end descriptor, this is where we want to add the new descriptors
    for(int i = k; i < wadInfo.size(); i++){
        if(strcmp(wadInfo[i]->name, charFindPar) == 0){
            //std::cout << wadInfo[i]->name << " ";
            j = i;
            break;
        }
    }

    //add to wadInfo
    auto it = wadInfo.begin() + j;
    wadInfo.insert(it, newFileDescriptor);

    for(int i = 1; i < wadInfo.size() - 1; i++){
        wadInfo[i + 1]->currPos = wadInfo[i]->currPos + (long int)16;
    }

    for(int i = j; i < wadInfo.size() - 1; i++){
        file.seekp(wadInfo[i]->currPos);

        file.write((char*)&zeros, 8);
        file.flush();

        file.write(wadInfo[i]->name, 8);
        file.flush();
    }

    file.close();
    return;
}

int Wad::writeToFile(const std::string &path, const char *buffer, int length, int offset){
    //path should not be a directory or content path should not already exist
    if(isDirectory(path) || !isContent(path)){
        return -1;
    }

    std::string pathWithNoSlash = path;

    if(pathWithNoSlash != "/" && pathWithNoSlash[pathWithNoSlash.length() - 1] == '/'){
        pathWithNoSlash.pop_back();
    }    
    //make sure the file written to is empty
    std::shared_ptr<Descriptor> checkEmpty = filePaths[pathWithNoSlash];
    if(checkEmpty->length != 0){
        return 0;
    }

    checkEmpty->length = length;
    checkEmpty->offset = offset + 16;

    uint32_t newLength = length;
    uint32_t newOffset = offset + 16;

    std::fstream file;
    file.open(originalFile, std::ios::in | std::ios::out | std::ios::binary);

    //update length and offset
    file.seekp(checkEmpty->currPos);
    file.write((char*)&newOffset, 4);
    file.write((char*)&newLength, 4);

    //jump to descriptor offset
    file.seekg(descriptorOffset, std::ios::beg);
    //save descriptor data
    std::vector<char> remainder(std::istreambuf_iterator<char>(file), {});

    //jump to lump data with offset
    file.seekp(offset + 16, std::ios::beg);

    //write to lump data
    file.write(buffer, length);
    file.flush();

    //write back descriptor information
    file.write(remainder.data(), remainder.size());
    file.flush();

    file.close();

    return length;
}

void addToMap(std::vector<std::shared_ptr<Descriptor>> _stack, std::shared_ptr<Descriptor> _des, std::map<std::string, std::shared_ptr<Descriptor>> &pathMap){
    std::string newPath;
    for(int k = 0; k < _stack.size(); k++){
        newPath += _stack[k]->name;
        if(k > 0){
            newPath += "/";
        }
    }
    if(newPath != "/"){
        newPath.pop_back();
    }
    pathMap[newPath] = _des;
}