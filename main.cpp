#include <modbus/modbus-tcp.h>
int main() {
    modbus_t *mb;
    uint16_t tab_reg[32];
    mb = modbus_new_tcp("",502);
    modbus_connect(mb);
    modbus_read_registers(mb,16386, 5, tab_reg);
    modbus_close(mb);
    modbus_free(mb);
    return 0;
}
