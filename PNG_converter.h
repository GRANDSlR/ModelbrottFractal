#pragma once
#define NOMINMAX
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <windows.h>
#include <shellapi.h>

// ===================== Image =====================
struct Image {
    int w = 0, h = 0;
    std::vector<unsigned char> rgb; // w*h*3
};

// ===================== CRC32 (для чанков PNG) =====================
inline uint32_t crc32_table_entry(uint32_t n) {
    uint32_t c = n;
    for (int k = 0; k < 8; ++k)
        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
    return c;
}

inline uint32_t crc32_buf(const unsigned char* data, size_t len) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) table[i] = crc32_table_entry(i);
        init = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        c = table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// ===================== Adler32 (для zlib-потока) =====================
inline uint32_t adler32_buf(const unsigned char* data, size_t len) {
    const uint32_t MOD = 65521;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % MOD;
        b = (b + a) % MOD;
    }
    return (b << 16) | a;
}

// ===================== PPM reader (P6, maxval=255) =====================
inline void skipWS(std::istream& in) {
    while (true) {
        int c = in.peek();
        if (c == '#') { std::string line; std::getline(in, line); }
        else if (c != EOF && std::isspace(c)) in.get();
        else break;
    }
}

inline bool readPPM(const std::string& path, Image& img) {
    std::ifstream in(path, std::ios::binary);   // binary — безопасно и для P3, и для P6
    if (!in) { std::cerr << "readPPM: cannot open " << path << "\n"; return false; }

    std::string magic;
    in >> magic;
    if (magic != "P6" && magic != "P3") {
        std::cerr << "readPPM: magic=" << magic << ", need P6 or P3\n";
        return false;
    }

    skipWS(in); in >> img.w;
    skipWS(in); in >> img.h;
    skipWS(in); int maxval = 0; in >> maxval;

    if (img.w <= 0 || img.h <= 0) {
        std::cerr << "readPPM: bad size " << img.w << "x" << img.h << "\n";
        return false;
    }
    if (maxval <= 0 || maxval > 65535) {
        std::cerr << "readPPM: bad maxval=" << maxval << "\n";
        return false;
    }

    img.rgb.resize((size_t)img.w * img.h * 3);

    if (magic == "P6") {
        // ---- бинарный P6 ----
        int c = in.get();            // ровно один whitespace после maxval
        if (c == '\r') in.get();     // CRLF

        if (maxval == 255) {
            in.read(reinterpret_cast<char*>(img.rgb.data()),
                (std::streamsize)img.rgb.size());
            if ((size_t)in.gcount() != img.rgb.size()) {
                std::cerr << "readPPM(P6): short read (" << in.gcount()
                    << " of " << img.rgb.size() << ")\n";
                return false;
            }
        }
        else {
            // 16-битный P6 (maxval > 255): 2 байта на канал, big-endian
            std::vector<unsigned char> buf(img.rgb.size() * 2);
            in.read(reinterpret_cast<char*>(buf.data()), (std::streamsize)buf.size());
            if ((size_t)in.gcount() != buf.size()) {
                std::cerr << "readPPM(P6/16): short read\n";
                return false;
            }
            for (size_t i = 0; i < img.rgb.size(); ++i) {
                unsigned v = (unsigned(buf[i * 2]) << 8) | buf[i * 2 + 1];
                img.rgb[i] = (unsigned char)std::lround((double)v * 255.0 / maxval);
            }
        }
    }
    else {
        // ---- текстовый P3 ----
        for (size_t i = 0; i < img.rgb.size(); ++i) {
            int v = 0;
            skipWS(in);
            if (!(in >> v)) {
                std::cerr << "readPPM(P3): parse error at channel " << i << "\n";
                return false;
            }
            if (v < 0) v = 0;
            if (v > maxval) v = maxval;
            img.rgb[i] = (maxval == 255)
                ? (unsigned char)v
                : (unsigned char)std::lround((double)v * 255.0 / maxval);
        }
    }
    return true;
}

// ===================== PNG writer =====================
inline void putU32(std::vector<unsigned char>& v, uint32_t x) {
    v.push_back((unsigned char)(x >> 24));
    v.push_back((unsigned char)(x >> 16));
    v.push_back((unsigned char)(x >> 8));
    v.push_back((unsigned char)(x));
}

