#include <stdlib.h>
#include <stdio.h>
#include <kernel.h>

int main(int argc, char* argv[]) {

sem_init(&sem, 0, 1); // EM en la cola new
sem_init(&sem_ready, 0, 1);
sem_init(&sem_cant,0,0); // el productor es el inicial_proceso
sem_init(&sem_cant_ready,0,0);



kernel_logger = log_create(".//tp.log", "log_cliente", true, LOG_LEVEL_INFO);
	if (kernel_logger == NULL)
	{
		perror("Algo paso con el log. No se pudo crear.");
		exit(EXIT_FAILURE);
	}

//cambiar la ruta del archivo config a una abreviatura
	kernel_config = config_create("src/kernel.config");
	if (kernel_config == NULL)
	{
		perror("Error al crear el config.");
		exit(EXIT_FAILURE);
	}

    PUERTO_ESCUCHA = config_get_string_value (kernel_config , "PUERTO_ESCUCHA" );
    IP_MEMORIA = config_get_string_value (kernel_config , "IP_MEMORIA" );
    PUERTO_MEMORIA = config_get_string_value (kernel_config , "PUERTO_MEMORIA" );
    IP_CPU = config_get_string_value (kernel_config , "IP_CPU" );
    PUERTO_CPU_DISPATCH = config_get_string_value (kernel_config , "PUERTO_CPU_DISPATCH" );
    PUERTO_CPU_INTERRUPT = config_get_string_value (kernel_config , "PUERTO_CPU_INTERRUPT" );
    ALGORITMO_PLANIFICACION = config_get_string_value (kernel_config , "ALGORITMO_PLANIFICACION" );
    QUANTUM = config_get_string_value (kernel_config , "QUANTUM" ); //Da segmentation fault sino pongo get_string_value
    RECURSOS = config_get_string_value (kernel_config , "RECURSOS" );
    INSTANCIAS_RECURSOS = config_get_string_value (kernel_config , "INSTANCIAS_RECURSOS" );
    GRADO_MULTIPROGRAMACION = config_get_string_value (kernel_config , "GRADO_MULTIPROGRAMACION" );


    log_info(kernel_logger, "PUERTO_ESCUCHA: %s", PUERTO_ESCUCHA);
    log_info(kernel_logger, "IP_MEMORIA: %s", IP_MEMORIA);
    log_info(kernel_logger, "PUERTO_MEMORIA: %s", PUERTO_MEMORIA);
    log_info(kernel_logger, "IP_CPU: %s", IP_CPU);
    log_info(kernel_logger, "PUERTO_CPU_DISPATCH: %s", PUERTO_CPU_DISPATCH);
    log_info(kernel_logger, "PUERTO_CPU_INTERRUPT: %s", PUERTO_CPU_INTERRUPT);
    log_info(kernel_logger, "ALGORITMO_PLANIFICACION: %s", ALGORITMO_PLANIFICACION);
    log_info(kernel_logger, "QUANTUM: %s", QUANTUM);
    log_info(kernel_logger, "RECURSOS: %s", RECURSOS);
    log_info(kernel_logger, "INSTANCIAS_RECURSOS: %s", INSTANCIAS_RECURSOS);
    log_info(kernel_logger, "GRADO_MULTIPROGRAMACION: %s", GRADO_MULTIPROGRAMACION);

//inicia servidor kernel
fd_kernel = iniciar_servidor (PUERTO_ESCUCHA, kernel_logger, "INICIADO EL KERNEL");


//conestarse a memoria como cliente
log_info (kernel_logger, "Intentando conexion a memoria");
fd_memoria = crear_conexion (IP_MEMORIA, PUERTO_MEMORIA,"memoria");
//log_info (kernel_logger, "Conectado a memoria exitosamente.");
handshakeClient(fd_memoria, 2);

//conestarse a cpu como cliente DISPATCH
log_info (kernel_logger, "Intentando conexion a CPU");
fd_cpu_dispatch = crear_conexion (IP_CPU, PUERTO_CPU_DISPATCH,"CPU");
log_info (kernel_logger, "Conectado a cpu exitosamente por dispach.");
handshakeClient(fd_cpu_dispatch, 2);

//conestarse a cpu como cliente INTERRUPT
log_info (kernel_logger, "Intentando conexion a CPU");
fd_cpu_interrupt = crear_conexion (IP_CPU, PUERTO_CPU_INTERRUPT,"CPU");
log_info (kernel_logger, "Conectado a cpu exitosamente por interrupt.");
handshakeClient(fd_cpu_interrupt, 2);


/*
//esperar conexion de entradasalida
log_info (kernel_logger, "Esperando a conectar con EntradaSalida.");
fd_entradasalida = esperar_cliente (fd_kernel, kernel_logger, "ENTRADASALIDA");
handshakeServer(fd_entradasalida);
*/

/*
//escuchar mensajes de entradasalida
pthread_t hilo_entradasalida;
pthread_create (&hilo_entradasalida, NULL, (void*)kernel_escuchar_entradasalida, NULL);
pthread_detach (hilo_entradasalida);
*/


//escuchar mensajes de cpu
pthread_t hilo_cpu;
pthread_create (&hilo_cpu, NULL, (void*)kernel_escuchar_cpu, NULL);
pthread_detach (hilo_cpu);


//escuchar mensajes de memoria
pthread_t hilo_memoria;
pthread_create (&hilo_memoria, NULL, (void*)kernel_escuchar_memoria, NULL);
pthread_detach (hilo_memoria);

// Creo la cola que voy a usar para guardar mis PCBs
cola_new = queue_create();
cola_ready = queue_create();
cola_blocked = queue_create();

kernel_logs_obligatorios = log_create(".//logs_obligatorios.log", "logs", true, LOG_LEVEL_INFO);

if (kernel_logs_obligatorios == NULL)
{
    perror("Algo paso con el log_Obligatorio. No se pudo crear.");
	exit(EXIT_FAILURE);
}

pthread_t hilo_planificador_corto_plazo;
pthread_create (&hilo_planificador_corto_plazo, NULL, (void*)planificador_corto_plazo, NULL);
pthread_detach (hilo_planificador_corto_plazo);

pthread_t hilo_planificador_largo_plazo;
pthread_create (&hilo_planificador_largo_plazo, NULL, (void*)planificador_largo_plazo, NULL);
pthread_detach (hilo_planificador_largo_plazo);

pthread_t escuchar_instancias_entrada_salida;
int* fd_copy_kernel = malloc(sizeof(int));
*fd_copy_kernel = fd_kernel;
pthread_create(&escuchar_instancias_entrada_salida,NULL,(void*)atender_entrada_salida,fd_copy_kernel);
pthread_detach(escuchar_instancias_entrada_salida);

consolaInteractiva();

while(1)
{
    //nada
}

liberar_config(kernel_config);
liberar_logger(kernel_logger);
liberar_conexion(fd_memoria);
liberar_conexion(fd_cpu_interrupt);
liberar_conexion(fd_cpu_dispatch);
liberar_conexion(fd_kernel);
liberar_conexion(fd_entradasalida);
sem_destroy(&sem);
sem_destroy(&sem_cant);

return (EXIT_SUCCESS);
}




