#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#define DATA_SIZE 1024
#define ADDRESS_TEXT_SIZE 128
// Definir le port 
#define PORT_NUMBER 27598
int main()
{
	WSADATA info;
	SOCKET socket_client;
	struct sockaddr_in server_address;
	struct sockaddr_in response_source_address;
	int response_source_address_length;
	char address_text[ADDRESS_TEXT_SIZE];
	char data_to_send[DATA_SIZE];
	int quit;
	char received_data[DATA_SIZE];
	int received_data_size, status;
	// Initialisation Winsock 2 - spécifique à Windows
	WSAStartup(MAKEWORD(2, 2), &info);
	// Créer le socket UDP
	socket_client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// Saisie de l'adresse IP V4
	printf("Adresse IPV4:");
	fgets(address_text, sizeof address_text, stdin);
	address_text[strlen(address_text) - 1] = '\0';
	// Preparer la structure avec l'adresse et le port de destination
	memset(&server_address, 0, sizeof server_address);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT_NUMBER); // Port network byte order
	inet_pton(AF_INET, address_text, &server_address.sin_addr);
	// Gérer la communication
	do
	{
		// Saisie des données à envoyer
		printf("Message a envoyer:");
		fgets(data_to_send, sizeof data_to_send, stdin);
		data_to_send[strlen(data_to_send) - 1] = '\0';
		quit = strcmp(data_to_send, "quit") == 0;
		if (!quit)
		{
			// Envoyer les données
			status = sendto(socket_client,
				data_to_send, strlen(data_to_send), 0,
				(struct sockaddr*)&server_address,
				sizeof server_address);
			// Recevoir les données et l'adresse de l'émetteur
			response_source_address_length = sizeof response_source_address;
			received_data_size = recvfrom(socket_client,
				received_data, sizeof received_data - 1, 0,
				(struct sockaddr*)&response_source_address,
				&response_source_address_length);
			// Traiter les données de la réponse
			if (received_data_size >= 0)
			{
				received_data[received_data_size] = '\0';
				printf("Reponse recue:%s\n", received_data);
			}
			else
				printf("Erreur de reception\n");
		}
	} while (!quit);
	// Fermer le socket (libérer les ressources)
	closesocket(socket_client);
	// Finaliser Winsock 2 
	WSACleanup();
}