inline void writeChunk(std::ofstream& out, const char type[4],
    const std::vector<unsigned char>& data) {
    std::vector<unsigned char> buf;
    buf.reserve(12 + data.size());
    putU32(buf, (uint32_t)data.size());
    buf.insert(buf.end(), type, type + 4);
    buf.insert(buf.end(), data.begin(), data.end());
    uint32_t crc = crc32_buf(buf.data() + 4, buf.size() - 4); // type+data
    putU32(buf, crc);
    out.write(reinterpret_cast<const char*>(buf.data()), (std::streamsize)buf.size());
}

// zlib-поток из "stored" deflate-блоков (BTYPE=00, без сжатия)
inline std::vector<unsigned char> zlibStored(const std::vector<unsigned char>& raw) {
    std::vector<unsigned char> out;
    out.reserve(raw.size() + raw.size() / 65535 * 5 + 16);

    // zlib header: CMF=0x78, FLG=0x01 (check bits ok)
    out.push_back(0x78);
    out.push_back(0x01);

    size_t pos = 0;
    const size_t MAX = 65535;
    while (pos < raw.size()) {
        size_t n = std::min(MAX, raw.size() - pos);
        bool last = (pos + n == raw.size());
        out.push_back(last ? 0x01 : 0x00);          // BFINAL + BTYPE=00
        out.push_back((unsigned char)(n & 0xFF));   // LEN
        out.push_back((unsigned char)(n >> 8));
        out.push_back((unsigned char)(~n & 0xFF));  // NLEN
        out.push_back((unsigned char)((~n >> 8) & 0xFF));
        out.insert(out.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
    }
    // если raw пуст — нужен хотя бы один пустой блок
    if (raw.empty()) {
        out.push_back(0x01);
        out.push_back(0x00); out.push_back(0x00);
        out.push_back(0xFF); out.push_back(0xFF);
    }

    uint32_t ad = adler32_buf(raw.data(), raw.size());
    putU32(out, ad); // adler32 в big-endian
    return out;
}

inline bool writePNG(const std::string& path, const Image& img) {
    if (img.w <= 0 || img.h <= 0 || img.rgb.size() != (size_t)img.w * img.h * 3) {
        std::cerr << "writePNG: bad image\n";
        return false;
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) { std::cerr << "writePNG: cannot create " << path << "\n"; return false; }

    // 1. сигнатура
    static const unsigned char sig[8] = { 0x89,'P','N','G','\r','\n',0x1a,'\n' };
    out.write(reinterpret_cast<const char*>(sig), 8);

    // 2. IHDR
    std::vector<unsigned char> ihdr;
    putU32(ihdr, (uint32_t)img.w);
    putU32(ihdr, (uint32_t)img.h);
    ihdr.push_back(8); // bit depth
    ihdr.push_back(2); // color type: truecolor RGB
    ihdr.push_back(0); // compression
    ihdr.push_back(0); // filter
    ihdr.push_back(0); // interlace
    writeChunk(out, "IHDR", ihdr);

    // 3. сырые данные: filter byte 0 + строка RGB
    std::vector<unsigned char> raw;
    raw.reserve((size_t)img.h * (1 + (size_t)img.w * 3));
    for (int y = 0; y < img.h; ++y) {
        raw.push_back(0);
        const unsigned char* row = &img.rgb[(size_t)y * img.w * 3];
        raw.insert(raw.end(), row, row + (size_t)img.w * 3);
    }

    // 4. IDAT (zlib stored)
    std::vector<unsigned char> comp = zlibStored(raw);
    writeChunk(out, "IDAT", comp);

    // 5. IEND
    writeChunk(out, "IEND", {});

    if (!out.good()) { std::cerr << "writePNG: write error\n"; return false; }
    return true;
}

// ===================== Точка активации =====================
// path     — путь к .ppm
// fileName — имя выходного файла БЕЗ расширения (например "mandelbrot")
inline int convertPPMtoPNG(const std::string& path, const std::string& fileName) {
    Image img;
    if (!readPPM(path, img)) {
        std::cerr << "convertPPMtoPNG: read failed\n";
        std::cerr << path;
        return 1;
    }
    std::cout << "Прочитано: " << img.w << "x" << img.h
        << ", байт: " << img.rgb.size() << "\n";

    std::string outName = fileName + ".png";
    if (!writePNG(outName, img)) {
        std::cerr << "convertPPMtoPNG: write failed\n";
        return 1;
    }
    std::cout << "Готово: " << outName << "\n";

    ShellExecuteA(NULL, "open", outName.c_str(), NULL, NULL, SW_SHOWNORMAL);

    return 0;
}