#include <stdio.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <istream>
#include <string>

#define WIN32_LEAN_AND_MEAN
//#include <stdio.h>
#include <windows.h>
//#include <ras.h>
//#include <raserror.h>
#include <string.h>
#include <winbase.h>
#include <time.h>
#include <stdlib.h>
#include <Strsafe.h>
#include <Winsvc.h>

//#include "comandos.h"

#include <fcntl.h>
#include <io.h>
#include <wchar.h>
#include <Winreg.h>
#include <process.h>
//#include "Error_msg.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Rasapi32.lib")
#pragma comment(lib, "advapi32.lib")

using namespace std;

// Variables

WORD wVersionRequested;
WSADATA wsaData;
int err;

HANDLE ProccessHeap;

void* debug_file_tx;
FILE* debug_file;
time_t tiempo;			// Estas dos variables son para determinar el momento en que se inicia la aplicación y 
struct tm* p_tiempo;	// generar el nombre del archivo debug.

wifstream myfile;	// Objeto stream que en donde se cargará el archivo de configuración.

int modo;
int rango;
void* Servidor_LDID;
int Puerto_LDID;
int Cantidad_servidores;
wchar_t** arreglo_nombres_servidores;
wchar_t** arreglo_IP_servidores;
int tiempo_ldid;
int intentos_ldid;
void* puerto_ovpn;
void* Path_ovpn;
void* Path_teleEEG;
bool al_menos_uno;

int GSALastError;
size_t* ch_convertidos;

SOCKET sock;				// Socket para establecer la comunicación con el software de control en el servidor VPN.
//SOCKET sock_lb;
int iFamily;				// Familia de protocolo Internet para el socket para establecer la comunicación con el software de control en el servidor VPN.
int iType;					// Tipo de socket que se va a establecer.
int iProtocol;				// Protocolo usado en el establecimiento de la comunicación.
int tamano_buffer;			// Tamaño del buffer recvbuf.
void* recvbuf;				// Buffer donde se reciben los datos llegados desde el servidor.
void* comando;				// Aquí se coloca el comando que se extrae del mensaje enviado por el servidor.
void* datos;				// Aquí se colocan los datos del comando que se extrae del mensaje enviado por el servidor.
void* IP_virtual_servidor;
struct sockaddr_in clientService;		// Estructura que se pasa a la función sendto() con los parámetros del servidor (dirección, puerto, etc.)
void* buffer_enviar;					// En este buffer se copian los datos que se van a enviar al servidor con la función sendto().
wchar_t* fubb;
bool recibida_respuesta;
sockaddr_in remoteaddr;				// Aqui se guarda la direccion de quien envio el comando que se recibe en el puerto 18.
socklen_t addrlen;					// Esta variable se usa para almacenar el tamaño de remoteaddr, es necesaria pues se debe
									// pasar como puntero su direccion a recvfrom.
int returnStatus;
int comp_string;						// Variable usada para comparar strings.
void* P_buffer_temp;
int intento;

//HANDLE m_timerHandle;
WSAEVENT NewEvent;
WSANETWORKEVENTS events;
intptr_t XHandle;

// funciones

void escribir_debug(LPTSTR p_text, void* p_text1, FILE* file,bool second_integer);
void salida_visual(string p_text, bool error);
UINT crea_archivo(wchar_t* nombre_archivo,UINT cantidad_bytes_buffer,int nivel,wchar_t* extension);
UINT lee_parameters();
wchar_t* busca_valor_settings(wchar_t* nombre_variable,bool allow_blank, wifstream* file);
UINT modifica_archivo_OpenVPN(wchar_t* archivo, wchar_t** arreglo_lineas_nuevas);
UINT Check_service(wchar_t* servicio);
bool chequea_Address();
UINT Stop_service(wchar_t* servicio);
//void CALLBACK TimerProc(void* lpParametar,BOOLEAN TimerOrWaitFired);
//void QueueTimerHandler();
