#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <iomanip>
#include <regex>
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

	inet_pton(AF_INET, ServerAddressText, &Connection->server_address.sin_addr);

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
	REP_PRESENCE = 0x61,
	COM_TEST_ERROR = 0x01
};

UINT8 UdpCheckSum(UINT8* buffer, int length)
{
	UINT8 checkSum = 0;
	int i;
	for (i = 0; i < length; i++)
	{
		checkSum += buffer[i];
	}
	return checkSum;
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

void UdpClearReceptionBuffer(UDP_CONNECTION* connection)
{
	char message[105];
	int length = 0;
	struct sockaddr_in address;
	while (wait_received(connection->socket_client, 0))
	{
		recvfrom(connection->socket_client, message, sizeof connection->received_data, 0,
			(struct sockaddr*)&address, &length);
	}
}

void SendMessageU(UDP_CONNECTION* connection, MessageType messageType, UINT8 dataLength, void* data)
{
	char controlSum = 0;
	UINT8 Message[105] = { 0 };
	Message[0] = messageType;
	Message[1] = connection->MessageCount++;
	Message[2] = dataLength;

	for (int i = 0; i < dataLength; i++)
	{
		Message[3 + i] = ((UINT8*)data)[i];
	}

	controlSum = (UINT8)UdpCheckSum(Message, dataLength + 3);

	Message[3 + dataLength] = controlSum;

	int status = 0;
	status = sendto(connection->socket_client,
		(char*)Message, dataLength + 4, 0,				//+4 pour messageType,Message count, dataLength et controlSum
		(struct sockaddr*)&connection->server_address,
		sizeof connection->server_address);
	
}

int  ReceiveMessage(UDP_CONNECTION* connection, MessageType messageType)
{
	// Recevoir les données et l'adresse de l'émetteur
	long timeout_milliseconds = 1000;
	if (wait_received(connection->socket_client, timeout_milliseconds) > 0)
	{
		connection->response_source_address_length = sizeof connection->response_source_address;
		connection->received_data_size = recvfrom(connection->socket_client,
			connection->received_data, sizeof connection->received_data, 0,
			(struct sockaddr*)&connection->response_source_address,
			&connection->response_source_address_length);
	}
	// Traiter les données de la réponse
	if (connection->received_data_size >= 0 && connection->received_data[0] == messageType + 1)//Controler checksum
	{


		//printf("Reponse recue:%s\n", connection->received_data);
		/*for (int i = 0; i < connection->received_data_size - 1; i++)
		{
			std::cout << std::hex << (int)connection->received_data[i];
		}
		std::cout << std::endl;*/

		return 1;

	}
	else if (connection->received_data_size >= 0 && connection->received_data[0] == 0x10)
	{
		printf("Erreur:%s\n", connection->received_data);
		return 0;
	}
	else
	{
		printf("Erreur de reception\n");
		connection->ConnectionError = 1;
		return 0;
	}

}

int UdpReceiveMessage(UDP_CONNECTION* connection, void* receiveData, int* receiveDataLength)
{
	int addressOriginalSize;
	int status, i;
	UINT8 reception[105];

	if (wait_received(connection->socket_client, 500))
	{
		addressOriginalSize = sizeof connection->server_address;
		status = recvfrom(connection->socket_client, (char*)reception, 105, 0,
			(struct sockaddr*)&connection->server_address, &addressOriginalSize);
		if (UdpCheckSum(reception, status) == reception[status - 1])
		{
			if ((receiveData != NULL) && (receiveDataLength != NULL))
			{
				*receiveDataLength = (int)reception[2];
				for (i = 0; i < reception[2]; i++)
					((UINT8*)receiveData)[i] = reception[i + 3];
			}
			return 1;
		}
		else
		{
			connection->ConnectionError = 1;
			connection->ConnectionErrorDescription = "CONERROR_BAD_CHECKSUM";
			return 2;
		}
	}
	else
		return 0;
}

void SendReceive(UDP_CONNECTION* connection, MessageType messageType, UINT8 dataLength, void* data)
{
	int attempts = 0;

	if (!connection->ConnectionError)
	{
		while (attempts < 3)
		{
			attempts++;
			SendMessageU(connection, messageType, dataLength, (void*)data);
			if (ReceiveMessage(connection, messageType))
			{
				break;
			}
		}
		if (attempts >= 3)
		{
			if (connection->received_data_size != NULL)
				connection->received_data_size = 0;
			connection->ConnectionError = 1;
			connection->ConnectionErrorDescription = "CONERROR_RECEIVE_TIMEOUT";
		}
	}

}






void SendMessagel(UDP_CONNECTION* Connection, UINT8 Message[], int dataLength)//char *response recuperer la réponse
{

	int status = 0;
	status = sendto(Connection->socket_client,
		(char*)Message, dataLength, 0,
		(struct sockaddr*)&Connection->server_address,
		sizeof Connection->server_address);

	// Recevoir les données et l'adresse de l'émetteur
	long timeout_milliseconds = 3000;
	if (wait_received(Connection->socket_client, timeout_milliseconds) > 0)
	{
		Connection->response_source_address_length = sizeof Connection->response_source_address;
		Connection->received_data_size = recvfrom(Connection->socket_client,
			Connection->received_data, sizeof Connection->received_data, 0,
			(struct sockaddr*)&Connection->response_source_address,
			&Connection->response_source_address_length);
	}
	// Traiter les données de la réponse
	if (Connection->received_data_size >= 0)
	{

		printf("Reponse recue:%s\n", Connection->received_data);
		for (int i = 0; i < Connection->received_data_size - 1; i++)
		{
			std::cout << std::hex << (int)Connection->received_data[i];
		}
		std::cout << std::endl;
	}
	else
		printf("Erreur de reception\n");


}
//void UdpSendReceive(UDP_CONNECTION* connection, char commandNumber, void* data, int dataLength, void* receiveData, int* receiveDataLength)
//{
//	int attempts = 0;
//
//	if (!connection->ConnectionError)
//	{
//		while (attempts < 3)
//		{
//			attempts++;
//			UdpSendMessage(connection, commandNumber, (void*)data, dataLength);
//			if (UdpReceiveMessage(connection, receiveData, receiveDataLength))
//				break;
//		}
//		if (attempts >= 3)
//		{
//			if (receiveDataLength != NULL)
//				*receiveDataLength = 0;
//			connection->ConnectionError = 1;
//			connection->ConnectionErrorDescription = "CONERROR_RECEIVE_TIMEOUT";
//		}
//	}
//}

void UdpSendMessage(UDP_CONNECTION* connection, char commandNumber, void* data, int dataLength)
{
	int attempts = 0;
	int status = 0;
	int i, messageLength;
	UINT8 Message[105] = { 0 };

	char controlSum = 0;

	Message[0] = commandNumber;
	Message[1] = connection->MessageCount++;
	Message[2] = dataLength;

	for (int i = 0; i < dataLength; i++)
	{
		Message[3 + i] = ((UINT8*)data)[i];
	}

	controlSum = (UINT8)UdpCheckSum(Message, dataLength + 3);

	Message[3 + dataLength] = controlSum;

	if (!connection->ConnectionError)
	{
		UdpClearReceptionBuffer(connection);

		while (attempts < 3)
		{
			attempts++;
			status = sendto(connection->socket_client, (char*)Message, dataLength + 4, 0, (struct sockaddr*)&connection->server_address, sizeof(connection->server_address));
			if (status)
				break;
		}
		if (attempts >= 3)
		{
			connection->ConnectionError = 1;
			connection->ConnectionErrorDescription = "CONERROR_SEND_TIMEOUT";
		}
	}
}







//void UdpSend(UDP_CONNECTION* connection, char commandNumber, void* data, int dataLength)
//{
//	UdpSendReceive(connection, commandNumber, (void*)data, dataLength, NULL, NULL);
//}



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


void RobotConvoyeurOn(UDP_CONNECTION* connection)
{
	UINT32 data[6] = { 0 };
	MessageType messageType = COM_CONVEYOR;
	int dataLength = 0;
	data[0] = 1;
	messageType = COM_CONVEYOR;
	dataLength = 1;
	SendReceive(connection, messageType, dataLength, &data);

}

void RobotConvoyeurOff(UDP_CONNECTION* connection)
{
	UINT32 data[6] = { 0 };
	MessageType messageType = COM_CONVEYOR;
	int dataLength = 0;
	data[0] = 0;
	messageType = COM_CONVEYOR;
	dataLength = 1;
	SendReceive(connection, messageType, dataLength, &data);

}

void RobotCapteurPallette(UDP_CONNECTION* connection)
{
	UINT32 data[6] = { 0 };
	MessageType messageType = COM_PALLET_SENSOR;
	int dataLength = 0;
	data[0] = 0;
	dataLength = 0;
	SendReceive(connection, messageType, dataLength, &data);
	std::cout << std::hex << (int)connection->received_data[3] << std::endl;

}

void RobotVacuumOn(UDP_CONNECTION* connection)
{
	UINT32 data[6] = { 0 };
	MessageType messageType = COM_SET_VACUUM;
	data[0] = 1;
	int dataLength = 1;
	SendReceive(connection, messageType, dataLength, &data);

}

void RobotVacuumOff(UDP_CONNECTION* connection)
{
	UINT32 data[6] = { 0 };
	MessageType messageType = COM_SET_VACUUM;
	data[0] = 0;
	int dataLength = 1;
	SendReceive(connection, messageType, dataLength, &data);
}

void RobotPresencePiece(UDP_CONNECTION* connection)
{
	UINT32 data[6] = { 0 };
	MessageType messageType = COM_GET_HAS_PIECE;
	data[0] = 0;

	int dataLength = 0;
	SendReceive(connection, messageType, dataLength, &data);
	std::cout << std::hex << (int)connection->received_data[3] << std::endl;
}


bool CheckCapteur(UDP_CONNECTION* connection, MessageType messageType, int valueToExit)
{
	int attempts = 0;
	do
	{
		UINT32 data[6] = { 0 };
		data[0] = 0;
		int dataLength = 0;
		SendReceive(connection, messageType, dataLength, &data);
		
		if (attempts > 10000)
		{
			return false;
			std::cout << "Erreur de communication avec le robot"<<std::endl;
		}
		attempts++;
	} while (connection->received_data[3] != valueToExit);
	return true;

}

void RobotPICK(UDP_CONNECTION* connection)
{
	/*char data = 1;
	char dataLength = 1;
	char* Message = NULL;
	Message = (char*)malloc((dataLength + 3) * sizeof(char));


	SetMessage(Message, COM_ROBOT_GO_HOME, 1, dataLength, &data);
	SendMessagel(Connection, Message, dataLength + 3);*/
	CheckCapteur(connection, COM_ROBOT_IS_MOVING, 0);
	INT32 data[6] = { 0 };
	data[0] = htonl(Pick_X);
	data[1] = htonl(Pick_Y);
	data[2] = htonl(Pick_Z + 10000);
	data[3] = htonl(Pick_Rx);
	data[4] = htonl(Pick_Ry);
	data[5] = htonl(Pick_Rz);
	MessageType messageType = COM_ROBOT_MOVE;
	int dataLength = 24;

	SendReceive(connection, messageType, dataLength, &data);
	//Sleep(1000);
	std::cout << "lalalallalalala" << std::endl;
	CheckCapteur(connection, COM_ROBOT_IS_MOVING, 0);
	
		data[0] = htonl(Pick_X);
		data[1] = htonl(Pick_Y);
		data[2] = htonl(Pick_Z);
		data[3] = htonl(Pick_Rx);
		data[4] = htonl(Pick_Ry);
		data[5] = htonl(Pick_Rz);
		messageType = COM_ROBOT_MOVE;
		dataLength = 24;
	SendReceive(connection, messageType, dataLength, &data);
	
	CheckCapteur(connection, COM_ROBOT_IS_MOVING, 0);
}



void RobotHome(UDP_CONNECTION* connection)
{
	INT32 data[6] = { 0 };
	data[0] = htonl(HOME_X);
	data[1] = htonl(HOME_Y);
	data[2] = htonl(HOME_Z);
	data[3] = htonl(HOME_Rx);
	data[4] = htonl(HOME_Ry);
	data[5] = htonl(HOME_Rz);
	MessageType messageType = COM_ROBOT_MOVE;
	int dataLength = 24;
	SendReceive(connection, messageType, dataLength, &data);
}

void RobotPlace(UDP_CONNECTION* connection)
{
	/*char data = 1;
	char dataLength = 1;
	char* Message = NULL;
	Message = (char*)malloc((dataLength + 3) * sizeof(char));


	SetMessage(Message, COM_ROBOT_GO_HOME, 1, dataLength, &data);
	SendMessagel(Connection, Message, dataLength + 3);*/
	CheckCapteur(connection, COM_ROBOT_IS_MOVING, 0);
	INT32 data[6] = { 0 };
	data[0] = htonl(Place_X);
	data[1] = htonl(Place_Y);
	data[2] = htonl(Place_Z + 10000);
	data[3] = htonl(Place_Rx);
	data[4] = htonl(Place_Ry);
	data[5] = htonl(Place_Rz);
	MessageType messageType = COM_ROBOT_MOVE;
	int dataLength = 24;

	SendReceive(connection, messageType, dataLength, &data);
	//Sleep(1000);

	if (CheckCapteur(connection, COM_ROBOT_IS_MOVING, 0))
	{
		data[0] = htonl(Place_X);
		data[1] = htonl(Place_Y);
		data[2] = htonl(Place_Z);
		data[3] = htonl(Place_Rx);
		data[4] = htonl(Place_Ry);
		data[5] = htonl(Place_Rz);
		messageType = COM_ROBOT_MOVE;
		dataLength = 24;
		SendReceive(connection, messageType, dataLength, &data);
	}
	CheckCapteur(connection, COM_ROBOT_IS_MOVING, 0);
}

void PilotageAutomatiser(UDP_CONNECTION* connection, int nbreCyle)
{
	for (int i = 0; i < nbreCyle; i++)
	{

		RobotPICK(connection);
		CheckCapteur(connection, COM_GET_HAS_PIECE, 1);
		//Sleep(100);
		RobotVacuumOn(connection);
		CheckCapteur(connection, COM_PALLET_SENSOR, 1);
		//Sleep(100);
		RobotPlace(connection);
		//Sleep(100);
		RobotVacuumOff(connection);
		//Sleep(100);
		RobotHome(connection);
		CheckCapteur(connection, COM_ROBOT_IS_MOVING, 0);
		//Sleep(100);
		RobotConvoyeurOn(connection);
		//Sleep(100);
		if (!CheckCapteur(connection, COM_PALLET_SENSOR, 1))
		{
			break;
		}
	}
}

void ManagerManuelMenu(UDP_CONNECTION* Connection) {
	char chosenMenu;
	UINT8 Message[105] = { 0 };
	//char* Message=NULL;
	INT32 data[100] = { 0 };
	//char* data = NULL;


	do {
		bool invalidChoice = false;
		Connection->ConnectionError = 0;
		chosenMenu = ChooseMenuManuel();
		switch (chosenMenu) {
		case '1':
			RobotConvoyeurOn(Connection);
			break; // convoyeur on
		case '2':
			RobotConvoyeurOff(Connection);
			break; // convoyeur off
		case '3':
			RobotCapteurPallette(Connection);
			break; // show presence pallette
		case '4':
			RobotVacuumOn(Connection);
			break; // vacuum On
		case '5':
			RobotVacuumOff(Connection);
			break; // vacuum off
		case '6':
			RobotPresencePiece(Connection);
			break; // show presence piece
		case '7':
			RobotHome(Connection);
			break; // Home
		case '8':
			RobotPICK(Connection);
			break; // Pick
		case '9':
			RobotPlace(Connection);
			break; // Place
		case '10': break; // show mouvement
		case 'q':
			std::cout << "Quitte le menu des fonctions manuelles" << std::endl;

			break;
		default:
			std::cout << "Choix invalide" << std::endl;
			invalidChoice = true;
			break;
		}
		if (chosenMenu != 'q' && !invalidChoice)//Attention verfifier que l'on est rentré dans le switch case!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		{

			/*SendMessageU(&Connection,Message, messageType, dataLength, &data);
			ReceiveMessage(&Connection);*/
			//SendReceive(&Connection, messageType, dataLength, &data);
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
				"9. Quitter le programme\n" <<
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

const char* GetIpAdresse(const char * IpAdresse)
{
	char NewIpAdresse[4] = { 0 };
	char test[128];
	std::cout << "Entrer l'adresse IP" << std::endl;
	std::cin >> test;
	#pragma warning(suppress : 4996);
	if (sscanf(test, " %c. %c. %c. %c", &NewIpAdresse[0], &NewIpAdresse[1], &NewIpAdresse[2], &NewIpAdresse[3]))
	{
		std::cout << "Nouvelle adresse Ip : " << NewIpAdresse[0] << '.' << NewIpAdresse[1] << '.' << NewIpAdresse[2] << '.' << NewIpAdresse[3] << std::endl;
		return NewIpAdresse;
	}
	else
	{
		std::cout << "Adresse Ip invalide" << std::endl;
		return IpAdresse;
	}
}

void ManagerUserMenu(UDP_CONNECTION* Connection) {
	char chosenMenu = 0;
	int nbreCycle = 0;
	int choix = 0;
	Connection->ConnectionError = 0;
	INT32 data[6] = { 0 };
	do {
		Connection->ConnectionError = 0;
		chosenMenu = ChooseUserMenu();
		switch (chosenMenu) {
		case '1':
			ManagerManuelMenu(Connection);

			break; // Commande manuelle
		case '2':
			std::cout << "Combien de cycle voulez vous effectuer ?" << std::endl;
			std::cout << '>';
			std::cin >> nbreCycle;
			PilotageAutomatiser(Connection, nbreCycle);
			break; // Pilotage automatisé
		case '3':
			std::cout << "Veuillez choisir l'erreur à générer :" << std::endl;
			std::cin >> choix;
			switch (choix)
			{
			case 0:
				SendReceive(Connection, COM_TEST_ERROR, 1, data);
				break;
			case 1:
				SendReceive(Connection, COM_CONVEYOR, 10, data);
				break;
			default:
				std::cout << "Mauvais choix try again" << std::endl;
				break;
			}

			break; // Messages d'erreur de communication
		case '4':

			UdpInit(Connection, GetIpAdresse("127.0.0.1"));
			break; // Saisie de l'adresse de la machine
		case '5':
			UdpInit(Connection, "127.0.0.255");
			SendReceive(Connection, COM_PRESENCE, 0, data);
			printf("Reponse recue:%s\n", Connection->received_data);
			
			std::cout << (int)(struct sockaddr*)&Connection->response_source_address;
			
			//std::endl;
			for (int i = 2; i < Connection->received_data_size; i++)
			{
				std::cout << Connection->received_data[i];
			}
			break; // Detection par diffusion
		case '9': 
			std::cout << "Quitte le menu User" << std::endl; 
			exit(0);
			break;
		}
	} while (chosenMenu != '9');
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

	ManagerUserMenu(&udpConnection);

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