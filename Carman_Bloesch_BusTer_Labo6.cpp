#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#define DATA_SIZE 1024
#define ADDRESS_TEXT_SIZE 128

#define HOME_X (250000)
#define HOME_Y (-300000) 
#define HOME_Z (150000)
#define HOME_Rx (180000000)
#define HOME_Ry (0)
#define HOME_Rz (0)

#define Pick_X (426500)
#define Pick_Y (-191600)
#define Pick_Z (39300)
#define Pick_Rx (195000000)
#define Pick_Ry (18000000)
#define Pick_Rz (-10000000)

#define Place_X (155000)
#define Place_Y (-497000)
#define Place_Z (-50000)
#define Place_Rx (180000000)
#define Place_Ry (0)
#define Place_Rz (0)

// Definir le port 
#define PORT_NUMBER 27598

enum MessageType {
	REP_ERROR = 0x10,
	REP_INFO = 0x20,
	COM_CONVEYOR = 0x30,
	REP_CONVEYOR = 0x31,
	COM_PALLET_SENSOR = 0x32,
	REP_PALLET_SENSOR = 0x33,
	COM_SET_VACUUM = 0x34,
	REP_SET_VACUUM = 0x35,
	COM_GET_HAS_PIECE = 0x36,
	REP_GET_HAS_PIECE = 0x37,
	COM_ROBOT_MOVE = 0x40,
	REP_ROBOT_MOVE = 0x41,
	COM_ROBOT_IS_MOVING = 0x42,
	REP_ROBOT_IS_MOVING = 0x43,
	COM_PRESENCE = 0x60,
	REP_PRESENCE = 0x61
};

const char* SetMessage(MessageType messageType, INT8 messageNum,INT8 dataLength,INT8* data)
{
	
	char Message[104];
	Message[0] = messageType;
	Message[1] = messageNum;
	Message[2] = dataLength;
	for (int i = 0; i < dataLength; i++)
	{
		Message[3 + i] = data[i];
	}
	INT8 controlSum = 0;
	for (int i = 0; i < dataLength; i++)
	{
		controlSum += Message[i];
	}
	Message[3 + dataLength] = controlSum;
	return Message;
	
}

char ChooseUserMenu(){
	char chosenMenu = '0';
	bool firstRun = true;
	do {
		if(firstRun){
			std::cout << "Assemblage de processeurs =====================" <<
				"1. Commandes manuelles\n" <<
				"2. Pilotage automatise\n" <<
				"3. Messages d'erreur de communication\n" <<
				"4. Saisie de l'adresse de la machine\n" <<
				"5. Detection par diffusion\n" <<
				"9. Quitter le programme\n" <<
				"Choix > ";
		}
		else{
			std::cout << " Try again\n" << "Choix > ";
		}
		std::cin >> chosenMenu;
		firstRun = false;
	} while (!(chosenMenu > '0' && chosenMenu <= '5' || chosenMenu == '9'));
	return chosenMenu;
}

//void ManagerUserMenu(){
//	char chosenMenu;
//    do {
//		chosenMenu = ChooseUserMenu();
//		switch (chosenMenu) {
//		case '1':  ManagerManuelMenu(); break;
//		case '2': break;
//		case '3': break;
//		case '4': break;
//		case '5': break;
//		case '9': break;
//		}
//	} while (chosenMenu != 9);
//}

char ChooseMenuManuel(){
	char chosenMenu = '0';
	bool firstRun = true;
	do {
		if (firstRun) {
		std::cout << "Commandes manuelles ------------------------" <<
			"1. Allumer le convoyeur\n" <<
			"2. Arreter le convoyeur\n" <<
			"3. Afficher l'etat du capteur de palette\n" <<
			"4. Activer le vacuum\n" <<
			"5. Desactiver le vacuum\n" <<
			"6. Afficher l'etat presence piece\n" <<
			"7. Deplacer le robot en position Home\n" <<
			"8. Deplacer le robot en position Pick\n" <<
			"9. Deplacer le robot en position Place\n" <<
			"10. Afficher si le robot est en mouvement\n" <<
			"99. Quitter les fonctions manuelles\n" <<
			"Choix > ";
			}
			else{
				std::cout << " Try again\n" << "Choix > ";
			}
			std::cin >> chosenMenu;
			firstRun = false;
		} while (!(chosenMenu > '0' && chosenMenu <= '10' || chosenMenu == '99'));
		return chosenMenu;
	}

void ManagerManuelMenu() {
	char chosenMenu;
	do {
		chosenMenu = ChooseMenuManuel();
		switch (chosenMenu) {
		case '1': break;
		case '2': break;
		case '3': break;
		case '4': break;
		case '5': break;
		case '6': break;
		case '7': break;
		case '8': break;
		case '9': break;
		case '10': break;
		case '99': break;
		}
	} while (chosenMenu != 99);
}

int wait_received(SOCKET socket, long timeout_milliseconds)
{
	struct timeval timeout;
	fd_set fd_set_receive;
	int status;
	timeout.tv_sec = timeout_milliseconds / 1000;
	timeout.tv_usec = (timeout_milliseconds % 1000) * 1000;
	FD_ZERO(&fd_set_receive);
	FD_SET(socket, &fd_set_receive);
	status = select(0, &fd_set_receive, NULL, NULL, &timeout);
	if (status == SOCKET_ERROR)
	{
		printf("erreur d'utilisation de socket.\n");
		
	}
	return status > 0;
}

void AutoDetection()
{
	
}

int main()
{
	MessageType messageType;
	WSADATA info;
	SOCKET socket_client;
	struct sockaddr_in server_address;
	struct sockaddr_in response_source_address;
	int response_source_address_length;
	char address_text[ADDRESS_TEXT_SIZE] = {127,0,0,1};
	char data_to_send[DATA_SIZE];
	int quit;
	char received_data[DATA_SIZE];
	int received_data_size, status;
	// Initialisation Winsock 2 - spécifique à Windows
	WSAStartup(MAKEWORD(2, 2), &info);
	// Créer le socket UDP
	socket_client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// Saisie de l'adresse IP V4
	/*printf("Adresse IPV4:");
	fgets(address_text, sizeof address_text, stdin);*/
	address_text[strlen(address_text) - 1] = '\0';
	
	// Preparer la structure avec l'adresse et le port de destination
	memset(&server_address, 0, sizeof server_address);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT_NUMBER); // Port network byte order
	inet_pton(AF_INET, address_text, &server_address.sin_addr);
	//Autoriser la diffusion restreinte
	int value = 1;
	setsockopt(socket_client, SOL_SOCKET, SO_BROADCAST, (char*)&value, sizeof(value));
	// Gérer la communication
	do
	{
		// Saisie des données à envoyer
		printf("Message a envoyer:");
		fgets(data_to_send, sizeof data_to_send, stdin);
		data_to_send[strlen(data_to_send) - 1] = '\0';
		quit = strcmp(data_to_send, "quit") == 0;
		//;
		if (!quit)
		{
			// Envoyer les données
			INT8 data = 1;
			status = sendto(socket_client,
				SetMessage(COM_CONVEYOR,10,1,&data), strlen(data_to_send), 0,
				(struct sockaddr*)&server_address,
				sizeof server_address);
			
			// Recevoir les données et l'adresse de l'émetteur
			long timeout_milliseconds = 1000;
			if (wait_received(socket_client, timeout_milliseconds)>0)
			{
				response_source_address_length = sizeof response_source_address;
				received_data_size = recvfrom(socket_client,
					received_data, sizeof received_data - 1, 0,
					(struct sockaddr*)&response_source_address,
					&response_source_address_length);
			}
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
