#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
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

static WSADATA info; // Windows Specific

typedef struct
{
	SOCKET socket_client;
	struct sockaddr_in server_address;
	struct sockaddr_in response_source_address;
	int response_source_address_length;
	char received_data[DATA_SIZE];
	int received_data_size;
	int ConnectionError;
	const char* ConnectionErrorDescription;
	UINT8 MessageCount;
	
} UDP_CONNECTION;

void UdpInit(UDP_CONNECTION* Connection, const char* ServerAddressText)
{
	// Initialisation Winsock 2 - spécifique à Windows
	WSAStartup(MAKEWORD(2, 2), &info);
	
	// Preparer la structure avec l'adresse et le port de destination
	memset(&Connection->server_address, 0, sizeof Connection->server_address);
	
	// Initialiser la connection
	Connection->ConnectionError = 0;
	Connection->ConnectionErrorDescription = NULL;
	Connection->MessageCount = 0;

	// Initialiser le socket
	Connection->socket_client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (Connection->socket_client == INVALID_SOCKET)
	{
		Connection->ConnectionError = 1;
		Connection->ConnectionErrorDescription = "socket() failed";
		return;
	}

	// Initialiser l'adresse du serveur
	Connection->server_address.sin_family = AF_INET;
	Connection->server_address.sin_port = htons(PORT_NUMBER);
	inet_pton(AF_INET, "127.0.0.1", &Connection->server_address.sin_addr);
	
	//Autoriser la diffusion restreinte
	int value = 1;
	setsockopt(Connection->socket_client, SOL_SOCKET, SO_BROADCAST, (char*)&value, sizeof(value));
}

void UdpStop(UDP_CONNECTION* Connection)
{
	// Fermer le socket (libérer les ressources)
	closesocket(Connection->socket_client);
	// Finaliser Winsock 2
	WSACleanup();
}


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

void SetMessage(char* Message, MessageType messageType, char messageNum, char dataLength, char* data)
{
	//Message = (char*)malloc((dataLength+3) * sizeof(char));
	/*if (Message == NULL)
	{
		std::cout << "Error" << std::endl;
		return;
	}*/
	
	Message[0] = messageType;
	Message[1] = messageNum;
	Message[2] = dataLength;
	for (int i = 0; i < dataLength; i++)
	{
		Message[3 + i] = data[i];
	}
	char controlSum = 0;
	for (int i = 0; i < dataLength+3 ; i++)
	{
		controlSum += Message[i];
	}
	Message[3 + dataLength] = controlSum;
	Message[3 + dataLength+1] = '\0';
	
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


void SendMessagel(UDP_CONNECTION Connection,char Message[])
{
	
		// Envoyer les données
		int status = 0;
		/*char data = 1;
		char dataLength = 1;
		*/
		//char* Message = NULL;
		//Message = (char*)malloc((dataLength + 3) * sizeof(char));

		//SetMessage(Message, COM_CONVEYOR, 1, dataLength, &data);
		std::cout << std::to_string(Message[0]) << std::endl;
		std::cout << std::to_string(Message[1]) << std::endl;
		std::cout << std::to_string(Message[2]) << std::endl;
		std::cout << std::to_string(Message[3]) << std::endl;
		std::cout << std::to_string(Message[4]) << std::endl;
		std::cout << strlen(Message) << std::endl;
		//Attention strlen va jusqu'au /0
		status = sendto(Connection.socket_client,
			Message, strlen(Message), 0,
			(struct sockaddr*)&Connection.server_address,
			sizeof Connection.server_address);

		// Recevoir les données et l'adresse de l'émetteur
		long timeout_milliseconds = 1000;
		if (wait_received(Connection.socket_client, timeout_milliseconds) > 0)
		{
			Connection.response_source_address_length = sizeof Connection.response_source_address;
			Connection.received_data_size = recvfrom(Connection.socket_client,
				Connection.received_data, sizeof Connection.received_data - 1, 0,
				(struct sockaddr*)&Connection.response_source_address,
				&Connection.response_source_address_length);
		}
		// Traiter les données de la réponse
		if (Connection.received_data_size >= 0)
		{
			Connection.received_data[Connection.received_data_size] = '\0';
			printf("Reponse recue:%s\n", Connection.received_data);
			for (int i = 0; i < Connection.received_data_size - 1; i++)
			{
				std::cout << Connection.received_data[i];
			}
			std::cout << std::endl;


		}
		else
			printf("Erreur de reception\n");
	
	
}

char ChooseMenuManuel() {
	char chosenMenu = '0';
	bool firstRun = true;
	do {
		if (firstRun) {
			std::cout << "Commandes manuelles ------------------------\n" <<
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
				"q. Quitter les fonctions manuelles\n" <<
				"Choix > ";
		}
		else {
			std::cout << " Try again\n" << "Choix > ";
		}
		std::cin >> chosenMenu;
		firstRun = false;
	} while (!(chosenMenu > '0' && chosenMenu <= '10' || chosenMenu == '99'));
	return chosenMenu;
}




void ManagerManuelMenu(UDP_CONNECTION Connection) {
	char chosenMenu;
	char Message[105] = {0};
	//char* Message=NULL;
	char data[100] = { 0 };
	//char* data = NULL;
	
	int messageNum = 1;
	int dataLength = 0;
	MessageType messageType=COM_PRESENCE;
	do {
		bool invalidChoice = false;
		chosenMenu = ChooseMenuManuel();
		switch (chosenMenu) {
		case '1': 
			data[0] = 1;
			messageType = COM_CONVEYOR;
			dataLength = 1;
			break; // convoyeur on
		case '2': 
			data[0] = 0;
			messageType = COM_CONVEYOR;
			dataLength = 1;
			break; // convoyeur off
		case '3':

			break; // show presence pallette
		case '4': 
			data[0] = 1;
			messageType = COM_SET_VACUUM;
			dataLength = 1;
			break; // vacuum On
		case '5': 
			data[0] = 0;
			messageType = COM_SET_VACUUM;
			dataLength = 1;
			break; // vacuum off
		case '6': break; // show presence piece
		case '7': 
			data[0] = HOME_X;
			data[1] = HOME_Y;
			data[2] = HOME_Z;
			data[3] = HOME_Rx; 
			data[4] = HOME_Ry;
			data[5] = HOME_Rz;
			messageType = COM_ROBOT_MOVE;
			dataLength = 6;
			break; // Home
		case '8': break; // Pick
		case '9': break; // Place
		case '10': break; // show mouvement
		case 'q': 
			std::cout << "Quitte le menu des fonctions manuelles" << std::endl;
			
			break;
		default: 
			std::cout << "Choix invalide" << std::endl;
			invalidChoice = true;
		break;
		}
		if (chosenMenu != 'q'&&!invalidChoice)//Attention verfifier que l'on est rentré dans le switch case!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		{
			SetMessage(Message, messageType, messageNum, dataLength, data);
			SendMessagel(Connection, Message);
		}
	} while (chosenMenu != 'q');
}

