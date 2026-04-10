#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <limits>

// ═══════════════════════════════════════════════════════════════════
//  CONSOLE COLOR HELPERS
// ═══════════════════════════════════════════════════════════════════

static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

enum class Color : WORD {
    Black       = 0,
    DarkBlue    = 1,
    DarkGreen   = 2,
    DarkCyan    = 3,
    DarkRed     = 4,
    DarkMagenta = 5,
    DarkYellow  = 6,
    Gray        = 7,
    DarkGray    = 8,
    Blue        = 9,
    Green       = 10,
    Cyan        = 11,
    Red         = 12,
    Magenta     = 13,
    Yellow      = 14,
    White       = 15
};

void setColor(Color fg, Color bg = Color::Black) {
    SetConsoleTextAttribute(hConsole,
        static_cast<WORD>(fg) | (static_cast<WORD>(bg) << 4));
}

void resetColor() { setColor(Color::Gray); }

// Prints colored text and resets
void cprint(const std::string& text, Color fg, Color bg = Color::Black) {
    setColor(fg, bg);
    std::cout << text;
    resetColor();
}

// ═══════════════════════════════════════════════════════════════════
//  ASCII ART OWL
// ═══════════════════════════════════════════════════════════════════

void printBanner() {
    setColor(Color::DarkGray);
    std::cout << "\n";
    std::cout << "  " << std::string(66, '=') << "\n";
    resetColor();

    // Owl - ASCII art
    setColor(Color::Yellow);
    std::cout << R"(
  ,_.                                                          ,_.
  '\cXX.==- __                                        __ -==,XXv/`
      ~=_X7~ ,/~!g=-,_.                      ,_.-=s!~L. ~TX_=~
         ~=c. = /- T--e'T|=v._  ....   _,v=!7`z--\ -\ = ,v=~
            ~=c` ./ ,-`,/ /i/Z\/~~~~\/Z\i\ \.'-. \, 'v=~
               ~\s,/ ,/ ,/ Y]`/ @/\@ \'[Y \. \. \.g/~
                  '`Yc.v`,/Vs)-  \/  -(sV\.'c,v+'`
                       ~~]mZczTV '` VTevZm[~~
                    ,=-T|--2Y [      ] Y2--!T-=.
                    'i`_ -|-'i!      !i`-!- _'i`
                      '-s|.cf ,!]\/[!. 1v,!g-`
                          ~Y/v/vv..vv\v\Y~
                           ^            ^
)" << "\n";

    setColor(Color::DarkGray);
    std::cout << "  " << std::string(66, '=') << "\n";
    resetColor();

    cprint("  ★  ", Color::Yellow);
    cprint("GLAUX", Color::Green);
    cprint(" — Advanced BMP Steganography Engine\n", Color::White);

    cprint("  ★  ", Color::Yellow);
    cprint("Encryption: ", Color::Cyan);
    cprint("XTEA (eXtended Tiny Encryption Algorithm)\n", Color::White);

    cprint("  ★  ", Color::Yellow);
    cprint("Method:     ", Color::Cyan);
    cprint("LSB (Least Significant Bit) Embedding\n", Color::White);

    setColor(Color::DarkGray);
    std::cout << "  " << std::string(66, '=') << "\n\n";
    resetColor();
}

// ═══════════════════════════════════════════════════════════════════
//  XTEA — eXtended Tiny Encryption Algorithm
// ═══════════════════════════════════════════════════════════════════
namespace XTEA {

    constexpr uint32_t DELTA  = 0x9E3779B9u;
    constexpr uint32_t ROUNDS = 64u;

