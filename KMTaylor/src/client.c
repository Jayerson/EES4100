#include <modbus-tcp.h>
#include <stdio.h>
#include <errno.h>

int main (void) {

  modbus_t *mb;
  uint16_t tab_reg[32];

  mb = modbus_new_tcp("127.0.0.1", 502);
  modbus_connect(mb);

//check connection
  if (modbus_connect(mb) == -1) {
     fprintf(stderr, "Connection failed: %s\n", modbus_strerror (errno));
     modbus_free(mb);
     return -1;
       }

  /* Read 5 registers from the address 0 */
  modbus_read_registers(mb, 0, 5, tab_reg);

  modbus_close(mb);
  modbus_free(mb);

return 0;
}
