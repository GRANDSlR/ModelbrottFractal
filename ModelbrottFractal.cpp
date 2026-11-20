#include <iostream>
#include <fstream>
#include <complex>
#include <vector>
#include <ctime>
#include <iomanip>
#include <algorithm>

#include "PNG_converter.h"

#define _CRT_SECURE_NO_WARNINGS
template <typename T>
T clamp(T value, T low, T high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}
const double M_PI = 3.14159265358979323846;

void palette(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    
    double a = 0.5, b0 = 0.5;
    double rF = a + b0 * std::cos(6.28318 * (t + 0.00));
    double gF = a + b0 * std::cos(6.28318 * (t + 0.33));
    double bF = a + b0 * std::cos(6.28318 * (t + 0.67));
    r = (unsigned char)(255.0 * clamp(rF, 0.0, 1.0));
    g = (unsigned char)(255.0 * clamp(gF, 0.0, 1.0));
    b = (unsigned char)(255.0 * clamp(bF, 0.0, 1.0));
}


void palette2(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    r = (unsigned char)(255 * t);          // от чёрного к красному
    g = (unsigned char)(255 * (1 - t));    // от зелёного к чёрному
    b = (unsigned char)(255 * (0.5 + 0.5 * t)); // от серого к голубому
}

// красный - желый
void palette3(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    // t ∈ [0,1]
    if (t < 0.5) {
        // От чёрного к красному
        double k = t / 0.5;
        r = (unsigned char)(255 * k);
        g = 0;
        b = 0;
    }
    else {
        // От красного к желтому
        double k = (t - 0.5) / 0.5;
        r = 255;
        g = (unsigned char)(255 * k);
        b = 0;
    }
}

// голубой - синий
void palette4(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    // t ∈ [0,1]
    if (t < 0.5) {
        // От чёрного к голубому (cyan)
        double k = t / 0.5; 
        // Степень + для более длительного чёрного
        // power - степень для четкости границы между черным и голубым
        double power = 1.5;
        double bright = pow(k, power);

        r = 0;
        g = (unsigned char)(255 * bright);
        b = (unsigned char)(255 * bright);
    }
    else {
        // От голубого к синему
        double k = (t - 0.5) / 0.5;

        r = 0;
        // Голубой компонент (green + blue) убывает до чистого синего
        g = (unsigned char)(255 * (1 - k));
        b = 255; // синий всегда максимальный
    }
}



// голубой - зеленый
void palette5(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    // t ∈ [0,1]
    if (t < 0.5) {
        double k = t / 0.5;
        // Степень + для более длительного чёрного
        double power = 1.5;
        double bright = pow(k, power);

        r = 0;
        g = (unsigned char)(255 * bright);
        b = (unsigned char)(255 * bright);
    }
    else {
        // От голубого к синему
        double k = (t - 0.5) / 0.5;

        r = 0;
        g = (unsigned char)(255 * (1 - k * 0.6));
        b = (unsigned char)(255 * k);
    }
}

// от черного к голубому (меньше зеленого) от голубого к фиолетовому
void palette6(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    // t ∈ [0,1]
    if (t < 0.5) {
        double k = t / 0.5;
        // Степень + для более длительного чёрного
        double power = 1.5;
        double bright = pow(k, power);

        r = 0;
        g = (unsigned char)(80 * bright); // меньше зеленого 
        b = (unsigned char)(255 * bright);
    }
    else {
        // От голубого к синему
        double k = (t - 0.5) / 0.5;

        r = (unsigned char)(255 * k);
        g = (unsigned char)(80 * (1 - k));  // зелёный падает со 150
        b = 255;
    }
}

// голубой - синий
void palette7(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    // t ∈ [0,1]
    if (t < 0.5) {
        // От чёрного к голубому (cyan)
        double k = t / 0.5;
        // Степень + для более длительного чёрного
        // power - степень для четкости границы между черным и голубым
        double power = 1.5;
        double bright = pow(k, power);

        r = 0;
        g = (unsigned char)(80 * bright);
        b = (unsigned char)(255 * bright);
    }
    else {
        // От голубого к синему
        double k = (t - 0.5) / 0.5;

        r = 0;
        // Голубой компонент (green + blue) убывает до чистого синего
        g = (unsigned char)(80 * (1 - k));
        b = 255; // синий всегда максимальный
    }
}


