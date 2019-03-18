// ldid_teleEEG.cpp : Defines the entry point for the console application.
//
/*

	Esta aplicación deberá:

	- Cargar parámetros desde un archivo de configuración:

		* Modo de trabajo:

			0 : Uno o más servidores con dirección ip real fija.
			1 : Uno ó más servidores con dirección ip dinámica (se usa un servidor LDID).

		* Dirección ip real del servidor ldid.
		* Puerto del servicio ldid.
		* Nombre(s) del(os) servidor(es) de la aplicación con quien se debe conectar realmente.
		* Tiempo entre solicitudes ldid.
		* Cantidad de intentos de solicitudes ldid.
		* Puerto del servicio OpenVPN en el servidor.

	En el modo de trabajo 0 no hay que hacer nada con el servidor LDID, por lo tanto la única tarea de la aplicación
	será levantar el servicio OpenVPN, esperar a que haya conexión con el servidor de OpenVPN y luego levantar la aplicación de
	telemedicina.

	En el modo 1 la aplicación tiene que resolver primero la(s) direccion(es) IP del(os) servidor(res) conectándose con el
	servidor LDID. Los pasos a seguir serán:

	- Conectarse con el servidor ldid y solicitarle la dirección ip del(os) servidor(es) de aplicación.
	- Esperar un tiempo prudencial por la respuesta, si no llega repetir solicitud un número determinado de veces,
	  si sigue sin llegar la respuesta avisar al usuario del error.
	- Si la respuesta llega entonces chequear que OpenVPN esté inactivo (como servicio), si está activo pararlo.
	- Modificar el archivo de configuración de OpenVPN con la(s) direccion(es) IP del(os) servidor(es) y el puerto.
	- Levantar el servicio OpenVPN.
	- Esperar a que haya conexión con el servidor de OpenVPN.
	- Levantar la aplicación de telemedicina.

*/

#include "stdafx.h"
#include "ldid_teleEEG.h"

//UINT sockMsg = RegisterWindowMessageA("WM_SOCKET");

//void WINAPI RasDialFunc(UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError)
//	WSAAsyncSelect


UINT crea_archivo(wchar_t* nombre_archivo,UINT cantidad_bytes_buffer,int nivel,wchar_t* extension)
{
	errno_t err;
	FILE* debug;
	void* temp_file;
	void* nivel_texto;
	UINT resultado = 0;

	if (nivel == 100) return 1; // 100 búsquedas recursivas del nombre adecuado es un buen límite.

	temp_file = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,cantidad_bytes_buffer);
	nivel_texto  = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,20);

	StringCchCopy((wchar_t*)temp_file,cantidad_bytes_buffer/2,(wchar_t*)nombre_archivo);
	if (nivel > 0)
	{
		wcscat_s((wchar_t*)temp_file,cantidad_bytes_buffer/2,L"_");
		_itow_s(nivel,(wchar_t*)nivel_texto,10,10);
		wcscat_s((wchar_t*)temp_file,cantidad_bytes_buffer/2,(wchar_t*)nivel_texto);
	}
	wcscat_s((wchar_t*)temp_file,cantidad_bytes_buffer/2,(wchar_t*)extension);

	err  = _wfopen_s(&debug, (wchar_t*)temp_file, L"r" );	// Esto es para ver si ya existía un archivo log con
															// ese mismo nombre.
	if( err == 0 )
	{
		// Un archivo con ese nombre existe, por lo tanto llamar a la función recursivamente aumentando el nivel.
		fclose(debug);
		if (crea_archivo(nombre_archivo,cantidad_bytes_buffer,(nivel+1),extension)) resultado = 1;
		else resultado = 0;
	}
	else if (err == 2)
	{
		// El archivo no existía previamente, por lo tanto puedo usarlo.
		if (!_wfopen_s(&debug, (wchar_t*)temp_file, L"w" ))	// Crear el archivo con el nombre adecuado.
		{
			fclose(debug);
			StringCchCopy((wchar_t*)nombre_archivo,cantidad_bytes_buffer/2,(wchar_t*)temp_file);
			resultado = 0;
		}
		else resultado = 1;
	}
	else
	{
		resultado = 1;	// Error que no es de que el archivo no existía, por lo tanto retornar con código de error 1.
	}

	if (temp_file != NULL)
	{
		HeapFree(ProccessHeap, 0, temp_file);
		temp_file = NULL;
	}
	if (nivel_texto != NULL)
	{
		HeapFree(ProccessHeap, 0, nivel_texto);
		nivel_texto = NULL;
	}
	return resultado;
}

void escribir_debug(LPTSTR p_text, void* p_text1, FILE* file,bool second_integer)
{
	// Antes de hacer ninguna operación con el archivo debug al que se va a escribir debo chequear que este está abierto.
	// (Falta por hacer esto)

	time_t tiempo_debug;
	struct tm* p_tiempo_local = new struct tm;
	//FILE* file;
	void* integer_texto;
	integer_texto = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,100);

	//_wfopen_s( &file, archivo, L"a+" );

	// Aquí debo calcular el tamaño del buffer en función del texto que se envió a la función, para evitar que
	//se pase. Después poner el argumento del medio de wcscat_s al tamaño del buffer en caracteres.

	wchar_t buffer[2048];

	time(&tiempo_debug);
	localtime_s(p_tiempo_local,(time_t *)&tiempo_debug);

	int temp = p_tiempo_local->tm_year + 1900;

	SecureZeroMemory(integer_texto,100);
	_itow_s(temp,(wchar_t*)integer_texto,50,10);
	StringCchCopy((wchar_t*)buffer,2048,(wchar_t*)integer_texto);

	wcscat_s(buffer, L"/");

	temp = p_tiempo_local->tm_mon + 1;

	SecureZeroMemory(integer_texto,100);
	_itow_s(temp,(wchar_t*)integer_texto,50,10);
	wcscat_s(buffer, (wchar_t*)integer_texto);
	wcscat_s(buffer, L"/");

	SecureZeroMemory(integer_texto,100);
	_itow_s(p_tiempo_local->tm_mday,(wchar_t*)integer_texto,50,10);
	wcscat_s(buffer, (wchar_t*)integer_texto);
	wcscat_s(buffer, L" ");

	SecureZeroMemory(integer_texto,100);
	_itow_s(p_tiempo_local->tm_hour,(wchar_t*)integer_texto,50,10);
	wcscat_s(buffer, (wchar_t*)integer_texto);
	wcscat_s(buffer, L":");

	SecureZeroMemory(integer_texto,100);
	_itow_s(p_tiempo_local->tm_min,(wchar_t*)integer_texto,50,10);
	wcscat_s(buffer, (wchar_t*)integer_texto);
	wcscat_s(buffer, L":");

	SecureZeroMemory(integer_texto,100);
	_itow_s(p_tiempo_local->tm_sec,(wchar_t*)integer_texto,50,10);
	wcscat_s(buffer, (wchar_t*)integer_texto);
	wcscat_s(buffer, L" : ");

	if (lstrcmp( p_text, L"") != 0) wcscat_s(buffer,(wchar_t*)p_text);
	if (second_integer)
	{
		SecureZeroMemory(integer_texto,100);
		_itow_s(*(int*)p_text1,(wchar_t*)integer_texto,50,10);
		wcscat_s(buffer, (wchar_t*)integer_texto);
	}
	else wcscat_s(buffer, (wchar_t*)p_text1);
	wcscat_s(buffer, L"\r\n");

	fputws(buffer,file);
	fflush(file);
	//fclose(file);
	delete p_tiempo_local;
	if (integer_texto != NULL)
	{
		HeapFree(ProccessHeap, 0, integer_texto);
		integer_texto = NULL;
	}
	return;
}

