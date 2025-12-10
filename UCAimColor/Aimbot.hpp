#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <windows.h>
#include <chrono>
#include <thread>
#include "Stopwatch.hpp"
#include <memory>
#include <mutex> 
#include "Driver.hpp"
Driver::Comms XD;
std::mutex fovMutex; 
int currentFOV = 35; // Inicializar con un valor por defecto válido 
bool key_ativa = false; // Para Aim Assist
bool aimbot_ativa = false; // Para Aimbot (toggle)
#define M_PI 3.141592653589793238462643383279502884197169
int aim_x = 0;
int aim_y = 0;
int Width;
int Height;
int oX, oY;
int silent_x, silent_y;
int fov = 0;
void add_overflow(double Input, double& Overflow)
{
    Overflow = std::modf(Input, &Input) + Overflow;

    if (Overflow > 1.0)
    {
        double Integral{ 0.0 };
        Overflow = std::modf(Overflow, &Integral);
        Input += Integral;
    }
}
static bool InsideCircleTrigger(float centerX, float centerY, float fovX, float fovY, float x, float y)
{
    float minX = centerX - fovX / 2;
    float maxX = centerX + fovX / 2;
    float minY = centerY - fovY / 2;
    float maxY = centerY + fovY / 2;
    return x >= minX && x <= maxX && y >= minY && y <= maxY;
}
float DistanceBetweenCross(float X, float Y)
{
    float ydist = (Y - (Height / 2));
    float xdist = (X - (Width / 2));
    float Hypotenuse = sqrt(pow(ydist, 2) + pow(xdist, 2));
    return Hypotenuse;
}