// белый - оранжевый - красный
void palette8(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    // t ∈ [0,1]
    if (t < 0.5) {
        double k = t / 0.5; // 0>1
        // Степень + для более длительного чёрного
        double power = 1.5;
        double bright = pow(k, power);

        r = 0;
        g = (unsigned char)(255 * bright); //(unsigned char)(255 * (1.0 - bright * 0.5));
        b = (unsigned char)(255 * bright);
    }
    else {
        double k = (t - 0.5) / 0.5; // 0>1

        r = 255;
        g = (unsigned char)(127 * (1.0 - k));
        b = 0; 
    }
}

void palette_violet(double t, unsigned char& r, unsigned char& g, unsigned char& b) {
    // t ∈ [0,1]
    g = 0; // зелёный всегда 0 — иначе уедем в серый

    if (t < 0.5) {
        // Чёрный -> фиолетовый
        double k = t / 0.5;
        double power = 1.5;
        double bright = pow(k, power);

        // R растёт медленнее B — получаем насыщенный фиолетовый, а не серый
        r = (unsigned char)(180 * bright);  // 180 ≈ «фиолетовый» уровень красного
        b = (unsigned char)(255 * bright);
    }
    else {
        // Фиолетовый -> синий
        double k = (t - 0.5) / 0.5;

        r = (unsigned char)(180 * (1 - k)); // R убывает до 0
        b = 255;                            // B всегда максимум
    }

    double brightness = 2;  // 1.0 — как есть, 0.5 — вдвое темнее, 1.5 — ярче
    double saturation = 0.1;

    double luma = 0.299 * r + 0.587 * g + 0.114 * b;
    r = luma + (r - luma) * saturation;
    g = luma + (g - luma) * saturation;
    b = luma + (b - luma) * saturation;

    r = std::clamp((int)(r * brightness), 0, 255);
    g = std::clamp((int)(g * brightness), 0, 255);
    b = std::clamp((int)(b * brightness), 0, 255);
}


void palette_white_cyan_blue(double t, unsigned char& r, unsigned char& g, unsigned char& b)
{
    t = std::clamp(t, 0.0, 1.0);

    const double W_R = 255, W_G = 255, W_B = 255;   // белый
    const double C_R = 0, C_G = 255, C_B = 255;   // голубой
    const double S_R = 0, S_G = 0, S_B = 255;   // синий

    const double mid = 0.7;   // <-- точка голубого (было 0.5)
    // 0.6 — чуть больше белого, 0.75 — много белого

    double R, G, B;
    if (t < mid) {

        double k = t / mid;
        k = std::pow(k, 2.0);
        R = W_R + (C_R - W_R) * k;
        G = W_G + (C_G - W_G) * k;
        B = W_B + (C_B - W_B) * k;
    }
    else {
        double k = (t - mid) / (1.0 - mid);
        R = C_R + (S_R - C_R) * k;
        G = C_G + (S_G - C_G) * k;
        B = C_B + (S_B - C_B) * k;
    }

    r = (unsigned char)std::clamp(R, 0.0, 255.0);
    g = (unsigned char)std::clamp(G, 0.0, 255.0);
    b = (unsigned char)std::clamp(B, 0.0, 255.0);
}


void palette_white_blue(double t, unsigned char& r, unsigned char& g, unsigned char& b)
{
    t = std::clamp(t, 0.0, 1.0);

    double R = 255.0 * std::pow(1.0 - t, 3.0);
    double G = 255.0 * std::pow(1.0 - t, 2.0);
    double B = 255.0;

    r = (unsigned char)std::clamp(R, 0.0, 255.0);
    g = (unsigned char)std::clamp(G, 0.0, 255.0);
    b = (unsigned char)std::clamp(B, 0.0, 255.0);
}


double julia(int px, int py, int width, int height,
    double xmin, double xmax,
    double ymin, double ymax,
    int maxIter, std::complex<double> c) {
    double zx = xmin + (xmax - xmin) * px / (width - 1);
    double zy = ymin + (ymax - ymin) * py / (height - 1);
    std::complex<double> z(zx, zy);

    int iter = 0;
    while (iter < maxIter && std::norm(z) <= 4.0) {
        z = z * z + c;
        ++iter;
    }

    return (double)iter / maxIter;
}