void salida_visual(string p_text, bool error)
{
	cout << p_text;

	cout << "\n";
	if (error)
	{
		char entrada;
		cout << "Verifique el archivo log mas actual para mayores detalles del error...\n";
		cout << "Si persiste el problema contacte al servicio tecnico...\n";
		cout << "Puede cerrar esta ventana...\n";
		cin >> entrada;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	cout << "################################################################################";
	cout << "    ESTABLECIENDO COMUNICACION CON EL SERVIDOR DE LA APLICACION DE TELEEEG\n";
	cout << "################################################################################";
	cout << "\n";
	cout << "No cierre esta ventana hasta que se le indique...\n";
	Sleep(5000);

	ProccessHeap = GetProcessHeap();
	XHandle = NULL;

//*********************************************************************************************************
// Inicialización del archivo que se va a usar para registrar los eventos (el debug)
//*********************************************************************************************************

	void* integer_en_texto;
	integer_en_texto = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,100);

	debug_file_tx = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,100);

	time(&tiempo);
	p_tiempo = new struct tm;
	localtime_s(p_tiempo,(time_t *)&tiempo);

	_itow_s(p_tiempo->tm_year + 1900,(wchar_t*)integer_en_texto,50,10);
	StringCchCopy((wchar_t*)debug_file_tx,50,(wchar_t*)integer_en_texto);

	wcscat_s((wchar_t*)debug_file_tx,50, L"_");

	SecureZeroMemory(integer_en_texto,100);
	_itow_s((p_tiempo->tm_mon + 1),(wchar_t*)integer_en_texto,50,10);
	wcscat_s((wchar_t*)debug_file_tx,50,(wchar_t*)integer_en_texto);

	wcscat_s((wchar_t*)debug_file_tx,50, L"_");


	SecureZeroMemory(integer_en_texto,100);
	_itow_s((p_tiempo->tm_mday),(wchar_t*)integer_en_texto,50,10);
	wcscat_s((wchar_t*)debug_file_tx,50, (wchar_t*)integer_en_texto);

	wcscat_s((wchar_t*)debug_file_tx,50, L"_");


	SecureZeroMemory(integer_en_texto,100);
	_itow_s((p_tiempo->tm_hour),(wchar_t*)integer_en_texto,50,10);
	wcscat_s((wchar_t*)debug_file_tx,50, (wchar_t*)integer_en_texto);

	wcscat_s((wchar_t*)debug_file_tx,50, L"_");


	SecureZeroMemory(integer_en_texto,100);
	_itow_s(p_tiempo->tm_min,(wchar_t*)integer_en_texto,50,10);
	wcscat_s((wchar_t*)debug_file_tx,50, (wchar_t*)integer_en_texto);


	delete p_tiempo;
	if (integer_en_texto != NULL)
	{
		HeapFree(ProccessHeap, 0, integer_en_texto);
		integer_en_texto = NULL;
	}

	if (crea_archivo((wchar_t*)debug_file_tx,100,0,L".log"))
	{
		// No se pudo crear el archivo debug por alguna razón, esto debe señalizarse al usuario por un bitmap de error.
		salida_visual("No se pudo crear el archivo log...", true);
		return 1;
	}
	else
	{
		// Se creo el archivo debug, seguir adelante con las inicializaciones.

		_wfopen_s(&debug_file,(wchar_t*)debug_file_tx, L"a+" );	// Abrir el archivo debug en modo append.

		//escribir_debug(L"",L"Archivo debug de la aplicación abierto correctamente",debug_file,false);


		// Inicialización de las variables que contendrán los parámetros de trabajo.

		modo = 0;
		rango = 0;
		Servidor_LDID = NULL;
		Puerto_LDID = 0;
		Cantidad_servidores = 0;
		arreglo_nombres_servidores = NULL;
		tiempo_ldid = 0;
		intentos_ldid = 0;
		puerto_ovpn = 0;
		Path_ovpn = NULL;
		Path_teleEEG = NULL;
		al_menos_uno = false;

		if (lee_parameters())
		{
			escribir_debug(L"",L"Hay un problema con el archivo de configuración",debug_file,false);
			salida_visual("Hay un problema con el archivo de configuracion...", true);
			return 1;
		}
		else
		{
			//escribir_debug(L"",L"Variables del sistema leidas correctamente desde el archivo de configuración",debug_file,false);
			wVersionRequested = MAKEWORD(2, 2);

			err = WSAStartup(wVersionRequested, &wsaData);
			if (err != 0)
			{
				/* Tell the user that we could not find a usable */
				/* Winsock DLL.                                  */
				//printf("WSAStartup failed with error: %d\n", err);
				escribir_debug(L"",L"WSAStartup failed",debug_file,false);
				salida_visual("Fallo en la inicializacion del subsistema de conexion a redes de Windows...", true);
				return 1;
			}

			/* Confirm that the WinSock DLL supports 2.2.*/
			/* Note that if the DLL supports versions greater    */
			/* than 2.2 in addition to 2.2, it will still return */
			/* 2.2 in wVersion since that is the version we      */
			/* requested.                                        */

			if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
			{
				/* Tell the user that we could not find a usable */
				/* WinSock DLL.                                  */
				//printf("Could not find a usable version of Winsock.dll\n");
				escribir_debug(L"",L"Could not find a usable version of Winsock.dll",debug_file,false);
				salida_visual("No se puede encontrar un version adecuada de la biblioteca Winsock.dll...", true);
				WSACleanup();
				return 1;
			}
			//else	escribir_debug(L"",L"The Winsock 2.2 dll was found okay",debug_file,false);

			if (modo == 1)
			{
				ch_convertidos = new size_t;
				iFamily = AF_INET;
				iType = SOCK_DGRAM;
				iProtocol = IPPROTO_UDP;

				sock = socket(iFamily, iType, iProtocol);
				if (sock == INVALID_SOCKET)
				{
					GSALastError = WSAGetLastError();
					escribir_debug(L"la creación del socket LDID falló con el número de error: ",(int*)&GSALastError,debug_file,true);
					salida_visual("Fallo en la creacion del socket para comunicaciones ldid...", true);
					return 1;
				}
				else
				{
					//escribir_debug(L"",L"socket function succeeded para LDID.",debug_file,false);
					tamano_buffer = 224;
					recvbuf = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY, tamano_buffer);
					comando = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,24);
					datos = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,200);
					P_buffer_temp = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,448);

					IP_virtual_servidor = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,34);
					SecureZeroMemory(&clientService, sizeof(clientService));

					wcstombs_s(ch_convertidos,(char*)IP_virtual_servidor,34,(wchar_t*)Servidor_LDID,_TRUNCATE);
					//escribir_debug(L"conectándose al servidor LDID: ",(LPTSTR)Servidor_LDID, debug_file,false);
					clientService.sin_family = AF_INET;
					clientService.sin_addr.s_addr = inet_addr((char*)IP_virtual_servidor);
					clientService.sin_port = htons((u_short)Puerto_LDID);

					buffer_enviar = (char*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,250);
					fubb = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,500);

					NewEvent = WSACreateEvent();
					WSAEventSelect(sock, NewEvent, FD_READ);
					DWORD resultado;

					for (int conta_serv=0;conta_serv<Cantidad_servidores;conta_serv++)
					{
						SecureZeroMemory(fubb,500);
						SecureZeroMemory(buffer_enviar,250);
						StringCchCopy((wchar_t*)fubb,250,L"###SOLICITUD_DIRECCION:");
						wcscat_s((wchar_t*)fubb,250,(wchar_t*)arreglo_nombres_servidores[conta_serv]);
						wcstombs_s((size_t*)ch_convertidos,(char*)buffer_enviar,250,(wchar_t*)fubb,_TRUNCATE);
						escribir_debug(L"Resolviendo la dirección del servidor: ",(wchar_t*)arreglo_nombres_servidores[conta_serv],debug_file,false);
						recibida_respuesta = false;
						intento = 0;

						while ((recibida_respuesta == false) && (intento < intentos_ldid))
						{
							int b = sendto(sock,(char*)buffer_enviar,strlen((char*)buffer_enviar)+1,0,(SOCKADDR *)&clientService, sizeof(clientService));
							intento++;
							//escribir_debug(L"Este es el intento: ",(int*)&intento,debug_file,true);
							if (b == SOCKET_ERROR)
							{
								GSALastError = WSAGetLastError();
								escribir_debug(L"send() para LDID failed with error code: ",(int*)&GSALastError,debug_file,true);
								salida_visual("Fallo en el envio de la solicitud ldid...", false);
								//Sleep(1000);
							}
							else
							{
								escribir_debug(L"",(wchar_t*)L"Enviada una solicitud al servidor LDID",debug_file,false);
								salida_visual("Enviada una solicitud al servidor LDID...", false);
								Sleep(1000);

								while (1)
								{
									resultado = WSAWaitForMultipleEvents(1, &NewEvent, FALSE,(tiempo_ldid*1000), FALSE);

									if (resultado == WSA_WAIT_TIMEOUT)
									{
										escribir_debug(L"",L"Time-out",debug_file,false);
										salida_visual("Tiempo agotado esperando respuesta del servidor LDID...", false);
										break;
									}
									else
									{
										addrlen = sizeof(remoteaddr);
										SecureZeroMemory(recvbuf,tamano_buffer);
										SecureZeroMemory(&remoteaddr,addrlen);
										returnStatus = recvfrom(sock, (char*)recvbuf, tamano_buffer, 0,(struct sockaddr*)&remoteaddr, &addrlen);
										if (returnStatus == SOCKET_ERROR)
										{
											GSALastError = WSAGetLastError();
											escribir_debug(L"Se recibió el error tratando de contactar al servidor LDID: ",(int*)&GSALastError,debug_file,true);
											salida_visual("Recibido un error tratando de contactar al servidor LDID...", false);
											break;
										}
										else if (htonl(remoteaddr.sin_addr.s_addr) == htonl(clientService.sin_addr.s_addr))
										{
											SecureZeroMemory((char*)comando,24);
											strncpy_s((char*)comando,24,(char*)recvbuf,23);
											SecureZeroMemory((char*)datos,200);
											strncpy_s((char*)datos,200,(char*)((char*)recvbuf+23),returnStatus-23);
											SecureZeroMemory((wchar_t*)P_buffer_temp,448);
											mbstowcs_s(ch_convertidos, (wchar_t*)P_buffer_temp, 224, (char*)datos, _TRUNCATE);
											comp_string = lstrcmpA((char*)comando,"###DIRECCION_SERVIDOR&:");
											if (comp_string == 0)
											{
												escribir_debug(L"Recibida la dirección IP del servidor de la aplicación desde el servidor LDID: ",(wchar_t*)P_buffer_temp,debug_file,false);
												salida_visual("Recibida la direccion IP del servidor de la aplicacion desde el servidor LDID...", false);
												if (arreglo_IP_servidores[conta_serv] != NULL) HeapFree(ProccessHeap, 0, arreglo_IP_servidores[conta_serv]);
												arreglo_IP_servidores[conta_serv] = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,600);
												StringCchCopy((wchar_t*)arreglo_IP_servidores[conta_serv],300,L"remote ");
												wcscat_s((wchar_t*)arreglo_IP_servidores[conta_serv],300,(wchar_t*)P_buffer_temp);
												wcscat_s((wchar_t*)arreglo_IP_servidores[conta_serv],300,L" ");
												wcscat_s((wchar_t*)arreglo_IP_servidores[conta_serv],300,(wchar_t*)puerto_ovpn);
												//escribir_debug(L"línea a escribir en openvpn: ",(wchar_t*)arreglo_IP_servidores[conta_serv],debug_file,false);
												wcscat_s((wchar_t*)arreglo_IP_servidores[conta_serv],300,L"\n");
												recibida_respuesta = true;
												al_menos_uno = true;

											}
											else
											{
												comp_string = lstrcmpA((char*)comando,"###SERV_NO_REGISTRADO&&");
												if (comp_string == 0)
												{
													escribir_debug(L"No está registrado el servidor",(wchar_t*)P_buffer_temp,debug_file,false);
													salida_visual("No esta registrado el servidor...", false);
													if (arreglo_IP_servidores[conta_serv] != NULL) HeapFree(ProccessHeap, 0, arreglo_IP_servidores[conta_serv]);
													arreglo_IP_servidores[conta_serv] = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,600);
													StringCchCopy((wchar_t*)arreglo_IP_servidores[conta_serv],300,L";remote ");
													wcscat_s((wchar_t*)arreglo_IP_servidores[conta_serv],300,(wchar_t*)P_buffer_temp);
													//escribir_debug(L"línea a escribir en openvpn: ",(wchar_t*)arreglo_IP_servidores[conta_serv],debug_file,false);
													wcscat_s((wchar_t*)arreglo_IP_servidores[conta_serv],300,L" \r");
													recibida_respuesta = true;
												}
												else
												{
													escribir_debug(L"",L"Se recibió un comando incorrecto desde el servidor LDID",debug_file,false);
													salida_visual("Recibido un comando incorrecto desde el servidor LDID...", false);
												}
											}
											break;
										}
										escribir_debug(L"",L"Se recibió respuesta desde una localización desconocida",debug_file,false);
										salida_visual("Recibida una respuesta desde una localizacion desconocida...", false);
									}
								}
							}
						}
					}

				}
				WSACloseEvent(NewEvent);
				shutdown(sock,2);
				closesocket(sock);
				//Sleep(1000);

			}

			//DeleteTimerQueueTimer(NULL, m_timerHandle, NULL);
			//CloseHandle (m_timerHandle);
			else if (modo == 0)
			{
				for (int i=0;i<Cantidad_servidores;i++)
				{
					if (arreglo_IP_servidores[i] != NULL) HeapFree(ProccessHeap, 0, arreglo_IP_servidores[i]);
					arreglo_IP_servidores[i] = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,600);
					StringCchCopy((wchar_t*)arreglo_IP_servidores[i],300,L"remote ");
					wcscat_s((wchar_t*)arreglo_IP_servidores[i],300,(wchar_t*)arreglo_nombres_servidores[i]);
					wcscat_s((wchar_t*)arreglo_IP_servidores[i],300,L" ");
					wcscat_s((wchar_t*)arreglo_IP_servidores[i],300,(wchar_t*)puerto_ovpn);
					//escribir_debug(L"línea a escribir en openvpn: ",(wchar_t*)arreglo_IP_servidores[i],debug_file,false);
					wcscat_s((wchar_t*)arreglo_IP_servidores[i],300,L" \r");
					al_menos_uno = true;
				}
			}

			if (al_menos_uno)
			{
				UINT result_mod_ovpn;
				result_mod_ovpn = modifica_archivo_OpenVPN((wchar_t*) Path_ovpn, (wchar_t**) arreglo_IP_servidores);

				if (result_mod_ovpn == 1)
				{
					salida_visual("Error modificando el archivo de configuracion de openVPN...", true);
					return 1;
				}
				if (Stop_service(L"OpenVPNService"))
				{
					escribir_debug(L"",L"Error deteniendo el servicio openVPN",debug_file,false);
					salida_visual("Error deteniendo el servicio openVPN...", true);
					return 1;
				}
				Sleep(5000);

				if (Check_service(L"OpenVPNService"))
				{
					escribir_debug(L"",L"Error iniciando el servicio openVPN",debug_file,false);
					salida_visual("Error iniciando el servicio openVPN...", true);
					return 1;
				}

				bool aparecio = false;
				int contador_aparecio = 0;
				while (aparecio == false)
				{
					aparecio = chequea_Address();
					Sleep(1000);
					contador_aparecio++;
					if (contador_aparecio == 180)
					{
						escribir_debug(L"",L"error estableciendo la VPN",debug_file,false);
						salida_visual("Error estableciendo la VPN...", true);
						Stop_service(L"OpenVPNService");
						return 1;
					}
				}
				escribir_debug(L"",L"Establecida la VPN",debug_file,false);
				salida_visual("Establecida la VPN...", false);
				salida_visual("Invocando al agente de la aplicacion de telemedicina...", false);
				Sleep(3000);
				if (XHandle == NULL)
				{
					fclose(debug_file);
					WSACleanup();
					XHandle = _wspawnl(_P_NOWAIT,(wchar_t*)Path_teleEEG,(wchar_t*)Path_teleEEG,NULL);
					//XHandle = _wspawnl(_P_NOWAIT,L"agent.exe",L"agent.exe",NULL);

					if ((XHandle == NULL)	|| (XHandle == -1))
					{
						_wfopen_s(&debug_file,(wchar_t*)debug_file_tx, L"a+" );
						int error;
						_get_errno(&error);
						escribir_debug(L"",(int*)&error,debug_file,true);
						escribir_debug(L"",L"error iniciando el agente de la aplicacion de telemedicina",debug_file,false);
						salida_visual("Error iniciando el agente de la aplicacion de telemedicina...", true);
						XHandle = NULL;
						Stop_service(L"OpenVPNService");
						return 1;
					}
				}
			}
			else
			{
				escribir_debug(L"",L"No hay servidores activos para la aplicacion de telemedicina",debug_file,false);
				salida_visual("No hay servidores activos para la aplicacion de telemedicina...", true);
				return 1;
			}
		}
	}
	//escribir_debug(L"",L"Cerrando archivo debug",debug_file,false);
	//fclose(debug_file);


	return 0;
}

