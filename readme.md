Generate configuration binary:
   ~/esp/esp-idf/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate --version 1 config.csv config.bin 0x6000

Flash configuration binary:
   esptool -p /dev/ttyUSB0 write_flash 0x9000 ../config.bin