    // Encrypt a 64-bit block
    void encryptBlock(uint32_t v[2], const uint32_t key[4]) {
        uint32_t v0 = v[0], v1 = v[1], sum = 0;
        for (uint32_t i = 0; i < ROUNDS; ++i) {
            v0  += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + key[sum & 3]);
            sum += DELTA;
            v1  += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + key[(sum >> 11) & 3]);
        }
        v[0] = v0; v[1] = v1;
    }

    // Decrypt a 64-bit block
    void decryptBlock(uint32_t v[2], const uint32_t key[4]) {
        uint32_t v0 = v[0], v1 = v[1], sum = DELTA * ROUNDS;
        for (uint32_t i = 0; i < ROUNDS; ++i) {
            v1  -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + key[(sum >> 11) & 3]);
            sum -= DELTA;
            v0  -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + key[sum & 3]);
        }
        v[0] = v0; v[1] = v1;
    }

    // Generate 128-bit key from passphrase (simple key derivation)
    // Uses djb2-style hashing for each uint32_t chunk of the key.
    void deriveKey(const std::string& passphrase, uint32_t key[4]) {
        // Initialize with magic values
        key[0] = 0xDEADBEEFu;
        key[1] = 0xCAFEBABEu;
        key[2] = 0x8BADF00Du;
        key[3] = 0xFACEFEEDu;

        for (size_t i = 0; i < passphrase.size(); ++i) {
            uint8_t c = static_cast<uint8_t>(passphrase[i]);
            key[i % 4] = ((key[i % 4] << 5) + key[i % 4]) ^ c;
            // Extra mixing: rotate and XOR with position
            key[(i + 1) % 4] ^= (key[i % 4] >> 3) | (key[i % 4] << 29);
        }
    }

    // Encrypt byte vector (with PKCS7-style padding to multiples of 8 bytes)
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                                  const std::string& passphrase) {
        uint32_t key[4];
        deriveKey(passphrase, key);

        // PKCS7 padding: pad to multiple of 8
        size_t padded_len = ((plaintext.size() + 7) / 8) * 8;
        uint8_t pad_byte  = static_cast<uint8_t>(padded_len - plaintext.size());
        if (pad_byte == 0) pad_byte = 8, padded_len += 8;

        std::vector<uint8_t> buf(padded_len);
        std::copy(plaintext.begin(), plaintext.end(), buf.begin());
        std::fill(buf.begin() + plaintext.size(), buf.end(), pad_byte);

        // Encrypt per 8-byte block
        for (size_t i = 0; i < padded_len; i += 8) {
            uint32_t block[2];
            std::memcpy(block, buf.data() + i, 8);
            encryptBlock(block, key);
            std::memcpy(buf.data() + i, block, 8);
        }
        return buf;
    }

    // Decrypt byte vector and remove padding
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                                  const std::string& passphrase) {
        if (ciphertext.empty() || ciphertext.size() % 8 != 0) {
            throw std::runtime_error("Invalid ciphertext size (must be a multiple of 8)");
        }

        uint32_t key[4];
        deriveKey(passphrase, key);

        std::vector<uint8_t> buf = ciphertext;

        // Decrypt per block
        for (size_t i = 0; i < buf.size(); i += 8) {
            uint32_t block[2];
            std::memcpy(block, buf.data() + i, 8);
            decryptBlock(block, key);
            std::memcpy(buf.data() + i, block, 8);
        }

        // Remove PKCS7 padding
        if (buf.empty()) throw std::runtime_error("Empty plaintext after decryption");
        uint8_t pad = buf.back();
        if (pad == 0 || pad > 8) throw std::runtime_error("Wrong key or corrupted data (invalid padding)");
        buf.resize(buf.size() - pad);
        return buf;
    }

} // namespace XTEA

// ═══════════════════════════════════════════════════════════════════
//  BMP HANDLER
// ═══════════════════════════════════════════════════════════════════

class BMPImage {
public:
    static constexpr size_t HEADER_SIZE = 54; // First 54 bytes — NEVER CHANGE

    std::vector<uint8_t> data; // Ολόκληρο το αρχείο (header + pixels)
    uint32_t width  = 0;
    uint32_t height = 0;
    uint16_t bpp    = 0;       // Bits per pixel
    uint32_t pixelOffset = 0;  // Pixel data start position

    bool load(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "File not found: " << path << "\n";
            return false;
        }

        data.assign(std::istreambuf_iterator<char>(f), {});

        if (data.size() < HEADER_SIZE) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "File too small — not a valid BMP.\n";
            return false;
        }

        // Check signature "BM"
        if (data[0] != 'B' || data[1] != 'M') {
            cprint("  [✗] ", Color::Red);
            std::cerr << "Not a BMP file (missing 'BM' signature).\n";
            return false;
        }

        // Read metadata from header (little-endian)
        std::memcpy(&pixelOffset, data.data() + 10, 4);
        std::memcpy(&width,       data.data() + 18, 4);
        std::memcpy(&height,      data.data() + 22, 4);
        std::memcpy(&bpp,         data.data() + 28, 2);

        if (bpp != 24 && bpp != 32) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "Only 24-bit and 32-bit BMP are supported.\n";
            return false;
        }

        return true;
    }

    bool save(const std::string& path) {
        std::ofstream f(path, std::ios::binary);
        if (!f) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "Unable to write: " << path << "\n";
            return false;
        }
        f.write(reinterpret_cast<const char*>(data.data()), data.size());
        return f.good();
    }

    // Maximum capacity in BYTES (1 bit per channel, 3 channels/pixel)
    // First 32 bits (4 bytes) hold the payload size.
    size_t capacityBytes() const {
        size_t pixelBytes = data.size() - pixelOffset;
        size_t totalBits  = pixelBytes; // 1 LSB ανά byte channel
        return (totalBits / 8) - 4;    // -4 για τα 32-bit length header
    }
};