UINT lee_parameters()
//*********************************************************************************************************************************************************
//*********************************************************************************************************************************************************
// Esta función lee los parámetros de configuración desde el archivo en hdd, retorna 0 si todo fue ok, en caso contrario retorna 1.

/*		Parámetros:

		* Modo de trabajo:

			0 : Uno o más servidores con dirección ip real fija.
			1 : Uno o más servidores con dirección ip dinámica (se usa un servidor LDID).

		* rango de trabajo de la vpn.
		* Dirección ip real del servidor ldid.
		* Puerto del servicio ldid.
		* Nombre del servidor de la aplicación con quien se debe conectar realmente.
		* Tiempo entre solicitudes ldid.
		* Cantidad de intentos de solicitudes ldid.
		* Puerto del servicio OpenVPN en el servidor.

		modo = 0;
		rango = 0;
		Servidor_LDID = NULL;
		Puerto_LDID = 0;
		Cantidad_servidores = 0;
		arreglo_nombres_servidores = NULL;
		tiempo_ldid = 0;
		intentos_ldid = 0;
		puerto_ovpn = 0;
		Path_ovpn = NULL;
		Path_teleEEG = NULL;


		*/

//*********************************************************************************************************************************************************
{
	wchar_t* modo_str = NULL;
	wchar_t* rango_str = NULL;
	wchar_t* Puerto_LDID_str = NULL;
	wchar_t* Cantidad_servidores_str = NULL;
	wchar_t* tiempo_ldid_str = NULL;
	wchar_t* intentos_ldid_str = NULL;
	wchar_t* puerto_ovpn_str = NULL;

	//escribir_debug(L"",L"entré en lee_parameters()",debug_file,false);
	myfile.open("settings.txt");
	if (myfile.is_open())
	{
		modo_str = busca_valor_settings(L"Modo",false,&myfile);
		if (modo_str == NULL)
		{
			myfile.close();
			return 1;
		}
		swscanf_s((wchar_t*)modo_str, L"%d", &modo); //swscanf_s lee la información correctamente formateada (%d es como decimal)

		rango_str = busca_valor_settings(L"rango",false,&myfile);
		if (rango_str == NULL)
		{
			myfile.close();
			return 1;
		}
		swscanf_s((wchar_t*)rango_str, L"%d", &rango);

		if (modo == 1)
		{

			Servidor_LDID = busca_valor_settings(L"Servidor_LDID",false,&myfile);
			if (Servidor_LDID == NULL)
			{
				myfile.close();
				return 1;
			}
			Puerto_LDID_str = busca_valor_settings(L"Puerto_LDID",false,&myfile);
			if (Puerto_LDID_str == NULL)
			{
				myfile.close();
				return 1;
			}
			swscanf_s((wchar_t*)Puerto_LDID_str, L"%d", &Puerto_LDID);

			tiempo_ldid_str = busca_valor_settings(L"tiempo_ldid",false,&myfile);
			if (tiempo_ldid_str == NULL)
			{
				myfile.close();
				return 1;
			}
			swscanf_s((wchar_t*)tiempo_ldid_str, L"%d", &tiempo_ldid);

			intentos_ldid_str = busca_valor_settings(L"intentos_ldid",false,&myfile);
			if (intentos_ldid_str == NULL)
			{
				myfile.close();
				return 1;
			}
			swscanf_s((wchar_t*)intentos_ldid_str, L"%d", &intentos_ldid);
		}

		Cantidad_servidores_str = busca_valor_settings(L"Cantidad_servidores",false,&myfile);
		if (Cantidad_servidores_str == NULL)
		{
			myfile.close();
			return 1;
		}
		swscanf_s((wchar_t*)Cantidad_servidores_str, L"%d", &Cantidad_servidores);

		arreglo_nombres_servidores = (wchar_t**)calloc(Cantidad_servidores,sizeof(wchar_t*));
		arreglo_IP_servidores = (wchar_t**)calloc(Cantidad_servidores,sizeof(wchar_t*));
		void* nombre_servidor;
		nombre_servidor = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,100);
		void* numero_servidor_str;
		numero_servidor_str = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,100);

		for(int i=0;i<Cantidad_servidores;i++)

		{
			SecureZeroMemory(nombre_servidor,100);
			SecureZeroMemory(numero_servidor_str,100);
			StringCchCopy((wchar_t*)nombre_servidor,50,L"nombre_servidor");
			_itow_s(i,(wchar_t*)numero_servidor_str,50,10);
			wcscat_s((wchar_t*)nombre_servidor,50,(wchar_t*)numero_servidor_str);

			arreglo_nombres_servidores[i] = busca_valor_settings((wchar_t*)nombre_servidor,false,&myfile);
			arreglo_IP_servidores[i] = NULL;
			if (arreglo_nombres_servidores[i] == NULL)
			{
				myfile.close();
				return 1;
			}
		}

		puerto_ovpn = busca_valor_settings(L"puerto_ovpn",false,&myfile);
		if (puerto_ovpn == NULL)
		{
			myfile.close();
			return 1;
		}

		UINT return_status;
		HKEY hkResult;
		HKEY hkResult1;
		HKEY hkResult2;
		DWORD cbData=2000;

		RegOpenKeyEx(HKEY_LOCAL_MACHINE,NULL,0,KEY_QUERY_VALUE,&hkResult);
		RegOpenKeyEx(hkResult,L"SOFTWARE",0,KEY_QUERY_VALUE,&hkResult1);

		Path_ovpn = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,500);
		return_status = RegOpenKeyEx(hkResult1,L"OpenVPN",0,KEY_QUERY_VALUE,&hkResult2);
		if ( return_status != ERROR_SUCCESS)
		{
			escribir_debug(L"",L"No se encuentra la clave de registro OpenVPN",debug_file,false);
			//salida_visual("No se encuentra la clave de registro OpenVPN", true);
			myfile.close();
			return 1;
		}

		return_status = RegQueryValueEx(hkResult2,L"config_dir",NULL,NULL,(LPBYTE)Path_ovpn,&cbData);
		if ( return_status != ERROR_SUCCESS)
		{
			escribir_debug(L"",L"No se encuentra el valor config_dir de la clave de registro OpenVPN",debug_file,false);
			//salida_visual("No se encuentra el valor config_dir de la clave de registro OpenVPN", true);
			myfile.close();
			return 1;
		}
		wcscat_s((wchar_t*)Path_ovpn,250,L"\\teleEEG.ovpn");
		RegCloseKey(hkResult2);
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult);

		Path_teleEEG = busca_valor_settings(L"Path_teleEEG",false,&myfile);
		if (Path_teleEEG == NULL)
		{
			myfile.close();
			return 1;
		}

		myfile.close();

		//escribir_debug(L"",L"Parámetros leídos correctamente desde el archivo de configuración",debug_file,false);

		escribir_debug(L"modo de trabajo de la aplicación: ",(wchar_t*)modo_str,debug_file,false);
		escribir_debug(L"rango en que se establecerá la vpn: ",(wchar_t*)rango_str,debug_file,false);