// Итерации для Мандельброта
double mandelbrot(int px, int py, int width, int height,
    double xmin, double xmax,
    double ymin, double ymax,
    int maxIter) {
    double cx = xmin + (xmax - xmin) * px / (width - 1);
    double cy = ymin + (ymax - ymin) * py / (height - 1);
    std::complex<double> c(cx, cy);
    std::complex<double> z(0.0, 0.0);

    int iter = 0;
    while (iter < maxIter && std::norm(z) <= 4.0) {
        z = z * z + c;
        ++iter;
    }

    return (double)iter / maxIter;
}

// Универсальная функция генерации фрактала
void generateFractal(const std::string& filename,int width, int height, int maxIter,double xmin, double xmax,double ymin, double ymax) {

    std::ofstream out(filename);
    out << "P3\n" << width << " " << height << "\n255\n";

    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {

            double t = mandelbrot(px, py, width, height, xmin, xmax, ymin, ymax, maxIter);

            unsigned char r, g, b;

            //palette4(t, r, g, b);
            palette_white_blue(t, r, g, b);
            //palette_violet(t, r, g, b);
            out << (int)r << " " << (int)g << " " << (int)b << " ";
        }
        out << "\n";
    }

    out.close();
    std::cout << "Saved " << filename << std::endl;
}


void feigenbaumFractal(const std::string& filename,
    int width, int height, int maxIter,      // maxIter - количество x значений
    double rmin, double rmax,
    double xmin, double xmax) {

    std::ofstream out(filename);
    out << "P3\n" << width << " " << height << "\n255\n";

    
    std::vector<std::vector<int>> visits(height, std::vector<int>(width, 0)); // кол-во посещений в пикчеле
    std::vector<std::vector<double>> xValues(height, std::vector<double>(width, 0));

    std::cout << "Generation: 0% [";
    for (int i = 0; i < 10; i++)
    {
       std::cout << " ";
    }
    std::cout << "] 100%" << std::endl;

    double iterGeneration = 0.1;

    std::cout << "Generation: 0% [";
    // Для каждого значения r (по горизонтали)
    for (int px = 0; px < width; ++px) {
        double r = rmin + (rmax - rmin) * px / (width - 1);

        // Запускаем несколько начальных условий для полноты картины
        for (int seed = 0; seed < 10; ++seed) {
            double x = (double)seed / 10.0;  // разные начальные условия

            // Игнорируем первые итерации (transient)
            for (int i = 0; i < 500; ++i) {
                x = r * x * (1.0 - x);
            }

            // Записываем аттрактор
            for (int iter = 0; iter < maxIter; ++iter) {
                x = r * x * (1.0 - x);

                // Находим соответствующую y-координату
                int py = (int)((x - xmin) / (xmax - xmin) * (height - 1));
                if (py >= 0 && py < height) {
                    visits[py][px]++;
                    xValues[py][px] = x;
                }
            }
        }
        if(px > width * iterGeneration)
        { 
            std::cout << "#";
            iterGeneration += 0.1;
        }
    }
    std::cout << "#] 100%" << std::endl;


    std::cout << "Painting: 0% [";
    for (int i = 0; i < 10; i++)
    {
        std::cout << " ";
    }
    std::cout << "] 100%" << std::endl;

    iterGeneration = 0.1;

    std::cout << "Painting: 0% [";
    // Отрисовка с использованием палитры
    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {
            unsigned char r, g, b;

            if (visits[py][px] > 0) {
                // количество посещений для яркости обреззаем visits[py][px] кол-во посещений по 
                // шкале от 0.0 до 1.0 на 100 посещений пикселя
                double t = std::min(1.0, std::log(double(visits[py][px]) + 1) / std::log(100.0));
                // Инверсия при наименьших посещениях получаем приблих к 1 знач - более синий, 
                // далее к голубому, при максимальных посещениях 0 - черный
                t = 1.0 - t;
                palette8(t, r, g, b); // power - степень для четкости границы между черным
                // и голубым

                // Добавляем цветовой сдвиг на основе x значения
                double xt = std::abs(xValues[py][px] - 0.5) * 2.0;
                r = (unsigned char)(r * (1 - xt * 0.5));
                g = (unsigned char)(g * (1 - xt * 0.3));
                b = (unsigned char)(b * (1 - xt * 0.7));
            }
            else {
                // Пустые области - чёрный
                r = g = b = 0;
            }

            out << (int)r << " " << (int)g << " " << (int)b << " ";
        }
        out << "\n";
        if (py > height * iterGeneration)
        {
            std::cout << "#";
            iterGeneration += 0.1;
        }
    }
    std::cout << "#] 100%" << std::endl;

    out.close();
    std::cout << "Saved " << filename << "\n";
}


