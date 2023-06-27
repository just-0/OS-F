#include <iostream>
#include <unistd.h>
#include <sys/select.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <thread>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#define BUFFER_LENGTH 80
static char receive[BUFFER_LENGTH];
static char receive_temp[BUFFER_LENGTH];

#define KEY 21
union semun {
    	int val;
    	struct semid_ds *buf;
    	unsigned short *array;
    	struct seminfo *__buf;
};

int semid;

struct sembuf p = {0, -1, SEM_UNDO};
struct sembuf v = {0, +1, SEM_UNDO};

std::string* ImprimirNoWait()
{
    	struct timeval timeout;
    	timeout.tv_sec = 0;
    	timeout.tv_usec = 0;

    	fd_set fds;
    	FD_ZERO(&fds);
    	FD_SET(STDIN_FILENO, &fds);

    	int ready = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &timeout);

    	if (ready > 0) {
        	// Hay datos disponibles en la entrada estándar
        	if (FD_ISSET(STDIN_FILENO, &fds)) {
            		std::string* entrada = new std::string;
            		getline(std::cin, *entrada);
            		return entrada;
        	}
    	}

    	return nullptr;
}

int Leer(int id)
{
    	while (true) {
        	if (semop(id, &p, 1) < 0) {
            		perror("semop p");
            		exit(13);
        	}
        	int ret, fd;
        	fd = open("/dev/ucsp", O_RDWR);
		memset(receive, '\0', sizeof(receive));
        	ret = read(fd, receive, BUFFER_LENGTH);
        	if (ret < 0) {
            		perror("Todavia no se ha creado el dispositivo");
            		return errno;
        	}
        	if (*receive_temp != *receive) {
            		strcpy(receive_temp, receive);
            		printf("Mensaje recibido: [%s]\n", receive_temp);
        	}
        	close(fd);
        	if (semop(id, &v, 1) < 0) {
            		perror("semop v");
            		exit(14);
        	}
		sleep(1);
    	}

    	return 1;
}

int escribir(std::string mensaje, int id)
{
	if (semop(id, &p, 1) < 0) {
            	perror("semop p");
            	exit(13);
        }
   	int ret, fd;
    	fd = open("/dev/ucsp", O_RDWR);
    	ret = write(fd, mensaje.c_str(), mensaje.length()); 
    	if (ret < 0) {
        	perror("Failed to write the message to the device.");
        	return errno;
    	}

    	strcpy(receive_temp, mensaje.data());
    	close(fd);
 	if (semop(id, &v, 1) < 0) {
            	perror("semop v");
            	exit(14);
        }
   	return 1;
}

int main() {
    	semid = semget(KEY, 1, IPC_CREAT | 0666);
    	if (semid < 0) {
        	perror("semget");
        	exit(11);
    	}

    	union semun arg;
    	arg.val = 1;
    	if (semctl(semid, 0, SETVAL, arg) < 0) {
        	perror("semctl");
        	exit(12);
    	}

    	std::cout << "Bienvenido al chat" << std::endl;

    	std::thread t1(Leer, semid);

    	while (true) {
        	std::string* entrada = ImprimirNoWait();
        	if (entrada != nullptr) {
            		std::cout << "Has ingresado: " << *entrada << std::endl;
            		escribir(*entrada, semid);
            		delete entrada; // Liberar la memoria asignada a la cadena
        	}
    	}

    	t1.join();
    	return 0;
}

