#include "Driver.hpp"
#include <fstream>
#include <filesystem>
#include <vector>
#include <TlHelp32.h>
#include <cmath>
#include <algorithm>

std::string RandomString(const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	std::string tmp_s;
	tmp_s.reserve(len);

	for (int i = 0; i < len; ++i)
	{
		tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return tmp_s;
}

DWORD GetProcessPidByName(const wchar_t* ProcessName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  // Take Snapshot of All Process Running on the System
	if (!hSnapshot || hSnapshot == INVALID_HANDLE_VALUE || hSnapshot == ((HANDLE)(LONG_PTR)ERROR_BAD_LENGTH)) // Check Snapshot Is Invalid
	{
		return 0;
	}

	DWORD Pid;
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(ProcessEntry);
	if (Process32First(hSnapshot, &ProcessEntry)) // Copy First Process of Snapshot and Paste at PROCESSENTRY32 Struct
	{
		while (_wcsicmp(ProcessEntry.szExeFile, ProcessName)) // While Process Names not Same
		{
			if (!Process32Next(hSnapshot, &ProcessEntry)) // Copy The Next Process of the Snapshot and Paste at PROCESSENTRY32 Struct And Check if The Function Worked
			{
				CloseHandle(hSnapshot);
				return 0;
			}
		}

		Pid = ProcessEntry.th32ProcessID; // Found
	}
	else
	{
		CloseHandle(hSnapshot);
		return 0;
	}

	CloseHandle(hSnapshot);
	return Pid;
}

namespace Driver
{
	void Comms::CreateDeviceDrv()
	{
		hDriver = CreateFileA(("\\\\.\\Oykyo"), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
			NULL, NULL);
		// Verificar que el handle sea válido
		if (hDriver == INVALID_HANDLE_VALUE) {
			hDriver = NULL;
		}
	}

	void Comms::TryInitDriver()
	{
		// Cerrar cualquier conexión existente primero
		if (hDriver && hDriver != INVALID_HANDLE_VALUE) {
			CloseHandle(hDriver);
			hDriver = NULL;
		}

		// Intentar conectar primero (por si ya está cargado)
		CreateDeviceDrv();
		if (IsConnected()) {
			return; // Ya está conectado
		}

		// Si no está conectado, ejecutar el mapper
		UDMapper();

		// Esperar un poco más después de que el mapper termine
		Sleep(2000); // 2 segundos iniciales

		// Intentar conectar múltiples veces después del mapper
		// (el driver puede necesitar un momento para inicializarse)
		for (int i = 0; i < 10; i++) {
			CreateDeviceDrv();
			if (IsConnected()) {
				return; // Conectado exitosamente
			}
			Sleep(500); // Esperar 500ms entre intentos
		}
	}

	void Comms::UDMapper()
	{
		try {
			// Obtener la ruta del ejecutable actual
			char exePath[MAX_PATH];
			if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
				return; // Error al obtener la ruta
			}

			// Extraer solo el directorio (sin el nombre del ejecutable)
			std::string exeDir = exePath;
			size_t lastSlash = exeDir.find_last_of("\\/");
			if (lastSlash != std::string::npos) {
				exeDir = exeDir.substr(0, lastSlash + 1);
			}

			// Construir rutas completas a los archivos en el mismo directorio que el ejecutable
			std::string MapperFileName = exeDir + "Mapper.exe";
			std::string DriverFileName = exeDir + "Oykyo.sys";

			// Verificar si los archivos existen usando filesystem (más confiable)
			if (!std::filesystem::exists(MapperFileName)) {
				return; // Si no existe, simplemente retornar sin hacer nada
			}

			if (!std::filesystem::exists(DriverFileName)) {
				return; // Si el driver no existe, no tiene sentido ejecutar el mapper
			}

			PROCESS_INFORMATION ProcessInfo = { 0 };
			STARTUPINFOW StartupInfo = { 0 };
			StartupInfo.cb = sizeof(StartupInfo);
			StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
			StartupInfo.wShowWindow = SW_HIDE;

			// Construir argumentos con solo el nombre del archivo del driver (no la ruta completa)
			// El mapper buscará el archivo en su directorio de trabajo
			std::string driverNameOnly = "Oykyo.sys";
			std::string Start = "-map " + driverNameOnly;
			
			std::wstring wMapperFileName(MapperFileName.begin(), MapperFileName.end());
			std::wstring wExeDir(exeDir.begin(), exeDir.end());
			std::wstring wArgs(Start.begin(), Start.end());
			
			// Crear buffer para los argumentos
			std::vector<wchar_t> argsBuffer(wArgs.length() + 1);
			wcscpy_s(argsBuffer.data(), argsBuffer.size(), wArgs.c_str());

			// Ejecutar el mapper
			if (!CreateProcessW(wMapperFileName.c_str(), argsBuffer.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, wExeDir.c_str(), &StartupInfo, &ProcessInfo))
			{
				// Si falla, simplemente retornar - no es crítico
				return;
			}

			if (ProcessInfo.hProcess != NULL && ProcessInfo.hProcess != INVALID_HANDLE_VALUE) {
				// Esperar a que termine, con timeout para evitar bloqueos
				DWORD waitResult = WaitForSingleObject(ProcessInfo.hProcess, 15000); // 15 segundos timeout
				if (waitResult == WAIT_OBJECT_0) {
					DWORD exitCode = 0;
					if (GetExitCodeProcess(ProcessInfo.hProcess, &exitCode)) {
						// Verificar código de salida (0 = éxito normalmente)
						// No hacemos nada con el código, pero lo obtenemos para debug
					}
					// El sleep después del mapper se hace en TryInitDriver ahora
				}
				else if (waitResult == WAIT_TIMEOUT) {
					// Si el mapper tarda demasiado, terminarlo
					TerminateProcess(ProcessInfo.hProcess, 1);
				}

				if (ProcessInfo.hThread != NULL && ProcessInfo.hThread != INVALID_HANDLE_VALUE)
					CloseHandle(ProcessInfo.hThread);
				if (ProcessInfo.hProcess != NULL && ProcessInfo.hProcess != INVALID_HANDLE_VALUE)
					CloseHandle(ProcessInfo.hProcess);
			}
		}
		catch (...) {
			// Si hay cualquier error, simplemente continuar sin el mapper
			return;
		}
	}

	void Comms::Disconnect()
	{
		if (hDriver != INVALID_HANDLE_VALUE)
			CloseHandle(hDriver);
	}

	bool Comms::MouseEvent(double x, double y, MouseFlags ButtonFlags)
	{
		// Si el driver está disponible, usarlo (método preferido, menos detectable)
		if (hDriver && hDriver != INVALID_HANDLE_VALUE)
		{
			NF_MOUSE_REQUEST MouseRequest;
			MouseRequest.x = (int)x;
			MouseRequest.y = (int)y;
			MouseRequest.ButtonFlags = (int)ButtonFlags;

			return DeviceIoControl(hDriver, IO_SEND_MOUSE_EVENT, &MouseRequest, sizeof(NF_MOUSE_REQUEST), nullptr, NULL, nullptr, nullptr);
		}

		// FALLBACK: SendInput optimizado al MÁXIMO para máxima potencia sin driver
		INPUT input = { 0 };
		input.type = INPUT_MOUSE;
		
		// Optimización: movimiento relativo directo (más rápido que absoluto)
		input.mi.dx = (LONG)x;
		input.mi.dy = (LONG)y;
		
		// Si hay movimiento, agregar flag inmediatamente
		if (x != 0 || y != 0) {
			input.mi.dwFlags = MOUSEEVENTF_MOVE;
		}

		// Manejar botones del mouse (OR bitwise para combinar flags eficientemente)
		if (ButtonFlags & LeftButtonDown)
			input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
		if (ButtonFlags & LeftButtonUp)
			input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
		if (ButtonFlags & RightButtonDown)
			input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
		if (ButtonFlags & RightButtonUp)
			input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
		if (ButtonFlags & MiddleButtonDown)
			input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
		if (ButtonFlags & MiddleButtonUp)
			input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;

		// Enviar inmediatamente sin ningún delay - máxima velocidad posible
		if (input.mi.dwFlags != 0) {
			return SendInput(1, &input, sizeof(INPUT)) == 1;
		}
		
		return false;
	}
}