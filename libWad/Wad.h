#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <stack>
#include <map>
#include <array>
#include <memory>
#include <fstream>

struct Descriptor{
    bool isFile;
    char name[9];
    uint32_t offset;
    uint32_t length;
    std::streampos currPos;
    std::vector<std::shared_ptr<Descriptor>> files;
    
    Descriptor();
    Descriptor(uint32_t offset, uint32_t length, char name[9], bool isFile);
};

struct Wad{
    char magic[5];
    uint32_t numOfDesciptors;
    std::string originalFile;
    uint32_t descriptorOffset;
    std::vector<std::shared_ptr<Descriptor>> wadInfo;
    std::vector<std::shared_ptr<Descriptor>> sudoStack;
    std::map<std::string, std::shared_ptr<Descriptor>> filePaths;

    static Wad* loadWad(const std::string &path);
    std::string getMagic();
    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);
    int getSize(const std::string &path);
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);

    private:
        Wad(const std::string &path);
};

void addToMap(std::vector<std::shared_ptr<Descriptor>> _stack, std::shared_ptr<Descriptor> _des, std::map<std::string, std::shared_ptr<Descriptor>> &pathMap);