char ChooseUserMenu() {
	char chosenMenu = '0';
	bool firstRun = true;
	do {
		if (firstRun) {
			std::cout << "Assemblage de processeurs =====================\n" <<
				"1. Commandes manuelles\n" <<
				"2. Pilotage automatise\n" <<
				"3. Messages d'erreur de communication\n" <<
				"4. Saisie de l'adresse de la machine\n" <<
				"5. Detection par diffusion\n" <<
				"6. Quitter le programme\n" <<
				"Choix > ";
		}
		else {
			std::cout << " Try again\n" << "Choix > ";
		}
		std::cin >> chosenMenu;
		firstRun = false;
	} while (!(chosenMenu > '0' && chosenMenu <= '5' || chosenMenu == '9'));
	return chosenMenu;
}

void ManagerUserMenu(UDP_CONNECTION Connection) {
	char chosenMenu=0;
	do {
		chosenMenu = ChooseUserMenu();
		switch (chosenMenu) {
		case '1': 
			ManagerManuelMenu(Connection);
			
			break; // Commande manuelle
		case '2': break; // Pilotage automatisé
		case '3': break; // Messages d'erreur de communication
		case '4': break; // Saisie de l'adresse de la machine
		case '5': break; // Detection par diffusion
		case '6': std::cout << "Quitte le menu User" << std::endl; break;
		}
	} while (chosenMenu != '6');
}









int main()
{
	MessageType messageType;
	
	char data_to_send[DATA_SIZE];
	int quit;
	char received_data[DATA_SIZE];
	int received_data_size{}, status;
	UDP_CONNECTION udpConnection;
	UdpInit(&udpConnection, "127.0.0.1");
	
	// Gérer la communication

	char Message[105] = { 0 };
	/*do
	{*/
		// Saisie des données à envoyer
		/*printf("Message a envoyer:");
		fgets(data_to_send, sizeof data_to_send, stdin);
		data_to_send[strlen(data_to_send) - 1] = '\0';
		quit = strcmp(data_to_send, "quit") == 0;*/
		//
		ManagerUserMenu(udpConnection);
		
	//	if (!quit)
	//	{
	//		// Envoyer les données
	//		
	//		char Message[105] = {0};
	//		char data = 1;
	//		char dataLength = 1;
	//		//char* Message = NULL;
	//		//Message = (char*)malloc((dataLength + 3) * sizeof(char));
	//		
	//		SetMessage(Message, COM_CONVEYOR, 1, dataLength, &data);
	//		std::cout << std::to_string(Message[0]) << std::endl;
	//		std::cout << std::to_string(Message[1]) << std::endl;
	//		std::cout << std::to_string(Message[2]) << std::endl;
	//		std::cout << std::to_string(Message[3]) << std::endl;
	//		std::cout << std::to_string(Message[4]) << std::endl;
	//		std::cout << strlen(Message) << std::endl;
	//		//Attention strlen va jusqu'au /0
	//		status = sendto( udpConnection.socket_client,
	//			Message, strlen(Message), 0,
	//			(struct sockaddr*)&udpConnection.server_address,
	//			sizeof udpConnection.server_address);

	//		// Recevoir les données et l'adresse de l'émetteur
	//		long timeout_milliseconds = 1000;
	//		if (wait_received(udpConnection.socket_client, timeout_milliseconds) > 0)
	//		{
	//			udpConnection.response_source_address_length = sizeof udpConnection.response_source_address;
	//			received_data_size = recvfrom(udpConnection.socket_client,
	//				received_data, sizeof received_data - 1, 0,
	//				(struct sockaddr*)&udpConnection.response_source_address,
	//				&udpConnection.response_source_address_length);
	//		}
	//		// Traiter les données de la réponse
	//		if (received_data_size >= 0)
	//		{
	//			received_data[received_data_size] = '\0';
	//			printf("Reponse recue:%s\n", received_data);
	//			for (int i = 0; i < received_data_size-1; i++)
	//			{
	//				std::cout << received_data[i];
	//			}
	//			std::cout<<std::endl;
	//			
	//			
	//		}
	//		else
	//			printf("Erreur de reception\n");
	//	}
	//} while (!quit);
	
	UdpStop(&udpConnection);
}