//*******************************************************************************************************************

		if (modo == 1)
		{
			escribir_debug(L"Servidor_LDID: ",(wchar_t*)Servidor_LDID,debug_file,false);
			escribir_debug(L"Puerto_LDID: ",(wchar_t*)Puerto_LDID_str,debug_file,false);
			escribir_debug(L"tiempo_ldid: ",(wchar_t*)tiempo_ldid_str,debug_file,false);
			escribir_debug(L"intentos_ldid: ",(wchar_t*)intentos_ldid_str,debug_file,false);

		}

//********************************************************************************************************************
		escribir_debug(L"Cantidad_servidores: ",(wchar_t*)Cantidad_servidores_str,debug_file,false);
		for(int i=0;i<Cantidad_servidores;i++)
		{
			escribir_debug(L"servidor: ",(wchar_t*)arreglo_nombres_servidores[i],debug_file,false);
		}
		escribir_debug(L"Path_ovpn: ",(wchar_t*)Path_ovpn,debug_file,false);
		escribir_debug(L"Path_teleEEG: ",(wchar_t*)Path_teleEEG,debug_file,false);
		escribir_debug(L"puerto_ovpn: ",(wchar_t*)puerto_ovpn,debug_file,false);
		return 0;
	}
	else
	{
		return 1;
	}
}