// ═══════════════════════════════════════════════════════════════════
//  LSB STEGANOGRAPHY — EMBED & EXTRACT
// ═══════════════════════════════════════════════════════════════════

namespace LSB {

    /*
     * PAYLOAD STRUCTURE INSIDE THE IMAGE:
     * ─────────────────────────────────────
     * [32 bits: ciphertext size in bytes]
     * [N bytes: ciphertext from XTEA]
     *
     * Each bit is written/read from the LSB of one channel byte.
     * The order is: B, G, R, B, G, R, ... (for 24bpp)
     * For 32bpp: B, G, R, A, B, G, R, A, ...
     */

    // Write a bit to the LSB of the byte at byteIdx
    inline void setBit(std::vector<uint8_t>& data, size_t byteIdx, uint8_t bit) {
        data[byteIdx] = (data[byteIdx] & 0xFEu) | (bit & 1u);
    }

    // Read the LSB of the byte at byteIdx
    inline uint8_t getBit(const std::vector<uint8_t>& data, size_t byteIdx) {
        return data[byteIdx] & 1u;
    }

    // Embed payload bytes into image starting from pixelOffset
    bool embed(BMPImage& img, const std::vector<uint8_t>& payload) {
        size_t pixelStart = img.pixelOffset; // Start AFTER the 54 byte header
        size_t available  = img.data.size() - pixelStart;

        // Need: 32 bits for size + payload.size() * 8 bits
        size_t bitsNeeded = 32 + payload.size() * 8;
        if (bitsNeeded > available) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "CAPACITY CHECK FAILED: Need " << bitsNeeded
                      << " bits, available " << available << " bits.\n";
            return false;
        }

        size_t bitPos = 0; // Current bit position (counted from pixelStart)

        // --- Write the 32-bit payload size (big-endian) ---
        uint32_t payloadSize = static_cast<uint32_t>(payload.size());
        for (int i = 31; i >= 0; --i) {
            uint8_t bit = (payloadSize >> i) & 1u;
            setBit(img.data, pixelStart + bitPos, bit);
            ++bitPos;
        }

        // --- Write the payload bytes ---
        for (uint8_t byte : payload) {
            for (int i = 7; i >= 0; --i) {
                uint8_t bit = (byte >> i) & 1u;
                setBit(img.data, pixelStart + bitPos, bit);
                ++bitPos;
            }
        }

        return true;
    }

    // Extract payload from image
    std::vector<uint8_t> extract(const BMPImage& img) {
        size_t pixelStart = img.pixelOffset;
        size_t available  = img.data.size() - pixelStart;

        if (available < 32) {
            throw std::runtime_error("Image is too small to contain data.");
        }

        size_t bitPos = 0;

        // --- Read the 32-bit size ---
        uint32_t payloadSize = 0;
        for (int i = 31; i >= 0; --i) {
            uint8_t bit = getBit(img.data, pixelStart + bitPos);
            payloadSize |= (static_cast<uint32_t>(bit) << i);
            ++bitPos;
        }

        // Sanity check on size
        if (payloadSize == 0 || payloadSize > 100 * 1024 * 1024u) {
            throw std::runtime_error(
                "Invalid payload size (" + std::to_string(payloadSize) +
                " bytes). Maybe wrong image or no hidden message.");
        }

        size_t bitsNeeded = 32 + payloadSize * 8;
        if (bitsNeeded > available) {
            throw std::runtime_error("Payload exceeds image bounds.");
        }

        // --- Read the bytes ---
        std::vector<uint8_t> payload(payloadSize);
        for (uint32_t b = 0; b < payloadSize; ++b) {
            uint8_t byte = 0;
            for (int i = 7; i >= 0; --i) {
                uint8_t bit = getBit(img.data, pixelStart + bitPos);
                byte |= (bit << i);
                ++bitPos;
            }
            payload[b] = byte;
        }

        return payload;
    }

} // namespace LSB

