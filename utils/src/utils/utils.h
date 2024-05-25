#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include <pthread.h>
#include<readline/readline.h>
#include<commons/collections/queue.h>
#include <semaphore.h>


typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum{
	NEW,
	READY,
	BLOCKED,
	EXEC,
	EXIT
} estado_proceso;

typedef struct
{
	uint8_t AX;
	uint8_t BX;
	uint8_t CX;
	uint8_t DX;
	uint32_t EAX;
	uint32_t EBX;
	uint32_t ECX;
	uint32_t EDX;
	uint32_t SI;
	uint32_t DI;
} RegistrosCPU;

typedef struct
{
	int PID;
	int PC;
	int quantum;
	RegistrosCPU registro;
	estado_proceso estado;
	char* path;
} PCB;

typedef struct
{
	int PID;
	char* path;
} ProcesoMemoria;

//funciones crear conexion
int crear_conexion(char* ip, char* puerto,char* nameServ);
int iniciar_servidor(char* puerto, t_log* un_log, char* msj_server);
int esperar_cliente(int socket_servidor, t_log* un_log, char* msj);
int recibir_operacion(int socket_cliente);

//funciones de paquetes, buffers y envios de mensajes
void crear_buffer(t_paquete* paquete);
void* recibir_buffer(int* size, int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int bytes);
t_paquete *crear_paquete(void);
void paquete(int conexion,char* lo_que_se_desea_enviar);
void agregar_a_paquete(t_paquete* paquete, void* valor, size_t tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_list* recibir_paquete(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

//funciones propias para la serializacion


//handshake y liberar conexiones
void handshakeClient(int fd_servidor, int32_t handshake);
void handshakeServer(int fd_client);
void liberar_conexion(int socket);
void liberar_logger(t_log* logger);
void liberar_config(t_config* config);

#endif /* UTILS_H_ */