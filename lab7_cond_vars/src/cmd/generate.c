#include <stdint.h>
#include <stdio.h>

struct record {
  char name[80];
  char address[80];
  uint8_t semester;
};

int main() {
  FILE *file = fopen("build/data.bin", "w");
  int count = 42;
  for (int i = 0; i < count; i++) {
    struct record record = {0};
    sprintf(record.name, "Aboba %d", i);
    sprintf(record.address, "Street %d", i);
    record.semester = i;
    fwrite(&record, sizeof(record), 1, file);
  }
  fclose(file);
}