// ═══════════════════════════════════════════════════════════════════
//  HEX DUMP (για debug/επαλήθευση)
// ═══════════════════════════════════════════════════════════════════

void hexDump(const std::vector<uint8_t>& data, size_t maxBytes = 64) {
    size_t limit = std::min(data.size(), maxBytes);
    setColor(Color::DarkGray);
    for (size_t i = 0; i < limit; ++i) {
        if (i % 16 == 0) std::cout << "\n    " << std::hex << std::setw(4)
                                   << std::setfill('0') << i << "  ";
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << "\n";
    resetColor();
}

// ═══════════════════════════════════════════════════════════════════
//  ENCODE COMMAND - SIMPLIFIED
// ═══════════════════════════════════════════════════════════════════

void cmdEncode() {
    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Green);
    cprint("  ║                    HIDE MESSAGE IN IMAGE                     ║\n", Color::Green);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Green);
    std::cout << "\n";

    // Step 1: Get BMP file
    std::string inputPath;
    while (true) {
        cprint("  STEP 1: ", Color::Cyan);
        std::cout << "Drag and drop your BMP image here, or type the full path:\n";
        cprint("  → ", Color::Yellow);
        std::getline(std::cin, inputPath);

        // Remove quotes if present (from drag & drop)
        if (!inputPath.empty() && inputPath.front() == '"' && inputPath.back() == '"') {
            inputPath = inputPath.substr(1, inputPath.size() - 2);
        }

        // Check if file exists
        std::ifstream testFile(inputPath);
        if (!testFile.good()) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "File not found: " << inputPath << "\n";
            cprint("  [i] ", Color::Yellow);
            std::cout << "Make sure the file exists and the path is correct.\n";
            cprint("  [?] ", Color::Cyan);
            std::cout << "Try again? (Y/n): ";
            std::string retry;
            std::getline(std::cin, retry);
            if (retry == "n" || retry == "N") return;
            continue;
        }
        testFile.close();
        break;
    }

    BMPImage img;
    if (!img.load(inputPath)) {
        cprint("  [✗] ", Color::Red);
        std::cerr << "This doesn't appear to be a valid BMP file.\n";
        cprint("  [i] ", Color::Yellow);
        std::cout << "BMP files should be 24-bit or 32-bit color depth.\n";
        return;
    }

    cprint("  [✓] ", Color::Green);
    std::cout << "Image loaded successfully!\n";
    cprint("  [i] ", Color::Yellow);
    std::cout << "Size: " << img.width << "×" << img.height << " pixels\n";
    cprint("  [i] ", Color::Yellow);
    std::cout << "Can hide up to: " << img.capacityBytes() << " characters\n\n";

    // Step 2: Get message
    std::string message;
    while (true) {
        cprint("  STEP 2: ", Color::Cyan);
        std::cout << "Type your secret message:\n";
        cprint("  → ", Color::Yellow);
        std::getline(std::cin, message);

        if (message.empty()) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "Message cannot be empty.\n";
            continue;
        }

        if (message.size() > img.capacityBytes()) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "Message too long! Maximum: " << img.capacityBytes() << " characters\n";
            cprint("  [i] ", Color::Yellow);
            std::cout << "Your message: " << message.size() << " characters\n";
            cprint("  [?] ", Color::Cyan);
            std::cout << "Shorten message? (Y/n): ";
            std::string retry;
            std::getline(std::cin, retry);
            if (retry == "n" || retry == "N") return;
            continue;
        }
        break;
    }

    // Step 3: Get passphrase
    std::string passphrase;
    while (true) {
        cprint("  STEP 3: ", Color::Cyan);
        std::cout << "Create a passphrase (password) to protect your message:\n";
        cprint("  → ", Color::Yellow);
        std::getline(std::cin, passphrase);

        if (passphrase.empty()) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "Passphrase cannot be empty.\n";
            continue;
        }

        if (passphrase.size() < 4) {
            cprint("  [⚠] ", Color::Yellow);
            std::cout << "Weak passphrase! Use at least 4 characters.\n";
            cprint("  [?] ", Color::Cyan);
            std::cout << "Continue anyway? (y/N): ";
            std::string confirm;
            std::getline(std::cin, confirm);
            if (confirm != "y" && confirm != "Y") continue;
        }
        break;
    }

    // Step 4: Get output file
    std::string outputPath;
    while (true) {
        cprint("  STEP 4: ", Color::Cyan);
        std::cout << "Choose where to save the encoded image:\n";
        cprint("  → ", Color::Yellow);
        std::cout << "Type filename (e.g., secret_image.bmp): ";
        std::getline(std::cin, outputPath);

        if (outputPath.empty()) {
            outputPath = inputPath.substr(0, inputPath.find_last_of('.')) + "_encoded.bmp";
            cprint("  [i] ", Color::Yellow);
            std::cout << "Using default: " << outputPath << "\n";
            break;
        }

        // Add .bmp extension if missing
        if (outputPath.find('.') == std::string::npos) {
            outputPath += ".bmp";
        }

        // Check if output file already exists
        std::ifstream checkFile(outputPath);
        if (checkFile.good()) {
            checkFile.close();
            cprint("  [⚠] ", Color::Yellow);
            std::cout << "File already exists: " << outputPath << "\n";
            cprint("  [?] ", Color::Cyan);
            std::cout << "Overwrite? (y/N): ";
            std::string confirm;
            std::getline(std::cin, confirm);
            if (confirm != "y" && confirm != "Y") continue;
        }
        break;
    }

    // Processing
    cprint("\n  [~] ", Color::Yellow);
    std::cout << "Encrypting your message...\n";

    std::vector<uint8_t> plainVec(message.begin(), message.end());
    std::vector<uint8_t> cipher = XTEA::encrypt(plainVec, passphrase);

    cprint("  [~] ", Color::Yellow);
    std::cout << "Hiding message in image...\n";

    if (!LSB::embed(img, cipher)) {
        cprint("  [✗] ", Color::Red);
        std::cerr << "Failed to hide message in image.\n";
        return;
    }

    cprint("  [~] ", Color::Yellow);
    std::cout << "Saving encoded image...\n";

    if (!img.save(outputPath)) {
        cprint("  [✗] ", Color::Red);
        std::cerr << "Failed to save file.\n";
        return;
    }

    // Success!
    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Green);
    cprint("  ║                    ✓ MESSAGE HIDDEN!                        ║\n", Color::Green);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Green);
    std::cout << "\n";
    cprint("  [✓] ", Color::Green);
    std::cout << "Secret message hidden in: " << outputPath << "\n";
    cprint("  [✓] ", Color::Green);
    std::cout << "Passphrase: " << passphrase << "\n";
    cprint("  [i] ", Color::Yellow);
    std::cout << "Remember your passphrase - you'll need it to read the message!\n";
    cprint("  [i] ", Color::Yellow);
    std::cout << "The image looks normal - only you know the secret!\n\n";

    cprint("  Press Enter to continue...", Color::Cyan);
    std::cin.ignore();
}

