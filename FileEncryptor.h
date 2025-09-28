#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

class CustomFileEncryptor {
private:
    std::string userKey;
    unsigned char generateTransformValue(size_t position) {
        if (userKey.empty()) {
            return (position * 7 + 13) % 256;
        }
        
        unsigned long keySum = 0;
        for (char c : userKey) {
            keySum += static_cast<unsigned char>(c);
        }
        
        return ((position * keySum) + (keySum * 3) + (position % 17)) % 256;
    }
    unsigned char transformByte(unsigned char byte, size_t position, bool encrypt) {
        unsigned char transform = generateTransformValue(position);
        unsigned char result;
        
        if (encrypt) {
            result = ((byte + transform) * 5) % 256;
            result = (result + (position % 13)) % 256;
            result = ((result << 2) | (result >> 6)) % 256;
        } else {
            result = ((byte >> 2) | (byte << 6)) % 256;
            result = (result - (position % 13) + 256) % 256;
            result = findInverseMultiplication(result);
            result = (result - transform + 256) % 256;
        }
        
        return result;
    }
    unsigned char findInverseMultiplication(unsigned char value) {
        for (int x = 0; x < 256; x++) {
            if ((x * 5) % 256 == value) {
                return x;
            }
        }
        return value;
    }
    std::vector<unsigned char> matrixTransform(const std::vector<unsigned char>& data, bool encrypt) {
        std::vector<unsigned char> result = data;
        size_t size = data.size();
        
        if (size < 2) return result;

        for (size_t i = 0; i < size - 1; i += 2) {
            unsigned char a = result[i];
            unsigned char b = result[i + 1];
            
            if (encrypt) {
                result[i] = (a + b * 2) % 256;
                result[i + 1] = (b + a * 3) % 256;
            } else {
                result[i] = solveForOriginalA(a, b);
                result[i + 1] = solveForOriginalB(a, b);
            }
        }
        
        return result;
    }
    unsigned char solveForOriginalA(unsigned char transformedA, unsigned char transformedB) {
        
        for (int origA = 0; origA < 256; origA++) {
            for (int origB = 0; origB < 256; origB++) {
                if ((origA + origB * 2) % 256 == transformedA && 
                    (origB + origA * 3) % 256 == transformedB) {
                    return origA;
                }
            }
        }
        return transformedA;
    }

    unsigned char solveForOriginalB(unsigned char transformedA, unsigned char transformedB) {
        for (int origA = 0; origA < 256; origA++) {
            for (int origB = 0; origB < 256; origB++) {
                if ((origA + origB * 2) % 256 == transformedA && 
                    (origB + origA * 3) % 256 == transformedB) {
                    return origB;
                }
            }
        }
        return transformedB;
    }

    unsigned char waveTransform(unsigned char byte, size_t position, bool encrypt) {
        double wave = sin(position * 0.1) * 127 + 127; // Generate wave between 0-254
        unsigned char waveValue = static_cast<unsigned char>(wave);
        
        if (encrypt) {
            return (byte + waveValue) % 256;
        } else {
            return (byte - waveValue + 256) % 256;
        }
    }

public:
    CustomFileEncryptor(const std::string& key = "") : userKey(key) {}

    bool encryptFile(const std::string& inputFile, const std::string& outputFile) {
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Error: Cannot open input file '" << inputFile << "'" << std::endl;
            return false;
        }

        std::vector<unsigned char> data;
        char byte;
        while (inFile.get(byte)) {
            data.push_back(static_cast<unsigned char>(byte));
        }
        inFile.close();
        
        if (data.empty()) {
            std::cerr << "Error: Input file is empty" << std::endl;
            return false;
        }

        for (size_t i = 0; i < data.size(); i++) {
            data[i] = transformByte(data[i], i, true);
        }

        data = matrixTransform(data, true);

        for (size_t i = 0; i < data.size(); i++) {
            data[i] = waveTransform(data[i], i, true);
        }

        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Cannot create output file '" << outputFile << "'" << std::endl;
            return false;
        }
        
        for (unsigned char encryptedByte : data) {
            outFile.put(static_cast<char>(encryptedByte));
        }
        outFile.close();
        
        std::cout << "File encrypted successfully! (" << data.size() << " bytes processed)" << std::endl;
        return true;
    }

    bool decryptFile(const std::string& inputFile, const std::string& outputFile) {
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Error: Cannot open encrypted file '" << inputFile << "'" << std::endl;
            return false;
        }

        std::vector<unsigned char> data;
        char byte;
        while (inFile.get(byte)) {
            data.push_back(static_cast<unsigned char>(byte));
        }
        inFile.close();
        
        if (data.empty()) {
            std::cerr << "Error: Encrypted file is empty" << std::endl;
            return false;
        }

        for (size_t i = 0; i < data.size(); i++) {
            data[i] = waveTransform(data[i], i, false);
        }

        data = matrixTransform(data, false);

        for (size_t i = 0; i < data.size(); i++) {
            data[i] = transformByte(data[i], i, false);
        }

        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Cannot create output file '" << outputFile << "'" << std::endl;
            return false;
        }
        
        for (unsigned char decryptedByte : data) {
            outFile.put(static_cast<char>(decryptedByte));
        }
        outFile.close();
        
        std::cout << "File decrypted successfully! (" << data.size() << " bytes processed)" << std::endl;
        return true;
    }

    bool validateFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: File '" << filename << "' not found or cannot be opened" << std::endl;
            return false;
        }
        
        file.seekg(0, std::ios::end);
        long size = file.tellg();
        file.close();
        
        if (size == 0) {
            std::cerr << "Warning: File '" << filename << "' is empty" << std::endl;
            return false;
        }
        
        std::cout << "File '" << filename << "' validated successfully (" << size << " bytes)" << std::endl;
        return true;
    }
    
    void setKey(const std::string& newKey) {
        userKey = newKey;
        std::cout << "Encryption key updated" << std::endl;
    }
    
    void displayAlgorithmInfo() {
        std::cout << "\n=== Custom Encryption Algorithm Details ===" << std::endl;
        std::cout << "Stage 1: Mathematical byte transformation using key-based values" << std::endl;
        std::cout << "Stage 2: Custom matrix operations on byte pairs" << std::endl;
        std::cout << "Stage 3: Sine wave-based position transformation" << std::endl;
        std::cout << "Key Impact: Affects mathematical transformations and position calculations" << std::endl;
        std::cout << "Reversibility: Each stage has corresponding inverse operations" << std::endl;
    }
};

