#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <memory>
#include <bitset>
#include <map>

// Define constants for file modes
constexpr int O_RDONLY = std::ios::in;
constexpr int O_WRONLY = std::ios::out;
constexpr int O_CREAT = std::ios::out | std::ios::trunc;
constexpr int S_IRUSR = 0;
constexpr int S_IWUSR = 0;

struct Node {
    char character;
    int freq;
    std::shared_ptr<Node> left, right;

    Node(char character, int freq)
        : character(character), freq(freq), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(const std::shared_ptr<Node>& l, const std::shared_ptr<Node>& r) {
        return l->freq > r->freq;
    }
};

using MinHeap = std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, Compare>;

MinHeap createAndBuildMinHeap(const std::vector<char>& arr, const std::vector<int>& freq) {
    MinHeap minHeap;
    for (size_t i = 0; i < arr.size(); ++i) {
        minHeap.push(std::make_shared<Node>(arr[i], freq[i]));
    }
    return minHeap;
}

std::shared_ptr<Node> buildHuffmanTree(const std::vector<char>& arr, const std::vector<int>& freq) {
    MinHeap minHeap = createAndBuildMinHeap(arr, freq);

    while (minHeap.size() != 1) {
        auto left = minHeap.top();
        minHeap.pop();
        auto right = minHeap.top();
        minHeap.pop();

        auto top = std::make_shared<Node>('$', left->freq + right->freq);
        top->left = left;
        top->right = right;

        minHeap.push(top);
    }

    return minHeap.top();
}

bool isLeaf(const std::shared_ptr<Node>& root) {
    return !root->left && !root->right;
}

void printCodesIntoFile(std::ofstream& outFile, const std::shared_ptr<Node>& root, std::vector<int>& code, int top, std::map<char, std::vector<int>>& huffmanCode) {
    if (root->left) {
        code[top] = 0;
        printCodesIntoFile(outFile, root->left, code, top + 1, huffmanCode);
    }

    if (root->right) {
        code[top] = 1;
        printCodesIntoFile(outFile, root->right, code, top + 1, huffmanCode);
    }

    if (isLeaf(root)) {
        huffmanCode[root->character] = std::vector<int>(code.begin(), code.begin() + top);
        outFile << root->character;
        outFile.write(reinterpret_cast<const char*>(&top), sizeof(top));
        int dec = std::bitset<32>(std::string(code.begin(), code.begin() + top).c_str()).to_ulong();
        outFile.write(reinterpret_cast<const char*>(&dec), sizeof(dec));
    }
}

void compressFile(std::ifstream& inFile, std::ofstream& outFile, const std::map<char, std::vector<int>>& huffmanCode) {
    char n;
    unsigned char a = 0;
    int h = 0;

    while (inFile.get(n)) {
        const auto& code = huffmanCode.at(n);
        for (int bit : code) {
            if (h < 7) {
                a = (a << 1) | bit;
                h++;
            } else {
                a = (a << 1) | bit;
                outFile.put(a);
                a = 0;
                h = 0;
            }
        }
    }

    for (int i = 0; i < 7 - h; i++) {
        a = a << 1;
    }
    outFile.put(a);
}

struct Tree {
    char g;
    int len;
    int dec;
    std::shared_ptr<Tree> left, right;

    Tree() : g('\0'), len(0), dec(0), left(nullptr), right(nullptr) {}
};

void extractCodesFromFile(std::ifstream& inFile, Tree& t) {
    inFile.get(t.g);
    inFile.read(reinterpret_cast<char*>(&t.len), sizeof(t.len));
    inFile.read(reinterpret_cast<char*>(&t.dec), sizeof(t.dec));
}

void rebuildHuffmanTree(std::ifstream& inFile, int size, std::shared_ptr<Tree>& tree) {
    tree = std::make_shared<Tree>();
    for (int k = 0; k < size; k++) {
        auto temp = tree;
        Tree t;
        extractCodesFromFile(inFile, t);

        std::bitset<32> bin(t.dec);
        std::string bin_str = bin.to_string().substr(32 - t.len);

        for (char bit : bin_str) {
            if (bit == '0') {
                if (!temp->left) {
                    temp->left = std::make_shared<Tree>();
                }
                temp = temp->left;
            } else {
                if (!temp->right) {
                    temp->right = std::make_shared<Tree>();
                }
                temp = temp->right;
            }
        }
        temp->g = t.g;
        temp->len = t.len;
        temp->dec = t.dec;
    }
}

bool isRoot(const std::shared_ptr<Tree>& node) {
    return !node->left && !node->right;
}

void decompressFile(std::ifstream& inFile, std::ofstream& outFile, std::shared_ptr<Tree>& tree, int fileLength) {
    unsigned char p;
    std::bitset<8> inp;
    auto temp = tree;
    int k = 0;

    while (inFile.read(reinterpret_cast<char*>(&p), sizeof(p))) {
        inp = std::bitset<8>(p);
        for (int i = 0; i < 8 && k < fileLength; ++i) {
            if (!isRoot(temp)) {
                temp = inp[i] == 0 ? temp->left : temp->right;
            } else {
                outFile.put(temp->g);
                temp = tree;
                k++;
                i--;
            }
        }
    }
}

int main() {
    std::ifstream inFile("sample.txt", std::ios::binary);
    if (!inFile) {
        std::cerr << "Open Failed For Input File:\n";
        return 1;
    }

    std::ofstream outFile("sample-compressed.txt", std::ios::binary);
    if (!outFile) {
        std::cerr << "Open Failed For Output File:\n";
        return 1;
    }

    // Example data for characters and their frequencies
    std::vector<char> arr = {'a', 'b', 'c', 'd', 'e', 'f'};
    std::vector<int> freq = {5, 9, 12, 13, 16, 45};

    auto root = buildHuffmanTree(arr, freq);

    std::vector<int> code(100);
    std::map<char, std::vector<int>> huffmanCode;
    printCodesIntoFile(outFile, root, code, 0, huffmanCode);

    inFile.clear();
    inFile.seekg(0);
    compressFile(inFile, outFile, huffmanCode);

    inFile.close();
    outFile.close();

    std::ifstream compressedFile("sample-compressed.txt", std::ios::binary);
    std::ofstream decompressedFile("sample-decompressed.txt", std::ios::binary);
    if (!compressedFile || !decompressedFile) {
        std::cerr << "Open Failed For Compressed/Decompressed File:\n";
        return 1;
    }

    int fileLength = 100; // Example length, set appropriately
    std::shared_ptr<Tree> tree;
    rebuildHuffmanTree(compressedFile, arr.size(), tree);
    decompressFile(compressedFile, decompressedFile, tree, fileLength);

    compressedFile.close();
    decompressedFile.close();

    return 0;
}