// ═══════════════════════════════════════════════════════════════════
//  DECODE COMMAND - SIMPLIFIED
// ═══════════════════════════════════════════════════════════════════

void cmdDecode() {
    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Magenta);
    cprint("  ║                   EXTRACT HIDDEN MESSAGE                     ║\n", Color::Magenta);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Magenta);
    std::cout << "\n";

    // Step 1: Get BMP file with hidden message
    std::string inputPath;
    while (true) {
        cprint("  STEP 1: ", Color::Cyan);
        std::cout << "Drag and drop the BMP image with hidden message, or type the path:\n";
        cprint("  → ", Color::Yellow);
        std::getline(std::cin, inputPath);

        // Remove quotes if present (from drag & drop)
        if (!inputPath.empty() && inputPath.front() == '"' && inputPath.back() == '"') {
            inputPath = inputPath.substr(1, inputPath.size() - 2);
        }

        // Check if file exists
        std::ifstream testFile(inputPath);
        if (!testFile.good()) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "File not found: " << inputPath << "\n";
            cprint("  [i] ", Color::Yellow);
            std::cout << "Make sure you selected the correct encoded image.\n";
            cprint("  [?] ", Color::Cyan);
            std::cout << "Try again? (Y/n): ";
            std::string retry;
            std::getline(std::cin, retry);
            if (retry == "n" || retry == "N") return;
            continue;
        }
        testFile.close();
        break;
    }

    BMPImage img;
    if (!img.load(inputPath)) {
        cprint("  [✗] ", Color::Red);
        std::cerr << "This doesn't appear to be a valid BMP file.\n";
        cprint("  [i] ", Color::Yellow);
        std::cout << "Make sure you're using the encoded image from step 1.\n";
        return;
    }

    cprint("  [✓] ", Color::Green);
    std::cout << "Image loaded successfully!\n\n";

    // Step 2: Get passphrase
    std::string passphrase;
    while (true) {
        cprint("  STEP 2: ", Color::Cyan);
        std::cout << "Enter the passphrase you used to hide the message:\n";
        cprint("  → ", Color::Yellow);
        std::getline(std::cin, passphrase);

        if (passphrase.empty()) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "Passphrase cannot be empty.\n";
            continue;
        }
        break;
    }

    // Processing
    cprint("\n  [~] ", Color::Yellow);
    std::cout << "Extracting hidden data...\n";

    std::vector<uint8_t> cipher;
    try {
        cipher = LSB::extract(img);
    } catch (const std::exception& e) {
        cprint("  [✗] ", Color::Red);
        std::cerr << "No hidden message found in this image.\n";
        cprint("  [i] ", Color::Yellow);
        std::cout << "Make sure you're using an image that contains a hidden message.\n";
        return;
    }

    cprint("  [~] ", Color::Yellow);
    std::cout << "Decrypting message...\n";

    std::vector<uint8_t> plain;
    try {
        plain = XTEA::decrypt(cipher, passphrase);
    } catch (const std::exception& e) {
        cprint("  [✗] ", Color::Red);
        std::cerr << "Wrong passphrase or corrupted data.\n";
        cprint("  [i] ", Color::Yellow);
        std::cout << "Make sure you entered the correct passphrase.\n";
        cprint("  [?] ", Color::Cyan);
        std::cout << "Try a different passphrase? (Y/n): ";
        std::string retry;
        std::getline(std::cin, retry);
        if (retry != "n" && retry != "N") {
            // Recursively call decode again
            cmdDecode();
            return;
        }
        return;
    }

    std::string message(plain.begin(), plain.end());

    // Success!
    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Magenta);
    cprint("  ║                   ✓ MESSAGE FOUND!                          ║\n", Color::Magenta);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Magenta);
    std::cout << "\n";
    cprint("  SECRET MESSAGE:\n", Color::Green);
    cprint("  ──────────────────────────────────────────────────────────────\n", Color::DarkGray);
    cprint("  ", Color::White);
    std::cout << message << "\n";
    cprint("  ──────────────────────────────────────────────────────────────\n", Color::DarkGray);
    cprint("  [i] ", Color::Cyan);
    std::cout << "Message length: " << message.size() << " characters\n\n";

    cprint("  Press Enter to continue...", Color::Cyan);
    std::cin.ignore();
}