class UserInterface {
private:
    CustomFileEncryptor* encryptor;
    
    void showWelcome() {
        std::cout << "\n" << std::string(60, '*') << std::endl;
        std::cout << "         CUSTOM FILE ENCRYPTION SYSTEM" << std::endl;
        std::cout << "    Original Algorithm - Educational Implementation" << std::endl;
        std::cout << std::string(60, '*') << std::endl;
    }
    
    void showMenu() {
        std::cout << "\n--- MAIN MENU ---" << std::endl;
        std::cout << "1. Encrypt a file" << std::endl;
        std::cout << "2. Decrypt a file" << std::endl;
        std::cout << "3. Change encryption key" << std::endl;
        std::cout << "4. View algorithm information" << std::endl;
        std::cout << "5. Test with sample file" << std::endl;
        std::cout << "6. Exit program" << std::endl;
        std::cout << "\nSelect option (1-6): ";
    }
    
    std::string getUserInput(const std::string& prompt) {
        std::string input;
        std::cout << prompt;
        std::cin.ignore();
        std::getline(std::cin, input);
        return input;
    }
    
    void handleEncryption() {
        std::cout << "\n--- FILE ENCRYPTION ---" << std::endl;
        std::string inputFile = getUserInput("Enter path to file to encrypt: ");
        
        if (!encryptor->validateFile(inputFile)) {
            return;
        }
        
        std::string outputFile = getUserInput("Enter name for encrypted output file: ");
        std::cout << "\nStarting encryption process..." << std::endl;
        encryptor->encryptFile(inputFile, outputFile);
    }
    
    void handleDecryption() {
        std::cout << "\n--- FILE DECRYPTION ---" << std::endl;
        std::string inputFile = getUserInput("Enter path to encrypted file: ");
        
        if (!encryptor->validateFile(inputFile)) {
            return;
        }
        
        std::string outputFile = getUserInput("Enter name for decrypted output file: ");
        std::cout << "\nStarting decryption process..." << std::endl;
        encryptor->decryptFile(inputFile, outputFile);
    }
    
    void handleKeyChange() {
        std::cout << "\n--- KEY MANAGEMENT ---" << std::endl;
        std::cout << "Current key affects all three encryption stages." << std::endl;
        std::string newKey = getUserInput("Enter new encryption key: ");
        encryptor->setKey(newKey);
    }
    
    void createTestFile() {
        std::cout << "\n--- CREATING TEST FILE ---" << std::endl;
        std::ofstream testFile("test_sample.txt");
        if (testFile.is_open()) {
            testFile << "=== ENCRYPTION TEST FILE ===" << std::endl;
            testFile << "This file tests our custom encryption algorithm." << std::endl;
            testFile << "Mixed content: UPPERCASE, lowercase, 123456789" << std::endl;
            testFile << "Special chars: !@#$%^&*()_+-=[]{}|\\:;\"'<>,.?/" << std::endl;
            testFile << "Unicode test: àáâãäåæçèéêë" << std::endl;
            testFile << "Numbers: 0.123456789 -987.654321" << std::endl;
            testFile << "End of test file." << std::endl;
            testFile.close();
            
            std::cout << "Test file 'test_sample.txt' created successfully!" << std::endl;
            std::cout << "You can now encrypt this file to test the algorithm." << std::endl;
        } else {
            std::cerr << "Error: Could not create test file" << std::endl;
        }
    }

public:
    UserInterface() {
        encryptor = new CustomFileEncryptor();
    }
    
    ~UserInterface() {
        delete encryptor;
    }
    
    void runProgram() {
        showWelcome();
        
        std::cout << "\nInitial setup - Please set an encryption key." << std::endl;
        std::string initialKey = getUserInput("Enter encryption key (or press Enter for default): ");
        encryptor->setKey(initialKey);
        
        int choice = 0;
        bool keepRunning = true;
        
        while (keepRunning) {
            showMenu();
            
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                std::cout << "\nInvalid input! Please enter a number 1-6." << std::endl;
                continue;
            }
            
            switch (choice) {
                case 1:
                    handleEncryption();
                    break;
                case 2:
                    handleDecryption();
                    break;
                case 3:
                    handleKeyChange();
                    break;
                case 4:
                    encryptor->displayAlgorithmInfo();
                    break;
                case 5:
                    createTestFile();
                    break;
                case 6:
                    std::cout << "\nThank you for using the Custom File Encryption System!" << std::endl;
                    keepRunning = false;
                    break;
                default:
                    std::cout << "\nInvalid choice! Please select 1-6." << std::endl;
                    break;
            }
            
            if (keepRunning && choice >= 1 && choice <= 5) {
                std::cout << "\nPress Enter to continue...";
                std::cin.ignore();
                std::cin.get();
            }
        }
    }
};

int main() {
    try {
        UserInterface ui;
        ui.runProgram();
    } catch (const std::exception& e) {
        std::cerr << "Critical error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred!" << std::endl;
        return 1;
    }
    
    return 0;
}