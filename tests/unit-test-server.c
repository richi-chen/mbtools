/*
 * Copyright © 2013 Stéphane Raimbault <stephane.raimbault@webstack.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <modbus.h>

static int socket = -1;
static modbus_t *ctx;
static modbus_mapping_t *mb_mapping;

static void signal_handler(int dummy)
{
    modbus_mapping_free(mb_mapping);
    if (socket != -1) {
        close(socket);
    }
    modbus_close(ctx);
    modbus_free(ctx);

    exit(dummy);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc == 1) {
        ctx = modbus_new_rtu("/dev/ttyUSB1", 115200, 'N', 8, 1);
        modbus_set_debug(ctx, TRUE);
        modbus_set_slave(ctx, 1);
        modbus_connect(ctx);
    } else {
        ctx = modbus_new_tcp("127.0.0.1", 1502);
        modbus_set_debug(ctx, TRUE);
        printf("Listen on 127.0.0.1:1502\n");
        socket = modbus_tcp_listen(ctx, 5);
        modbus_tcp_accept(ctx, &socket);
    }

    mb_mapping = modbus_mapping_new(0, 0, 100, 0);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    modbus_set_float(12345.6, mb_mapping->tab_registers);
    modbus_set_float(789.0, mb_mapping->tab_registers + 2);
    for (i=0; i < 10; i++) {
        mb_mapping->tab_registers[i + 4] = i+1;
    }

    signal(SIGINT, signal_handler);

    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;

        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping);
        } else if (rc == -1) {
            /* Connection closed by the client or error */
            break;
        }
    }
    printf("Quit the loop: %s\n", modbus_strerror(errno));

    modbus_mapping_free(mb_mapping);
    if (socket != -1) {
        close(socket);
    }
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