// ═══════════════════════════════════════════════════════════════════
//  INFO COMMAND — SIMPLIFIED BMP INFORMATION
// ═══════════════════════════════════════════════════════════════════

void cmdInfo() {
    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Cyan);
    cprint("  ║                   IMAGE INFORMATION                         ║\n", Color::Cyan);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Cyan);
    std::cout << "\n";

    std::string path;
    while (true) {
        cprint("  STEP 1: ", Color::Cyan);
        std::cout << "Drag and drop a BMP image here, or type the full path:\n";
        cprint("  → ", Color::Yellow);
        std::getline(std::cin, path);

        // Remove quotes if present (from drag & drop)
        if (!path.empty() && path.front() == '"' && path.back() == '"') {
            path = path.substr(1, path.size() - 2);
        }

        // Check if file exists
        std::ifstream testFile(path);
        if (!testFile.good()) {
            cprint("  [✗] ", Color::Red);
            std::cerr << "File not found: " << path << "\n";
            cprint("  [?] ", Color::Cyan);
            std::cout << "Try again? (Y/n): ";
            std::string retry;
            std::getline(std::cin, retry);
            if (retry == "n" || retry == "N") return;
            continue;
        }
        testFile.close();
        break;
    }

    BMPImage img;
    if (!img.load(path)) {
        cprint("  [✗] ", Color::Red);
        std::cerr << "This doesn't appear to be a valid BMP file.\n";
        cprint("  [i] ", Color::Yellow);
        std::cout << "BMP files should be 24-bit or 32-bit color depth.\n";
        return;
    }

    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Cyan);
    cprint("  ║                     IMAGE DETAILS                           ║\n", Color::Cyan);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Cyan);
    std::cout << "\n";

    cprint("  📁 File: ", Color::White);
    std::cout << path << "\n";
    cprint("  📐 Size: ", Color::White);
    std::cout << img.width << " × " << img.height << " pixels\n";
    cprint("  🎨 Colors: ", Color::White);
    std::cout << img.bpp << " bits per pixel\n";
    cprint("  💾 File size: ", Color::White);
    std::cout << img.data.size() << " bytes\n";
    cprint("  🔒 Steganography capacity: ", Color::Green);
    std::cout << img.capacityBytes() << " characters\n";

    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Green);
    cprint("  ║                     WHAT THIS MEANS                         ║\n", Color::Green);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Green);
    std::cout << "\n";

    if (img.capacityBytes() < 100) {
        cprint("  ⚠️  SMALL IMAGE: ", Color::Yellow);
        std::cout << "Can only hide short messages\n";
    } else if (img.capacityBytes() < 1000) {
        cprint("  ✅ MEDIUM IMAGE: ", Color::Green);
        std::cout << "Good for hiding notes or short messages\n";
    } else {
        cprint("  🌟 LARGE IMAGE: ", Color::Green);
        std::cout << "Perfect for hiding longer messages or files\n";
    }

    cprint("  💡 TIP: ", Color::Cyan);
    std::cout << "Larger images can hide more secret data!\n\n";

    cprint("  Press Enter to continue...", Color::Cyan);
    std::cin.ignore();
}