wchar_t* busca_valor_settings(wchar_t* nombre_variable,bool allow_blank, wifstream* file)
{
	int compare = 0;						// Variable que se usa para las comparaciones.
	void* var_temp = NULL;					// puntero a cualquier tipo de datos (void), se inicializa en 0 (NULL).
	void* variable = NULL;					// puntero a cualquier tipo de datos (void), se inicializa en 0 (NULL).
	wchar_t* comienzo_variable = NULL;		// puntero a tipo de datos wchar_t (caracteres dobles, para datos de tipo Unicode)
	wchar_t* comienzo_valor = NULL;			// puntero a tipo de datos wchar_t (caracteres dobles, para datos de tipo Unicode)
	wchar_t* final_valor = NULL;			// puntero a tipo de datos wchar_t (caracteres dobles, para datos de tipo Unicode)
	wchar_t* comentario_en_linea = NULL;	// puntero a tipo de datos wchar_t (caracteres dobles, para datos de tipo Unicode)
	size_t temp = 0;						// variable de tipo size_t (enteros)
	size_t nombre_variable_size = 0;		// variable de tipo size_t (enteros)

	// Asignar un bloque de memoria de 5120 bytes desde el Heap del proceso y colocar el puntero var_temp apuntando al comienzo.
	// El bloque de memoria se llena de ceros como inicialización.

	var_temp = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,5120);

	file->seekg(0);							// Pone el puntero de lectura del archivo al principio.

	while (file->good())					// El ciclo entre corchetes se ejecutará mientras se estén leyendo líneas desde el
	{										// archivo
		file->getline((wchar_t*)var_temp,2560,'\n');	// Lee la línea completa (hasta 2560 caracteres) hacia el buffer a donde
														// apunta var_temp, el caracter \n (nueva línea) no se incluye en la
														// cadena de caracteres leida. Esta función leerá la cantidad de caracteres
														// que se le instruye (2560) o hasta que encuentre el caracter delimitador
														// (en este caso \n), o hasta el final del archivo, la primera condición
														// que se cumpla.

		// Busca el primer caracter en la línea que no sea espacio, ese será el comienzo de la variable.

		comienzo_variable = _wcsspnp((wchar_t*)var_temp,L" ");	// Devuelve un puntero al primer caracter que no esté contenido
																// en el set de caracteres de comparación, en este caso " ", si
																// no hay caracteres que cumplan la condición devuelve NULL.

		if (comienzo_variable != NULL)	// Si el resultado de la función anterior es distinto de NULL es que hay algo en la línea
										// que no sean caracteres espacio.
		{
			compare = wcsncmp((wchar_t*)comienzo_variable,L";",1);	// Compara el primer caracter de la cadena de caracteres
																	// con el caracter ";" (delimitador de comentario)

			if (compare != 0)	// Si el resultado de la comparación anterior es distinto de cero es que la línea no es un comentario
			{
				// Halla el tamaño del nombre de la variable que se pasó como parámetro a la función
				nombre_variable_size = wcslen((wchar_t*)nombre_variable);

				// Compara los primeros caracteres de la variable encontrada y de la variable que se está buscando.
				compare = wcsncmp((wchar_t*)comienzo_variable,(wchar_t*)nombre_variable,nombre_variable_size);

				if (compare == 0)	// si son iguales es que esta es la variable.
				{
				// Busca el primer caracter a partir de donde termina el nombre de la variable que no sea espacio,
				// ese será el comienzo del valor.

					comienzo_valor = _wcsspnp((wchar_t*)(comienzo_variable+nombre_variable_size),L" ");
					if (comienzo_valor != NULL)		// Hay caracteres distintos al espacio en la línea, por lo tanto hay un valor.
					{
						temp = wcslen((wchar_t*)comienzo_valor);
						comentario_en_linea = wcschr((wchar_t*)comienzo_valor,';');

						if (comentario_en_linea != NULL)
						{
							SecureZeroMemory(comentario_en_linea,((char*)comentario_en_linea+(wcslen((wchar_t*)comentario_en_linea)*2))-(char*)comentario_en_linea);
						}

						if (temp != 0)	//Si el tamaño de la cadena de caracteres no es 0.
						{
							final_valor = comienzo_valor;
							wchar_t* final_valor_anterior = NULL;

							while (final_valor != NULL)
							{
								final_valor_anterior = final_valor;
								final_valor = _wcsspnp((wchar_t*)final_valor+1,L" \0");
							}
							wcscpy_s((wchar_t*) final_valor_anterior + 1,1,L"\0");
							variable = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,(temp+1)*2);
							wcscpy_s((wchar_t*) variable,(temp+1),(wchar_t*)comienzo_valor);
							//escribir_debug(L"variable: ",(wchar_t*)variable,debug_file,false);
						}
						else if (allow_blank == 0)
						{
							escribir_debug(L"No puede estar en blanco el valor: ",(wchar_t*)nombre_variable,debug_file,false);
							//salida_visual("El valor de la variable no puede estar en blanco", false);
							//salida_visual("Verifique el archivo log mas actual para mayores detalles del error", true);
							HeapFree(ProccessHeap, 0, var_temp);
							var_temp = NULL;
							return NULL;
						}
					}
					else if (allow_blank != 0)	// Se permite que el valor de la variable sea nulo.
					{
						variable = (wchar_t*)HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,2);
						wcscpy_s((wchar_t*) variable,1,L"");
					}
					else if (allow_blank == 0)
					{
						escribir_debug(L"No puede estar en blanco el valor: ",(wchar_t*)nombre_variable,debug_file,false);
						//salida_visual("El valor de la variable no puede estar en blanco", false);
						//salida_visual("Verifique el archivo log mas actual para mayores detalles del error", false);
						HeapFree(ProccessHeap, 0, var_temp);
						var_temp = NULL;
						return NULL;
					}
					break;
				}
			}
		}
	}

	if (variable == NULL) escribir_debug(L"No se encuentra la variable: ",(wchar_t*)nombre_variable,debug_file,false);
	//salida_visual("No se encuentra una variable del archivo de configuracion", false);
	//salida_visual("Verifique el archivo log mas actual para mayores detalles del error", false);

	HeapFree(ProccessHeap, 0, var_temp);
	var_temp = NULL;
	return (wchar_t*)variable;
}

