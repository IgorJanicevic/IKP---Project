#include "menu.h"

void display_menu(SOCKET sock) {
    while (1) {
        printf("\n\n****************************\n");
        printf("1. Alociraj memoriju\n");
        printf("2. Oslobodi memoriju\n");
        printf("0. Izlaz\n");

        int action;
        Request req;
        if (scanf("%d", &action) != 1) {
            printf("Nepostojeci zahtev.\n");
            while (getchar() != '\n'); 
            continue;
        }

        switch (action) {
            case 1:
                printf("Unesi velicinu bloka:\n");
                if (scanf("%d", &req.size) != 1 || req.size <= 0) {
                    printf("Nevazeca vrednost! Molimo vas uneseti pozitvan broj.\n");
                    while (getchar() != '\n'); 
                    break;
                }
                req.type = 1;
                send_request(sock, &req);
                receive_message(sock);
                break;

            case 2:
                req.type = 2;
                char input[64];
                printf("Unesi adresu bloka (heksadecimalno):\n");
                scanf("%63s", input);

                if (!is_valid_hex_address(input)) {
                    printf("Nevalidan format adrese!\n");
                    break;
                }
                sscanf(input, "%p", &req.block_id);
                send_request(sock, &req);
                receive_message(sock);
                break;

            case 0:
                printf("Uspesno ste se odjavili.\n");
                return;

            default:
                printf("Nevalidna komadna, probajte ponovo.\n");
                break;
        }
    }
}