// ═══════════════════════════════════════════════════════════════════
//  MAIN MENU - SIMPLIFIED
// ═══════════════════════════════════════════════════════════════════

void printMenu() {
    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Green);
    cprint("  ║                    GLAUX STEGANOGRAPHY                      ║\n", Color::Green);
    cprint("  ║              Hide Secret Messages in Images                 ║\n", Color::Green);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Green);
    std::cout << "\n";

    cprint("  What would you like to do?\n\n", Color::White);

    cprint("  [1] ", Color::Yellow);
    cprint("🔒 HIDE MESSAGE", Color::Green);
    cprint(" - Put a secret message in an image\n", Color::White);

    cprint("  [2] ", Color::Yellow);
    cprint("🔓 EXTRACT MESSAGE", Color::Magenta);
    cprint(" - Read a hidden message from an image\n", Color::White);

    cprint("  [3] ", Color::Yellow);
    cprint("ℹ️  CHECK IMAGE", Color::Cyan);
    cprint(" - See how much data an image can hide\n", Color::White);

    cprint("  [0] ", Color::Red);
    cprint("🚪 EXIT", Color::Red);
    cprint(" - Close the program\n", Color::White);

    std::cout << "\n";
    cprint("  Choose an option (0-3): ", Color::Cyan);
}

int main() {
    // Set up console for UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // Window title
    SetConsoleTitleA("Glaux — Hide Secret Messages in Images");

    printBanner();

    // Welcome message
    std::cout << "\n";
    cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Cyan);
    cprint("  ║                    WELCOME TO GLAUX!                        ║\n", Color::Cyan);
    cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Cyan);
    std::cout << "\n";

    cprint("  🦉 Glaux is a tool that lets you hide secret messages inside images.\n", Color::White);
    cprint("  🔒 Your messages are encrypted and invisible to the naked eye.\n", Color::White);
    cprint("  📁 Works with BMP images (24-bit or 32-bit color).\n", Color::White);
    cprint("  🎯 Just drag and drop files - no typing long paths!\n\n", Color::White);

    cprint("  Press Enter to continue...", Color::Green);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string choice;
    while (true) {
        printMenu();
        std::getline(std::cin, choice);

        if      (choice == "1") cmdEncode();
        else if (choice == "2") cmdDecode();
        else if (choice == "3") cmdInfo();
        else if (choice == "0") {
            std::cout << "\n";
            cprint("  ╔══════════════════════════════════════════════════════════════╗\n", Color::Yellow);
            cprint("  ║                    THANK YOU FOR USING GLAUX!               ║\n", Color::Yellow);
            cprint("  ╚══════════════════════════════════════════════════════════════╝\n", Color::Yellow);
            cprint("  🦉 The owl says goodbye! Keep your secrets safe.\n\n", Color::Yellow);
            break;
        } else {
            cprint("  [⚠] ", Color::Yellow);
            std::cout << "Please choose 0, 1, 2, or 3.\n";
            cprint("  Press Enter to try again...", Color::Cyan);
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    resetColor();
    return 0;
}
