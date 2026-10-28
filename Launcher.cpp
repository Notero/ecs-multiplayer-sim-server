#include "Client.hpp"

int main()
{
    Client curr_client;

    curr_client.Connect();
    curr_client.startReceiver();
    curr_client.GUI();
    curr_client.Disconnect();
    return 0;
}