void Aimbot(int aimX, int aimY, double smooth) {
    static bool keyPressProcessed = false;
    
    // TOGGLE: Activar/desactivar con una pulsación
    if ((GetAsyncKeyState)(cfg::aimkey) & 0x8000) {
        if (!keyPressProcessed) {
            aimbot_ativa = !aimbot_ativa;  // Cambiar estado (ON/OFF)
            keyPressProcessed = true;
        }
    }
    else {
        keyPressProcessed = false;
    }
    
    if(cfg::aimbot_ativo && aimbot_ativa){
        // Verificar que hay un objetivo válido (no 0,0)
        if (aimX == 0 && aimY == 0) {
            return; // No hay objetivo detectado
        }
        
        if (smooth <= 0) return; // Evitar división por cero
        
        std::lock_guard<std::mutex> lock(fovMutex);
        currentFOV = cfg::aimbot_fov;
        
        // BLATANT: Movimiento instantáneo y rápido - sin suavizado
        // OPTIMIZACIÓN MÁXIMA para modo fallback (sin driver)
        bool isFallback = !XD.IsConnected();
        double effectiveSpeed;
        double effectiveSmooth;
        
        if (isFallback) {
            // MODO FALLBACK: Máxima potencia posible sin driver
            effectiveSpeed = cfg::speed * 6.0;  // Velocidad EXTREMA (6.0x) - máximo posible
            effectiveSmooth = 0.3;  // Smooth mínimo absoluto para movimiento instantáneo
        }
        else {
            // Con driver: valores normales
            effectiveSpeed = cfg::speed * 2.0;
            effectiveSmooth = smooth;
        }
        
        // BLATANT: Movimiento completamente INSTANTÁNEO (sin pasos)
        if (effectiveSmooth <= 1.0 || isFallback) {
            // Movimiento DIRECTO e INSTANTÁNEO sin pasos ni delays - MÁXIMA VELOCIDAD
            double totalX = static_cast<double>(aimX) * effectiveSpeed;
            double totalY = static_cast<double>(aimY) * effectiveSpeed;
            
            // En modo fallback, aplicar movimiento directo sin ningún overhead
            if (cfg::recoil_ativo) {
                XD.MouseEvent((int)totalX, (int)totalY + cfg::recoil_offset, Driver::None);
            }
            else {
                XD.MouseEvent((int)totalX, (int)totalY, Driver::None);
            }
        }
        else {
            // Solo si smooth > 1.0, hacer movimiento suavizado (para modo legítimo)
            double totalX = static_cast<double>(aimX) * effectiveSpeed;
            double totalY = static_cast<double>(aimY) * effectiveSpeed;
            
            int numSteps = static_cast<int>(smooth);
            if (numSteps < 1) numSteps = 1;
            
            // Calcular incremento por paso (lineal, no exponencial)
            double stepX = totalX / numSteps;
            double stepY = totalY / numSteps;
            
            double currentX = 0.0;
            double currentY = 0.0;
            
            for (int i = 0; i < numSteps; i++) {
                // Acumular el movimiento hasta este paso
                double targetX = stepX * (i + 1);
                double targetY = stepY * (i + 1);
                
                // Calcular cuanto mover en este paso
                int moveX = static_cast<int>(targetX - currentX);
                int moveY = static_cast<int>(targetY - currentY);
                
                // Solo mover si hay movimiento real
                if (moveX != 0 || moveY != 0) {
                    if (cfg::recoil_ativo) {
                        XD.MouseEvent(moveX, moveY + cfg::recoil_offset, Driver::None);
                    }
                    else {
                        XD.MouseEvent(moveX, moveY, Driver::None);
                    }
                    
                    // Sin delays para máxima velocidad
                    if (!XD.IsConnected() && numSteps > 3 && i < numSteps - 1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                
                currentX = targetX;
                currentY = targetY;
            }
        }
    }
}

void recoil_control()
{
    stopwatch timer;

    while (true)
    {
        if ((GetAsyncKeyState)(cfg::recoil_key) && cfg::recoil_ativo)
        {
            if (timer.get_elapsed() > cfg::time_to_start)
            {
                if (cfg::recoil_offset < cfg::recoil_length)
                {
                    cfg::recoil_offset += cfg::recoil_speed;
                }
                else
                {
                    cfg::recoil_offset = cfg::recoil_length;
                }
            }
        }
        else
        {
            if (cfg::recoil_offset > 0)
            {
                cfg::recoil_offset -= cfg::recoil_speed;
            }
            else
            {
                cfg::recoil_offset = 0;
                timer.update();
            }
        }

        // Resetear recoil cuando el aimbot se desactiva (toggle)
        if (!aimbot_ativa)
        {
            cfg::recoil_offset = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
void Magnet(int aimX, int aimY, double smooth) {
    static bool keyPressProcessed = false; 

    if ((GetAsyncKeyState)(cfg::assist_aimkey) & 0x8000) {
        if (!keyPressProcessed) {
            key_ativa = !key_ativa;
            keyPressProcessed = true; 
        }
    }
    else {
        keyPressProcessed = false; 
    }

    if (cfg::aimassist_ativo && key_ativa) {
        // Verificar que hay un objetivo válido (no 0,0)
        if (aimX == 0 && aimY == 0) {
            return; // No hay objetivo detectado
        }
        
        std::lock_guard<std::mutex> lock(fovMutex);
        currentFOV = cfg::aimassist_fov;
        
        // Ajustar smooth para modo fallback: reducir pasos para más potencia
        double effectiveSmooth = smooth;
        double effectiveSpeed = cfg::assist_speed;
        if (!XD.IsConnected()) {
            double calculatedSmooth = smooth * 0.3;
            effectiveSmooth = (calculatedSmooth > 1.0) ? calculatedSmooth : 1.0;  // BLATANT: Aún más agresivo
            effectiveSpeed = cfg::assist_speed * 2.5;  // BLATANT: Velocidad máxima (2.5x)
        }
        
        // Algoritmo de smooth mejorado: más potente y perceptible
        // Dividir el movimiento total en pasos iguales
        double totalX = static_cast<double>(aimX) * effectiveSpeed;
        double totalY = static_cast<double>(aimY) * effectiveSpeed;
        
        int numSteps = static_cast<int>(effectiveSmooth);
        if (numSteps < 1) numSteps = 1;
        
        // BLATANT: Si smooth es muy bajo, hacer movimiento casi instantáneo
        if (numSteps <= 1) {
            // Movimiento directo sin pasos - máxima potencia
            XD.MouseEvent((int)totalX, (int)totalY, Driver::None);
        }
        else {
            // Calcular incremento por paso (lineal, no exponencial)
            double stepX = totalX / numSteps;
            double stepY = totalY / numSteps;
            
            double currentX = 0.0;
            double currentY = 0.0;
            
            for (int i = 0; i < numSteps; i++) {
                // Acumular el movimiento hasta este paso
                double targetX = stepX * (i + 1);
                double targetY = stepY * (i + 1);
                
                // Calcular cuanto mover en este paso
                int moveX = static_cast<int>(targetX - currentX);
                int moveY = static_cast<int>(targetY - currentY);
                
                // Solo mover si hay movimiento real
                if (moveX != 0 || moveY != 0) {
                    XD.MouseEvent(moveX, moveY, Driver::None);
                    
                    // BLATANT: Sin delays para máxima velocidad
                    // Solo un delay mínimo si está en modo fallback y hay muchos pasos
                    if (!XD.IsConnected() && numSteps > 3 && i < numSteps - 1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                
                currentX = targetX;
                currentY = targetY;
            }
        }
    }
}