UINT modifica_archivo_OpenVPN(wchar_t* archivo, wchar_t** arreglo_lineas_nuevas)
{
	FILE *stream;
	if( _wfopen_s( &stream, archivo, L"r t" ) != 0 )
	{
		escribir_debug(L"",L"error abriendo el archivo de configuración de OpenVPN para lectura",debug_file,false);
		//salida_visual("Error abriendo el archivo de configuración de OpenVPN para lectura", true);
		return 1;
	}
	else
	{
		escribir_debug(L"",L"se abrió el archivo de configuración de OpenVPN para lectura",debug_file,false);
		wchar_t* buffer = new wchar_t[500];
		wchar_t** arreglo_lineas = new wchar_t*[500];
		for (UINT i=0;i<500;i++)
		{
			arreglo_lineas[i] = new wchar_t[100];
			SecureZeroMemory((char*)arreglo_lineas[i],200);
		}
		UINT contador = 0;
		wchar_t* comienzo_valor = NULL;
		size_t cantidad_caracteres_valor = 0;
		wchar_t* delimitadores = new wchar_t[4];
		StringCchCopy(delimitadores,4,L" \r\n");
		wchar_t* variable;
		int compa = 0;
		int compa1 = 0;


		while (fgetws((wchar_t*)buffer, 1000,stream) != NULL)
		{
			comienzo_valor = _wcsspnp(buffer,delimitadores);		// Busca en la línea de texto el primer caracter que no sea espacio retorno o nueva línea,
																	// si no hay ningún caracter que no sean estos devuelve NULL.

			if (comienzo_valor != NULL)								// Si se encontró algún caracter entonces es que hay una variable en esta línea.
			{
				cantidad_caracteres_valor = wcscspn(comienzo_valor,delimitadores);	// Busca, a partir de donde empezó el valor de la variable definida en
																					// la línea, la primera aparición de espacio, retorno o nueva línea,
																					// lo que demarca la terminación del valor.
				variable = new wchar_t[cantidad_caracteres_valor+4];
				SecureZeroMemory((char*)variable,(cantidad_caracteres_valor+4)*2);
				wcsncpy_s(variable,cantidad_caracteres_valor+4,comienzo_valor,cantidad_caracteres_valor);
			}

			compa = lstrcmpW((wchar_t*)variable,L"remote");
			compa1 = lstrcmpW((wchar_t*)variable,L";remote");
			//if (compa == 0)	StringCchCopy((wchar_t*)arreglo_lineas[contador],100,(wchar_t*)linea_nueva);
			//else StringCchCopy((wchar_t*)arreglo_lineas[contador],100,(wchar_t*)buffer);
			if ((compa != 0) && (compa1 != 0))	StringCchCopy((wchar_t*)arreglo_lineas[contador],100,(wchar_t*)buffer);
			contador++;
		}
		fclose(stream);
		if( _wfopen_s( &stream, archivo, L"w t" ) != 0 )
		{
			escribir_debug(L"",L"error abriendo el archivo de configuración de OpenVPN para escritura",debug_file,false);
			//salida_visual("Error abriendo el archivo de configuración de OpenVPN para escritura", true);
			return 1;
		}
		else
		{
			escribir_debug(L"",L"se abrió el archivo de configuración de OpenVPN para escritura",debug_file,false);
			for (UINT i= 0;i<contador;i++)
			{
				fputws(arreglo_lineas[i],stream);
			}
			for (int i=0;i<Cantidad_servidores;i++)
			{
				fputws(arreglo_lineas_nuevas[i],stream);
				escribir_debug(L"Se escribió en el archivo TeleEEG.ovpn la línea: ",(wchar_t*)arreglo_lineas_nuevas[i],debug_file,false);
				salida_visual("Actualizado el archivo de configuracion de openvpn con los datos adecuados...", false);
			}
			fclose(stream);
		}
	}
	return 0;
}

