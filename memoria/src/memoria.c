#include <stdlib.h>
#include <stdio.h>
#include <memoria.h>

int main(int argc, char* argv[]) {
	
	

	//LISTA DE PROCESOS
	listProcesos = list_create();
	listStdin = list_create();
	listStdout = list_create();
	listDialfs = list_create();

	sem_init(&protect_list_procesos,0,1);

	memoria_logger = log_create(".//tp.log", "log_cliente", true, LOG_LEVEL_INFO);
	if (memoria_logger == NULL)
	{
		perror("Algo paso con el log. No se pudo crear.");
		exit(EXIT_FAILURE);
	}

memoria_logs_obligatorios = log_create(".//logs_obligatorios.log", "logs", true, LOG_LEVEL_INFO);

if (memoria_logs_obligatorios == NULL)
{
    perror("Algo paso con el log_Obligatorio. No se pudo crear.");
	exit(EXIT_FAILURE);
}

	//t_config *memoria_config = config_create("/home/utnso/tp-2024-1c-Grupo-120/memoria/src/memoria.config");
	t_config *memoria_config = config_create("src/memoria.config");
	if (memoria_config == NULL)
	{
		perror("Error al crear el config.");
		exit(EXIT_FAILURE);
	}
    PUERTO_ESCUCHA = config_get_string_value (memoria_config , "PUERTO_ESCUCHA" );
    TAM_MEMORIA = config_get_int_value (memoria_config , "TAM_MEMORIA" );
    TAM_PAGINA = config_get_int_value (memoria_config , "TAM_PAGINA" );
    PATH_INSTRUCCIONES = config_get_string_value (memoria_config , "PATH_INSTRUCCIONES" );
    RETARDO_RESPUESTA = config_get_int_value (memoria_config , "RETARDO_RESPUESTA" );


    log_info(memoria_logger, "PUERTO_ESCUCHA: %s", PUERTO_ESCUCHA);
    log_info(memoria_logger, "TAM_MEMORIA: %i", TAM_MEMORIA);
    log_info(memoria_logger, "TAM_PAGINA: %i", TAM_PAGINA);
    log_info(memoria_logger, "PATH_INSTRUCCIONES: %s", PATH_INSTRUCCIONES);
    log_info(memoria_logger, "RETARDO_RESPUESTA: %i", RETARDO_RESPUESTA);


	espacio_usuario = malloc(TAM_MEMORIA);
	if (espacio_usuario == NULL) 
	{
		perror("Error al asignar memoria");
    	return 1;
	}

	//LISTA DE MARCOS
	listMarcos = generarListaDeMarcos(listMarcos);

	//inicia servidor memoria
	fd_memoria = iniciar_servidor (PUERTO_ESCUCHA, memoria_logger, "INICIADA LA MEMORIA");

	//espero conexion cpu
	log_info (memoria_logger, "Esperando a conectar con CPU.");
	fd_cpu = esperar_cliente (fd_memoria, memoria_logger, "CPU");
	handshakeServer(fd_cpu);


	//espero conexion kernel
	log_info (memoria_logger, "Esperando a conectar con Kernel.");
	fd_kernel = esperar_cliente (fd_memoria, memoria_logger, "KERNEL");
	handshakeServer(fd_kernel);

	/*
	//espero conexion entradasalida
	log_info (memoria_logger, "Esperando a conectar con EntradaSalida.");
	fd_entradasalida = esperar_cliente (fd_memoria, memoria_logger, "ENTRADASALIDA");
	handshakeServer(fd_entradasalida);
	*/

	//escuchar mensajes de cpu
	pthread_t hilo_cpu;
	pthread_create (&hilo_cpu, NULL, (void*)memoria_escuchar_cpu, NULL);
	pthread_detach (hilo_cpu);

	//enviar a cpu TAM_PAGINA
	int* tam_pag = malloc(sizeof(int));
	*tam_pag = TAM_PAGINA;
	enviarEntero(tam_pag,fd_cpu,TAMPAGINA);
	
	/*
	//escuchar mensajes de entradasalida
	pthread_t hilo_entradasalida;
	pthread_create (&hilo_entradasalida, NULL, (void*)memoria_escuchar_entradasalida, NULL);
	pthread_detach (hilo_entradasalida);
	*/

	//escuchar mensajes de kernel
	pthread_t hilo_kernel;
	pthread_create (&hilo_kernel, NULL, (void*)memoria_escuchar_kernel, NULL);
	pthread_detach (hilo_kernel);

	//escuchar entradas y salidas
	pthread_t hilo_io;
	pthread_create(&hilo_io, NULL, (void*)escuchar_io,NULL);
	pthread_detach (hilo_io);

	while(1)
	{
		sleep(1);
	}

	liberar_config(memoria_config);
	liberar_logger(memoria_logger);
	liberar_conexion(fd_memoria);
	liberar_conexion(fd_cpu);
	liberar_conexion(fd_kernel);
	liberar_conexion(fd_entradasalida);

	free(espacio_usuario);
	free(tam_pag);
	return (EXIT_SUCCESS);
}