/*
* 
* wallpaper, fix resolution
const int width = 19200;
const int height = 12000;
const int maxIter = 15;

// Область для спирали
//double centerX = -0.7435;
//double centerY = 0.1410;
//double scale = 0.005;
double centerX = -1.3;
double centerY = 0.95;
double scale = 0.5;*/



void startMadelbrot(int width, int height, int maxIter)
{
    // Область для спирали
    //double centerX = -0.7435;
    //double centerY = 0.1410;
    //double scale = 0.005;

    // wallpaper черный низ
    //double centerX = -1.3;
    //double centerY = 0.95;
    //double scale = 0.5;

    // wallpaper черный верх
    double centerX = -1.3;
    double centerY = -0.95;
    double scale = 0.5;

    double xmin = centerX - scale, xmax = centerX + scale;
    double ymin = centerY - scale, ymax = centerY + scale;



    const std::string fileName = "mandelbrot" + std::to_string(rand());
    const std::string outputFilePPM = "Mandelbrott\\" + fileName + ".ppm";
    const std::string outputFilePNG = "Mandelbrott\\" + fileName;

    //  Мандельброт
    generateFractal(outputFilePPM, width, height, maxIter,
        xmin, xmax, ymin, ymax);

    convertPPMtoPNG(outputFilePPM, outputFilePNG);
    
}

struct WindowParamsHight {

    int width = 5760;
    int height = 3600;
    int maxIter = 20;
    //int maxIter = 1000;

};

struct WindowParamsLow {

    int width = 960;
    int height = 600;
    int maxIter = 20;
    //int maxIter = 1000;

};

struct WindowParamsGenerate {

    int width = 320; // 320 640
    int height = 200; // 200 400
    int maxIter = 20;
    //int maxIter = 1000;

};


int main() {
    std::srand(std::time(nullptr));

    const WindowParamsHight windowParamsHight;
    const WindowParamsGenerate windowParamsGenerate;



    time_t now = time(nullptr);

    time_t timeStart = std::time(&now);

    struct tm local_time;
    localtime_s(&local_time, &timeStart);
    std::cout << "Time start: " << std::put_time(&local_time, "%H:%M:%S") << std::endl;

    // генерация
    //startMadelbrot(windowParamsGenerate.width, windowParamsGenerate.height, windowParamsGenerate.maxIter);

    // итоговое фото в высоком разрешении
    startMadelbrot(windowParamsHight.width, windowParamsHight.height, windowParamsHight.maxIter);

    //const std::string fileName = "mandelbrot5874";
    //const std::string outputFilePPM = "Mandelbrott\\" + fileName + ".ppm";
    //const std::string outputFilePNG = "Mandelbrott\\" + fileName;

    //convertPPMtoPNG(outputFilePPM, outputFilePNG);



    // Продвинутый Фейгенбаум с плотностью
    // 
    // double rmin = 2.8, rmax = 4.0, xmin = 0.0, xmax = 1.0;
    // 
    //feigenbaumFractal("Feigenbaum\\feigenbaum_density" + std::to_string(rand()) + ".ppm",
    //    width,
    //    height,
    //    maxIter,
    //    rmin, rmax, xmin, xmax);
    time_t timeEnd = std::time(&now);

    localtime_s(&local_time, &timeEnd);
    std::cout << "Time end: " << std::put_time(&local_time, "%H:%M:%S") << std::endl;

    std::cout << "Time differenve in sec: " << std::to_string(difftime(timeEnd, timeStart)) << std::endl;

    return 0;
}