UINT Check_service(wchar_t* servicio)
{
	SC_HANDLE schSCManager;
    SC_HANDLE schService;
	SERVICE_STATUS_PROCESS ssStatus;
	SERVICE_STATUS ser_Status;
	DWORD dwOldCheckPoint;
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	DWORD LastError;
	/*La secuencia general de trabajo sería:

	1- Chequear que el servicio está instalado
	2- Si está instalado chequear si está activo o no
	3- Si no está activo hacerlo.
	4- Retornar con el código de retorno correcto.

	Se debe chequear si el servicio está configurado para arranque manual, sino usar ChangeServiceConfig para establecerlo de esa manera, para la próxima vez.*/

	schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
	{
		LastError = GetLastError();
		escribir_debug(L"OpenSCManager failed with error code: ",(DWORD*)&LastError,debug_file,true);
		//salida_visual("Fallo en el procedimiento de apertura del manipulador de servicios", true);
		return 1;
	}
	else
	{
		escribir_debug(L"",L"OpenSCManager success",debug_file,false);
		schService = OpenService(schSCManager,(wchar_t*)servicio,SERVICE_ALL_ACCESS);
		if (schService == NULL)
		{
			LastError = GetLastError();
			escribir_debug(L"OpenService failed with error code: ",(DWORD*)&LastError,debug_file,true);
			//salida_visual("Fallo en el procedimiento de apertura del servicio openvpn", true);
			CloseServiceHandle(schSCManager);
			return 1;
		}
		else
		{
			escribir_debug(L"",L"OpenService success",debug_file,false);
			if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
			{
				LastError = GetLastError();
				escribir_debug(L"QueryServiceStatusEx(1) failed with error code: ",(DWORD*)&LastError,debug_file,true);
				//salida_visual("Fallo en la adquisicion del estado del servicio openvpn", true);
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return 1;
			}
			else
			{
				switch(ssStatus.dwCurrentState)
				{
				case SERVICE_STOPPED:						//arrancar el servicio.
					if (!StartService(schService,0,NULL))
					{
						LastError = GetLastError();
						escribir_debug(L"StartService failed with error code: ",(DWORD*)&LastError,debug_file,true);
						//salida_visual("Fallo en el arranque del servicio openvpn", true);
						CloseServiceHandle(schService);
						CloseServiceHandle(schSCManager);
						return 1;
					}
					else
					{
						escribir_debug(L"",L"Service start pending...",debug_file,false);
						// Check the status until the service is no longer start pending.

						if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
						{
							LastError = GetLastError();
							escribir_debug(L"QueryServiceStatusEx failed with error code: ",(DWORD*)&LastError,debug_file,true);
							//salida_visual("Fallo en la adquisicion del estado del servicio openvpn", true);
							CloseServiceHandle(schService);
							CloseServiceHandle(schSCManager);
							return 1;
						}

						// Save the tick count and initial checkpoint.

						dwStartTickCount = GetTickCount();
						dwOldCheckPoint = ssStatus.dwCheckPoint;

						while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
						{
							// Do not wait longer than the wait hint. A good interval is
							// one-tenth the wait hint, but no less than 1 second and no
							// more than 10 seconds.

							dwWaitTime = ssStatus.dwWaitHint / 10;

							if( dwWaitTime < 1000 )
								dwWaitTime = 1000;
							else if ( dwWaitTime > 10000 ) dwWaitTime = 10000;
							Sleep( dwWaitTime );

							// Check the status again.

							if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
							{
								LastError = GetLastError();
								escribir_debug(L"QueryServiceStatusEx failed with error code: ",(DWORD*)&LastError,debug_file,true);
								//salida_visual("Fallo en la adquisicion del estado del servicio openvpn", true);
								CloseServiceHandle(schService);
								CloseServiceHandle(schSCManager);
								return 1;
							}
							if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
							{
								// Continue to wait and check.

								dwStartTickCount = GetTickCount();
								dwOldCheckPoint = ssStatus.dwCheckPoint;
							}
							else
							{
								if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
								{
									// No progress made within the wait hint.
									break;
								}
							}
						}

						// Determine whether the service is running.

						if (ssStatus.dwCurrentState == SERVICE_RUNNING)
						{
							escribir_debug(L"",L"Service started successfully.",debug_file,false);
							CloseServiceHandle(schService);
							CloseServiceHandle(schSCManager);
							return 0;
						}
						else
						{
							escribir_debug(L"",L"Service not started.",debug_file,false);
							//salida_visual("No se pudo iniciar el servicio openvpn", true);
							escribir_debug(L"Current State: ",(DWORD*)&ssStatus.dwCurrentState,debug_file,true);
							escribir_debug(L"Exit Code: ",(DWORD*)&ssStatus.dwWin32ExitCode,debug_file,true);
							escribir_debug(L"Check Point: ",(DWORD*)&ssStatus.dwCheckPoint,debug_file,true);
							escribir_debug(L"Wait Hint: ",(DWORD*)&ssStatus.dwWaitHint,debug_file,true);
							CloseServiceHandle(schService);
							CloseServiceHandle(schSCManager);
							return 1;
						}
					}
					break;
				case SERVICE_RUNNING:						//Es servicio ya está corriendo, no hay necesidad de arrancarlo.
					escribir_debug(L"",L"The service is already running",debug_file,false);
					//salida_visual("El servicio ya estaba ejecutandose", false);
					CloseServiceHandle(schService);
					CloseServiceHandle(schSCManager);
					return 0;
					break;
				case SERVICE_PAUSED:						//Enviarle el código para que salga de la pausa.
					/*
					Hay que ver la validez de este estado, pues en las pruebas no aparece la posibilidad de darle pausa al servicio uvnc
					En caso de que se pueda hacer hay que ver si el mecanismo de espera después de haberle mandado el código SERVICE_CONTROL_CONTINUE
					al servicio para que salga del estado de pausa es correcto, pues simplemente copié el mecanismo de SERVICE_STOPPED
					*/

					if (!ControlService(schService,SERVICE_CONTROL_CONTINUE,&ser_Status))
					{
						LastError = GetLastError();
						escribir_debug(L"ControlService failed with error code: ",(DWORD*)&LastError,debug_file,true);
						//salida_visual("Fallo en el control del servicio", false);
						CloseServiceHandle(schService);
						CloseServiceHandle(schSCManager);
						return 1;
					}
					else
					{
						escribir_debug(L"",L"Service start pending...",debug_file,false);
						// Check the status until the service is no longer start pending.

						if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
						{
							LastError = GetLastError();
							escribir_debug(L"QueryServiceStatusEx failed with error code: ",(DWORD*)&LastError,debug_file,true);
							//salida_visual("Fallo en la adquisicion del estado del servicio openvpn", false);
							CloseServiceHandle(schService);
							CloseServiceHandle(schSCManager);
							return 1;
						}

						// Save the tick count and initial checkpoint.

						dwStartTickCount = GetTickCount();
						dwOldCheckPoint = ssStatus.dwCheckPoint;

						while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
						{
							// Do not wait longer than the wait hint. A good interval is
							// one-tenth the wait hint, but no less than 1 second and no
							// more than 10 seconds.

							dwWaitTime = ssStatus.dwWaitHint / 10;

							if( dwWaitTime < 1000 )
								dwWaitTime = 1000;
							else if ( dwWaitTime > 10000 ) dwWaitTime = 10000;
							Sleep( dwWaitTime );

							// Check the status again.

							if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
							{
								LastError = GetLastError();
								escribir_debug(L"QueryServiceStatusEx failed with error code: ",(DWORD*)&LastError,debug_file,true);
								//salida_visual("Fallo en la adquisicion del estado del servicio openvpn", false);
								CloseServiceHandle(schService);
								CloseServiceHandle(schSCManager);
								return 1;
							}
							if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
							{
								// Continue to wait and check.

								dwStartTickCount = GetTickCount();
								dwOldCheckPoint = ssStatus.dwCheckPoint;
							}
							else
							{
								if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
								{
									// No progress made within the wait hint.
									break;
								}
							}
						}

						// Determine whether the service is running.

						if (ssStatus.dwCurrentState == SERVICE_RUNNING)
						{
							escribir_debug(L"",L"Service started successfully.",debug_file,false);
							CloseServiceHandle(schService);
							CloseServiceHandle(schSCManager);
							return 0;
						}
						else
						{
							escribir_debug(L"",L"Service not started.",debug_file,false);
							//salida_visual("Fallo en la ejecucion del servicio openvpn", false);
							escribir_debug(L"Current State: ",(DWORD*)&ssStatus.dwCurrentState,debug_file,true);
							escribir_debug(L"Exit Code: ",(DWORD*)&ssStatus.dwWin32ExitCode,debug_file,true);
							escribir_debug(L"Check Point: ",(DWORD*)&ssStatus.dwCheckPoint,debug_file,true);
							escribir_debug(L"Wait Hint: ",(DWORD*)&ssStatus.dwWaitHint,debug_file,true);
							CloseServiceHandle(schService);
							CloseServiceHandle(schSCManager);
							return 1;
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
	CloseServiceHandle(schSCManager);
	return 1;
}

UINT Stop_service(wchar_t* servicio)
{
	SC_HANDLE schSCManager;
    SC_HANDLE schService;
	SERVICE_STATUS_PROCESS ssStatus;
	//SERVICE_STATUS ser_Status;
	//DWORD dwOldCheckPoint;
    //DWORD dwStartTickCount;
    DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	DWORD LastError;
	
	
	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out


	schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
	{
		LastError = GetLastError();
		escribir_debug(L"OpenSCManager failed with error code: ",(DWORD*)&LastError,debug_file,true);
		return 1;
	}
	else 
	{
		escribir_debug(L"",L"OpenSCManager for close success",debug_file,false);
		schService = OpenService(schSCManager,(wchar_t*)servicio,SERVICE_ALL_ACCESS);
		if (schService == NULL)
		{ 
			LastError = GetLastError();
			escribir_debug(L"OpenService failed with error code: ",(DWORD*)&LastError,debug_file,true);
			CloseServiceHandle(schSCManager);
			return 1;
		}
		escribir_debug(L"",L"OpenService for close success",debug_file,false);
		/*
		
		Aquí debo configurar el servicio para arranque manual.
		
		*/

		if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
		{
			LastError = GetLastError();
			escribir_debug(L"QueryServiceStatusEx(1) failed with error code: ",(DWORD*)&LastError,debug_file,true);
			CloseServiceHandle(schService); 
			CloseServiceHandle(schSCManager);
			return 1;
		}
		if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
		{
			escribir_debug(L"",L"Service is already stopped.",debug_file,false);
			CloseServiceHandle(schService); 
			CloseServiceHandle(schSCManager);
			return 0;
		}

		// If a stop is pending, wait for it.

		while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) 
		{
			escribir_debug(L"",L"Service stop pending...",debug_file,false);
			
			// Do not wait longer than the wait hint. A good interval is 
			// one-tenth of the wait hint but not less than 1 second  
			// and not more than 10 seconds. 

			dwWaitTime = ssStatus.dwWaitHint / 10;
			if( dwWaitTime < 1000 )	dwWaitTime = 1000;
			else if ( dwWaitTime > 10000 ) dwWaitTime = 10000;

			Sleep(dwWaitTime);

			if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
			{
				LastError = GetLastError();
				escribir_debug(L"QueryServiceStatusEx(2) failed with error code: ",(DWORD*)&LastError,debug_file,true);
				CloseServiceHandle(schService); 
				CloseServiceHandle(schSCManager);
				return 1;
			}
			if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
			{
				escribir_debug(L"",L"Service stopped successfully.",debug_file,false);
				CloseServiceHandle(schService); 
				CloseServiceHandle(schSCManager);
				return 0;
			}
			if ( GetTickCount() - dwStartTime > dwTimeout )
			{
				escribir_debug(L"",L"Service stop timed out.",debug_file,false);
				CloseServiceHandle(schService); 
				CloseServiceHandle(schSCManager);
				return 1;
			}
		}
		
		// If the service is running, dependencies must be stopped first. (esto no lo implementé)

		// Send a stop code to the service.

		if ( !ControlService(schService,SERVICE_CONTROL_STOP,(LPSERVICE_STATUS) &ssStatus ) )  // ¿ ssStatus o ser_Status ?
		{
			LastError = GetLastError();
			escribir_debug(L"ControlService failed with error code: ",(DWORD*)&LastError,debug_file,true);
			CloseServiceHandle(schService); 
			CloseServiceHandle(schSCManager);
			return 1;
		}

		// Wait for the service to stop.

		while (ssStatus.dwCurrentState != SERVICE_STOPPED) 
		{
			Sleep(ssStatus.dwWaitHint);

			if (!QueryServiceStatusEx(schService,SC_STATUS_PROCESS_INFO,(LPBYTE) &ssStatus,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded ))
			{
				LastError = GetLastError();
				escribir_debug(L"QueryServiceStatusEx(3) failed with error code: ",(DWORD*)&LastError,debug_file,true);
				CloseServiceHandle(schService); 
				CloseServiceHandle(schSCManager);
				return 1;
			}
			if (ssStatus.dwCurrentState == SERVICE_STOPPED)		//Esto está extraño dentro de un while.
				break;
			if ( GetTickCount() - dwStartTime > dwTimeout )
			{
				escribir_debug(L"",L"Wait timed out",debug_file,false);
				CloseServiceHandle(schService); 
				CloseServiceHandle(schSCManager);
				return 1;
			}
		}
		escribir_debug(L"",L"Service stopped successfully.",debug_file,false);
	}
	return 0;
}

bool chequea_Address()
{
// En esta función tengo que parametrizar los rangos de direcciones que se chequean, que son los de las posibles VPNs en cada rango de direcciones privadas
// permitidos, esto debe ser un parámetro del sistema, que salga del archivo de configuración, como lo tiene OpenVPN, al final quien establece la VPN es
// ese software, por lo que el rango de valores posibles está establecido, es simplemente traer esa información para acá.

	char name[255];
	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;
	unsigned char* temp = new unsigned char[4];
	UINT temp1;
	SecureZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;

	size_t*  ch_convertidosss = new size_t;
	void* P_buffer_temppp = HeapAlloc(ProccessHeap, HEAP_ZERO_MEMORY,100);
	struct sockaddr_in const *sin;

	if( gethostname ( name, sizeof(name)) == 0)
	{
		if( getaddrinfo( name,"",&hints,&result) == 0)
		{
			for(ptr=result; ptr != NULL ;ptr=ptr->ai_next)
			{
				if( ptr->ai_family == AF_INET)
				{
					sin = (struct sockaddr_in*)ptr->ai_addr;

					mbstowcs_s(ch_convertidosss, (wchar_t*)P_buffer_temppp, 50,(char*)(inet_ntoa(sin->sin_addr)), _TRUNCATE);
					//escribir_debug(L"direccion ip: ",(wchar_t*)P_buffer_temppp,debug_file,false);

					for(int i=0; i<4; i++)
					{
						temp[i] = *((ptr->ai_addr->sa_data)+i+2);
					}
					// Esta formula funciona con las direcciones ip (aaa.bbb.ccc.ddd) de la siguiente manera:
					// temp1 = aaa * 16777216 + bbb* 65536 + ccc*256 + ddd

					temp1 = (temp[0]*16777216)+(temp[1]*65536)+(temp[2]*256)+temp[3];
					switch (rango)
					{
					case 1:
						if ((temp1 >= 167772161) && (temp1 <= 184549374)) 	// 10.0.0.1 - 10.255.255.254
						{
							//escribir_debug(L"direccion ip de la VPN: ",(wchar_t*)P_buffer_temppp,debug_file,false);
							if (P_buffer_temppp != NULL)
							{
								HeapFree(ProccessHeap, 0, P_buffer_temppp);
								P_buffer_temppp = NULL;
							}
							//freeaddrinfo(result);
							//freeaddrinfo(&hints);
							return true;
						}
						break;
					case 2:
						if ((temp1 >= 2886729729) && (temp1 <=  2887778302))	// 172.16.0.1 - 172.31.255.254
						{
							escribir_debug(L"direccion ip de la VPN: ",(wchar_t*)P_buffer_temppp,debug_file,false);
							if (P_buffer_temppp != NULL)
							{
								HeapFree(ProccessHeap, 0, P_buffer_temppp);
								P_buffer_temppp = NULL;
							}
							//freeaddrinfo(result);
							//freeaddrinfo(&hints);
							return true;
						}
						break;
					case 3:
						if ((temp1 >= 3232235521) && (temp1 <= 3232301054))	// 192.168.0.1 - 192.168.255.254
						{
							escribir_debug(L"direccion ip de la VPN: ",(wchar_t*)P_buffer_temppp,debug_file,false);
							if (P_buffer_temppp != NULL)
							{
								HeapFree(ProccessHeap, 0, P_buffer_temppp);
								P_buffer_temppp = NULL;
							}
							//freeaddrinfo(result);
							//freeaddrinfo(&hints);
							return true;
						}
						break;
					default:
						break;
					}
				}
			}
		}
	}
	if (P_buffer_temppp != NULL)
	{
		HeapFree(ProccessHeap, 0, P_buffer_temppp);
		P_buffer_temppp = NULL;
	}
	//freeaddrinfo(result);
	//freeaddrinfo(&hints);
	return false;
}


