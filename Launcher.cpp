#include "Client.hpp"

int main()
{
    Client curr_client;

    curr_client.connect();
    curr_client.startReceiver();
    curr_client.GUI();
    curr_client.disconnect();
    return 0;
}